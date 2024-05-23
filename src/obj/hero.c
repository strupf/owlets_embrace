// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"
#include "hero_hook.h"

#define HERO_DRAG_Y              256
#define HERO_GLIDE_VY            200
#define HERO_SPRINT_TICKS        60 // ticks walking until sprinting
#define HERO_VX_WALK             640
#define HERO_VX_SPRINT           900
#define HERO_VY_BOOST_SPRINT_ABS 0   // absolute vy added to a jump sprinted
#define HERO_VX_BOOST_SPRINT_Q8  300 // vx multiplier when jumping sprinted
#define HERO_VX_BOOST_SLIDE_Q8   370 // vx multiplier when jumping slided
#define HERO_DRAG_SLIDING        250
#define HERO_REEL_RATE           30
#define HERO_ROPEWALLJUMP_TICKS  30
#define HERO_SWIM_TICKS          50 // duration of swimming without upgrade
#define HERO_RUNUP_TICKS         50
#define HERO_ATTACK_TICKS        25
#define HERO_GRAVITY             80
#define HERO_SPRINT_DTAP_TICKS   20

#define HEROHOOK_N_HIST 4

typedef struct {
    i32 vy;
    i32 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    i32 v0;    // "jetpack" velocity, goes to v1 over ticks or less
    i32 v1;
} hero_jumpvar_s;

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND,
    HERO_JUMP_AIR_1,
    HERO_JUMP_AIR_2,
    HERO_JUMP_AIR_3,
    HERO_JUMP_WALL,
};

static const hero_jumpvar_s g_herovar[6] = {
    {825, 25, 70, 30},   // out of water
    {950, 30, 80, 40},   // ground
    {300, 25, 160, 40},  // airj 1
    {250, 25, 150, 40},  // airj 2
    {100, 25, 130, 0},   // airj 3
    {1000, 25, 80, 30}}; // wall jum

void          hero_on_update(game_s *g, obj_s *o);
void          hero_on_animate(game_s *g, obj_s *o);
//
static void   hero_start_jump(game_s *g, obj_s *o, i32 ID);
static void   hero_item_usage(game_s *g, obj_s *o, i32 state);
static void   hero_update_ladder(game_s *g, obj_s *o);
static void   hero_update_swimming(game_s *g, obj_s *o);
static void   hero_update_ground(game_s *g, obj_s *o);
static void   hero_update_air(game_s *g, obj_s *o, bool32 rope_stretched);
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout);
static i32    hero_can_walljump(game_s *g, obj_s *o);
static void   hero_restore_grounded_stuff(game_s *g, obj_s *o);

static bool32 hero_try_snap_to_ladder(game_s *g, obj_s *o, i32 diry)
{
    rec_i32 aabb    = obj_aabb(o);
    rec_i32 rladder = aabb;
    if (obj_grounded(g, o) && 0 < diry) {
        rladder = obj_rec_bottom(o);
    }

    v2_i32 lpos;
    if (!tile_map_ladder_overlaps_rec(g, rladder, &lpos)) return 0;

    hero_s *hero = &g->hero_mem;

    i32 posx = (lpos.x << 4) + 8 - (aabb.w / 2);
    aabb.x   = posx;
    aabb.y += diry;
    if (!game_traversable(g, aabb)) return 0;

    hero_unhook(g, o);
    o->flags &= ~OBJ_FLAG_MOVER;
    hero_restore_grounded_stuff(g, o);
    o->pos.x = posx;
    o->pos.y += diry;
    o->vel_q8.x        = 0;
    o->vel_q8.y        = 0;
    o->animation       = 0;
    hero->onladder     = 1;
    hero->ladderx      = posx;
    hero->sprint_ticks = 0;
    hero->attack_tick  = 0;
    return 1;
}

obj_s *hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->on_update       = hero_on_update;
    o->on_animate      = hero_on_animate;
    o->render_priority = 1000;

    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_TILE_COLLISION |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_CLAMP_TO_ROOM |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_SLOPES |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLIDE_Y_NEG;
    o->health_max      = g->save.health;
    o->health          = o->health_max;
    o->render_priority = RENDER_PRIO_HERO;
    o->n_sprites       = 2;
    o->drag_q8.x       = 256;
    o->drag_q8.y       = HERO_DRAG_Y;
    o->gravity_q8.y    = HERO_GRAVITY;
    o->vel_cap_q8.x    = 3000;
    o->vel_cap_q8.y    = 2500;
    o->w               = 12;
    o->h               = 26;
    o->facing          = 1;
    return o;
}

void hero_check_rope_intact(game_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (!ohook) return;

    rope_s *r = o->rope;
    if (!rope_intact(g, r)) {
        hook_destroy(g, o, ohook);
    }
}

static i32 hero_can_walljump(game_s *g, obj_s *o)
{

    if (!hero_has_upgrade(g, HERO_UPGRADE_WALLJUMP)) return 0;

#define WALLJUMP_MAX_DST 5
    i32 s = hero_determine_state(g, o, &g->hero_mem);
    if (s != HERO_STATE_AIR) return 0;

    i32 dst_l = 0xFF;
    i32 dst_r = 0xFF;
    for (i32 x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rl = {o->pos.x - 1 - x, o->pos.y, 1, o->h};
        if (!game_traversable(g, rl)) {
            dst_l = x;
            break;
        }
    }
    for (i32 x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rr = {o->pos.x + o->w + x, o->pos.y, 1, o->h};
        if (!game_traversable(g, rr)) {
            dst_r = x;
            break;
        }
    }

    if (dst_r == dst_l) return 0;
    if (dst_l < dst_r) return +1;
    if (dst_r < dst_l) return -1;
    return 0;
}

i32 hero_max_rope_len_q4(game_s *g)
{
    if (hero_has_upgrade(g, HERO_UPGRADE_HOOK_LONG))
        return HERO_ROPE_LEN_LONG;
    return HERO_ROPE_LEN_SHORT;
}

void hero_on_squish(game_s *g, obj_s *o)
{
}

void hero_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *sprite = &o->sprites[0];
    o->n_sprites         = 1;
    sprite->trec.t       = asset_tex(TEXID_HERO);
    sprite->flip         = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x       = o->w / 2 - 32;
    sprite->offs.y       = o->h - 64 + 16;
    sprite->mode         = ((o->invincible_tick >> 2) & 1) ? SPR_MODE_INV : 0;
    hero_s *h            = &g->hero_mem;
    i32     animID       = 0;
    i32     frameID      = 0;
    i32     state        = hero_determine_state(g, o, h);

    const bool32 was_idle   = h->is_idle;
    const i32    idle_animp = h->idle_anim;
    h->idle_anim            = 0;
    h->is_idle              = 0;

    if (h->sliding) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        animID  = 3;
        frameID = 0;
    }
    if (h->attack_tick || h->attack_hold_tick) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        sprite->offs.x += o->facing == 1 ? +12 : -6;
        animID = 11 + h->attack_flipflop;

        if (h->attack_hold_tick) {
            frameID = 0;
        } else {
            if (h->attack_tick <= 15) {
                frameID = lerp_i32(0, 5, h->attack_tick, 15);
            } else {
                frameID = 4;
            }
            frameID = min_i(frameID, 4);
        }
    }

    switch (state) {
    case HERO_STATE_GROUND: {
        if (0 < h->ground_impact_ticks && !h->carrying) {
            sprite->offs.y -= 4;
            animID  = 6; // "oof"
            frameID = 6;
            break;
        }

        sprite->offs.y -= 16;
        bool32 crawl = inp_pressed(INP_DPAD_D);

#if 1
        crawl = 0;
#endif

        if (crawl) {
            animID = 0; // todo
            if (o->vel_q8.x != 0) {
            } else {
            }
        } else {
            if (o->vel_q8.x != 0) {
                if (was_idle) {
                    animID  = 2;
                    frameID = 3;
                    break;
                }
                animID           = h->carrying ? 12 : 0; // "carrying/walking"
                i32 frameID_prev = (o->animation / 2000) & 7;
                o->animation += abs_i(o->vel_q8.x);
                frameID = (o->animation / 2000) & 7;
                if (frameID_prev != frameID && (frameID & 1) == 0) {
                    // snd_play_ext(SNDID_STEP, 0.5f, 1.f);
                    v2_i32 posc = obj_pos_bottom_center(o);
                    posc.x -= 16 + sgn_i(o->vel_q8.x) * 4;
                    posc.y -= 30;
                    rec_i32 trp = {0, 284, 32, 32};
                    spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, posc, TEXID_MISCOBJ,
                                       trp, 12, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
                }
                break;
            }

            // idle standing
            if (h->carrying) {
                animID = 11;
                o->animation++;
                i32 carryf = (o->animation >> 5) & 3;
                frameID    = (carryf == 0 ? 1 : 0);
            } else {
                animID     = 1; // "carrying/idle"
                h->is_idle = 1;
                h->idle_ticks++;
                if (!was_idle) {
                    h->idle_ticks = 0;
                }

                if (idle_animp) {
                    h->idle_anim = idle_animp + 1;
                } else if (100 <= h->idle_ticks && rngr_i32(0, 256) == 0) {
                    h->idle_anim = 1;
                }

#define HERO_TICKS_IDLE_SIT 60
                if (h->idle_anim) {
                    animID = 2;
                    if (HERO_TICKS_IDLE_SIT <= h->idle_anim) {
                        frameID = 12 + (((h->idle_anim - HERO_TICKS_IDLE_SIT) >> 6) & 1);
                    } else {
                        frameID = lerp_i32(0, 13, h->idle_anim, HERO_TICKS_IDLE_SIT);
                        frameID = clamp_i32(frameID, 0, 12);
                    }

                } else {
                    if (o->vel_prev_q8.x != 0) {
                        o->animation = 0; // just got idle
                    } else {
                        o->animation += 100;
                        frameID = (o->animation / 1500) & 3;
                    }
                }
            }
        }

        break;
    }

    case HERO_STATE_LADDER: {
        animID       = 8; // "ladder"
        frameID      = 0;
        sprite->flip = ((o->animation >> 3) & 1) ? SPR_FLIP_X : 0;
        break;
    }
    case HERO_STATE_SWIMMING: { // repurpose jumping animation for swimming
        if (inp_dpad_x()) {
            animID  = 9; // swim
            frameID = ((o->animation >> 3) % 6);
        } else {
            animID   = 6; // "air"
            i32 swim = ((o->animation >> 4) & 7);
            if (swim <= 1) {
                sprite->offs.y += swim == 0 ? 0 : -2;
                frameID = 2;
            } else {
                sprite->offs.y += swim - 2;
                frameID = 2 + ((swim - 2) % 3);
            }
        }
        break;
    }
    case HERO_STATE_AIR: {
        if (h->walljumpticks) {
            animID       = 6; // "walljump"
            sprite->flip = 0 < o->vel_q8.x ? 0 : SPR_FLIP_X;
            break;
        }

        if (h->carrying) {
            sprite->offs.y -= 16;
            animID = 12; // "carrying air"
            break;
        }

        animID = 6; // "air"
        if (0 < h->ground_impact_ticks) {
            frameID = 0;
        } else if (h->gliding) {
            frameID = 2 + (8 <= (o->animation & 63));
        } else if (+600 <= o->vel_q8.y) {
            frameID = 4 + ((o->animation >> 2) & 1);
        } else if (-10 <= o->vel_q8.y) {
            frameID = 3;
        } else if (-300 <= o->vel_q8.y) {
            frameID = 2;
        } else {
            frameID = 1;
        }
        break;
    }
    case HERO_STATE_DEAD: {
        sprite->offs.y -= 16;
        frameID = 0;
        animID  = 0;
    } break;
    }

    rec_i32 frame  = {frameID * 64, animID * 64, 64, 64};
    sprite->trec.r = frame;
}

void hero_hurt(game_s *g, obj_s *o, i32 damage)
{
    if (0 < o->invincible_tick) return;
    o->health -= damage;

    if (0 < o->health) {
        o->invincible_tick = ticks_from_ms(1000);
    } else {
        hero_kill(g, o);
    }
}

void hero_kill(game_s *g, obj_s *o)
{
    o->health = 0;
    o->flags &= ~OBJ_FLAG_CLAMP_ROOM_Y; // let hero fall through floor
    hero_unhook(g, o);
}

i32 hero_determine_state(game_s *g, obj_s *o, hero_s *h)
{
    if (o->health <= 0) return HERO_STATE_DEAD;

    i32 water_depth = water_depth_rec(g, obj_aabb(o));
    if (18 <= water_depth) return HERO_STATE_SWIMMING;

    bool32 grounded = obj_grounded(g, o) && 0 <= o->vel_q8.y;
    if (grounded) return HERO_STATE_GROUND;

    if (h->onladder) {
        if (tile_map_ladder_overlaps_rec(g, obj_aabb(o), NULL) &&
            h->ladderx == o->pos.x) {
            return HERO_STATE_LADDER;
        } else {
            h->onladder = 0;
        }
    }

    return HERO_STATE_AIR;
}

i32 hero_airjumps_max(game_s *g)
{
    if (hero_has_upgrade(g, HERO_UPGRADE_AIR_JUMP_3)) return 3;
    if (hero_has_upgrade(g, HERO_UPGRADE_AIR_JUMP_2)) return 2;
    if (hero_has_upgrade(g, HERO_UPGRADE_AIR_JUMP_1)) return 1;
    return 0;
}

i32 hero_airjumps_left(game_s *g)
{
    hero_s *h = &g->hero_mem;
    return h->airjumps_left;
}

static void hero_restore_grounded_stuff(game_s *g, obj_s *o)
{
    hero_s *h               = &g->hero_mem;
    g->jump_ui.out_of_water = 0;
    h->n_airjumps           = hero_airjumps_max(g);
    h->airjumps_left        = h->n_airjumps;
    h->swimticks            = HERO_SWIM_TICKS;
    h->runup_tick           = 0;
}

// tries to calculate a snapped ladder position
static bool32 hero_rec_on_ladder(game_s *g, rec_i32 aabb, rec_i32 *rout)
{
    i32 tx1 = (aabb.x >> 4);
    i32 tx2 = ((aabb.x + aabb.w - 1) >> 4);
    i32 ty  = ((aabb.y + (aabb.h >> 1)) >> 4);

    for (i32 tx = tx1; tx <= tx2; tx++) {
        if (g->tiles[tx + ty * g->tiles_x].collision != TILE_LADDER) continue;

        i32     lc_x = (tx << 4) + 8;
        rec_i32 r    = {lc_x - (aabb.w >> 1), aabb.y, aabb.w, aabb.h}; // aabb if on ladder
        if (!game_traversable(g, r)) continue;                         // if ladder position is valid
        if (rout) {
            *rout = r;
        }
        return 1;
    }
    return 0;
}

// tries to calculate a snapped ladder position
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout)
{
    return hero_rec_on_ladder(g, obj_aabb(o), rout);
}

bool32 hero_unhook(game_s *g, obj_s *o)
{
    if (obj_handle_valid(o->obj_handles[0])) {
        hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
        const i32 vx_abs = abs_i(o->vel_q8.x);
        const i32 vy_abs = abs_i(o->vel_q8.y);
        const i32 vx_sgn = sgn_i(o->vel_q8.x);
        const i32 vy_sgn = sgn_i(o->vel_q8.y);
        //
        i32       mulx   = 300;
        i32       muly   = 350;
        i32       addx   = 0;
        i32       addy   = 0;
        if (vx_abs < 600) {
            mulx = 400;
        }
        addx += vx_sgn * 200;

        if (vy_abs < 600) {
            muly = 400;
        }
        if (o->vel_q8.y < 0) {
            addy -= 250;
        }

        o->vel_q8.x = ((o->vel_q8.x * mulx) >> 8) + addx;
        o->vel_q8.y = ((o->vel_q8.y * muly) >> 8) + addy;

        return 1;
    }
    return 0;
}

static void hero_start_jump(game_s *g, obj_s *o, i32 ID)
{
    hero_s *h = &g->hero_mem;
#if 0
    spm_push();

    char *txt;
    txt_load("assets/jvar.json", spm_alloc, &txt);
    json_s js;
    json_root(txt, &js);

    g_herovar[1].ticks = jsonk_i32(js, "ticks");
    g_herovar[1].v0    = jsonk_i32(js, "v0");
    g_herovar[1].v1    = jsonk_i32(js, "v1");
    g_herovar[1].vy    = jsonk_i32(js, "vy");
    g_heropow          = jsonk_i32(js, "pow");
    HERO_GRAVITY       = jsonk_i32(js, "grav");
    spm_pop();
#endif
    snd_play_ext(SNDID_SPEAK, 1.f, 0.5f);
    hero_jumpvar_s jv  = g_herovar[ID];
    h->jump_index      = ID;
    h->edgeticks       = 0;
    h->jump_btn_buffer = 0;
    o->vel_q8.y        = -jv.vy;
    h->jumpticks       = jv.ticks;

    if (ID == HERO_JUMP_GROUND) {
        if (h->jump_boost_tick) {
            o->vel_q8.y        = (o->vel_q8.y * 300) >> 8;
            h->jump_boost_tick = 0;
        }

        v2_i32 posc = obj_pos_bottom_center(o);
        posc.x -= 16;
        posc.y -= 32;
        rec_i32 trp = {0, 284, 32, 32};
        spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                           trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
    }
}

static void hero_use_hook(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    snd_play_ext(SNDID_HOOK_THROW, 1.f, 1.f);
    // throw new hook
    i32 dirx = inp_dpad_x();
    i32 diry = inp_dpad_y();

    if (dirx == 0 && diry == 0) {
        diry = -1;
        dirx = o->facing;
    }

    v2_i32 center  = obj_pos_center(o);
    v2_i32 vlaunch = {dirx * 3000, diry * 2300};

    if (diry < 0) {
        vlaunch.y = (vlaunch.y * 5) / 4;
    } else if (diry == 0) {
        vlaunch.y = -600;
        vlaunch.x = (vlaunch.x * 5) / 4;
    }

    h->rope_active       = 1;
    h->reel_in           = 0;
    rope_s *rope         = &h->rope;
    obj_s  *hook         = hook_create(g, rope, center, vlaunch);
    o->obj_handles[0]    = obj_handle_from_obj(hook);
    hook->obj_handles[0] = obj_handle_from_obj(o);
    o->rope              = rope;
    o->ropenode          = rope->head;

    v2_i32 pcurr = v2_shl(center, 8);
    // "hint" the direction to the verlet sim
    v2_i32 pprev = {pcurr.x - (dirx << 9), pcurr.y - (diry << 9)};
    for (i32 n = 0; n < ROPE_VERLET_N; n++) {
        h->hookpt[n].p  = pcurr;
        h->hookpt[n].pp = pprev;
    }
}

static void hero_item_usage(game_s *g, obj_s *o, i32 state)
{
    hero_s *h      = &g->hero_mem;
    i32     dpad_x = inp_dpad_x();

    if (state == HERO_STATE_SWIMMING) {
        h->attack_tick = 0;
        hero_unhook(g, o);
    }

    if (state == HERO_STATE_LADDER || state == HERO_STATE_SWIMMING) return;

    bool32 grab_state = !h->carrying &&
                        state == HERO_STATE_GROUND &&
                        !o->rope &&
                        inp_pressed(INP_B);
    i32 grab_dir = (h->grabbingp ? h->grabbingp : dpad_x);

    if (grab_state && grab_dir) {
        rec_i32 grabrec = (0 < grab_dir ? obj_rec_right(o) : obj_rec_left(o));
        grabrec.h -= 2;

        if (map_blocked(g, o, grabrec, o->mass)) {
            // is grabbing
            if (h->grabbingp) {
                h->grabbing        = h->grabbingp;
                obj_s *obj_to_lift = NULL;
                for (obj_each(g, it)) {
                    if (!(it->flags & OBJ_FLAG_CARRYABLE)) continue;
                    if (overlap_rec(grabrec, obj_aabb(it))) {
                        obj_to_lift = it;
                        break;
                    }
                }

                if (obj_to_lift) {
                    obj_to_lift->vel_q8.x = 0;
                }

                if (inp_pressed(INP_DPAD_D)) {
                    if (obj_to_lift) {
                        rec_i32 rlift   = {0}; // target position on top of player
                        rec_i32 laabb   = obj_aabb(obj_to_lift);
                        v2_i32  liftpos = carryable_pos_on_hero(o, obj_to_lift, &rlift);

                        // check if object to lift can be moved in a L like
                        // shape around and on top of the player
                        rec_i32 rlift1 = {laabb.x,
                                          min_i32(rlift.y, laabb.y),
                                          laabb.w,
                                          laabb.h + abs_i32(laabb.y - rlift.y)};
                        rec_i32 rlift2 = {min_i32(rlift.x, laabb.x),
                                          rlift.y,
                                          laabb.w + abs_i32(laabb.x - rlift.x),
                                          laabb.h};

                        v2_i32 dtlift = v2_sub(liftpos, obj_to_lift->pos);

                        if (!map_blocked(g, obj_to_lift, rlift1, obj_to_lift->mass) &&
                            !map_blocked(g, obj_to_lift, rlift2, obj_to_lift->mass)) {
                            for (i32 n = 0; n < abs_i32(dtlift.y); n++) {
                                obj_step_y(g, obj_to_lift, sgn_i32(dtlift.y), 0, 0);
                            }
                            for (i32 n = 0; n < abs_i32(dtlift.x); n++) {
                                obj_step_x(g, obj_to_lift, sgn_i32(dtlift.x), 0, 0);
                            }

                            h->grabbing  = 0;
                            h->grabbingp = 0;
                            h->carrying  = 1;
                            obj_tag(g, obj_to_lift, OBJ_TAG_CARRIED);
                            obj_to_lift->carry.offs = v2_inv(dtlift);
                            carryable_on_lift(g, obj_to_lift);
                            assert(carryable_present(g));
                        } else {
                            h->trys_lifting = 1;
                        }

                    } else {
                        h->trys_lifting = 1;
                    }
                } else if (obj_to_lift) {
                    h->grabbing     = grab_dir;
                    h->grabbingp    = grab_dir;
                    h->trys_lifting = 1;
                    if (dpad_x) {
                        o->timer++;                // try pushing for a few ticks
                        if (dpad_x == -grab_dir && // pull
                            obj_step_x(g, o, dpad_x, 1, 0)) {
                            obj_step_x(g, obj_to_lift, dpad_x, 1, 0);
                        }
                        if (dpad_x == grab_dir && 20 <= o->timer && // push
                            obj_step_x(g, obj_to_lift, dpad_x, 1, 0)) {
                            obj_step_x(g, o, dpad_x, 1, 0);
                        }
                    } else {
                        o->timer = 0;
                    }
                }
            } else {
                assert(dpad_x);
                h->grabbing = dpad_x;
            }
            return;
        }
    }

    if (h->carrying) {
        if (inp_just_pressed(INP_B)) {
            obj_s *ocarry = carryable_present(g);
            assert(ocarry);
            carryable_on_drop(g, ocarry);
            h->carrying      = 0;
            ocarry->vel_q8.y = -1000;
            ocarry->vel_q8.x = o->facing * 1000;
        }
        return;
    }

    g->item_select.n_items = 2;

    switch (g->item_select.item) {
    case HERO_ITEM_HOOK: {
        assert(hero_has_upgrade(g, HERO_UPGRADE_HOOK));
        if (inp_just_pressed(INP_B)) {
            if (o->rope) {
                hero_unhook(g, o);
            } else {
                hero_use_hook(g, o);
                obj_s *ohook    = obj_from_obj_handle(o->obj_handles[0]);
                ohook->substate = 1;
            }
            break;
        }

        u32 rlen = o->rope ? rope_length_q4(g, o->rope) : 0;

        if (o->rope && h->reel_in) {
            ropenode_s *rnode = ropenode_neighbour(o->rope, o->ropenode);
            v2_i32      dtr   = v2_sub(rnode->p, o->pos);
            dtr               = v2_setlen(dtr, 600);
            o->gravity_q8.y   = 0;
            o->vel_q8         = v2_add(o->vel_q8, dtr);

            if (rlen <= HERO_ROPE_LEN_MIN) {
                h->reel_in = 0;
                rlen       = HERO_ROPE_LEN_MIN;
            }

            rope_set_len_max_q4(o->rope, rlen);
        }

        if (inp_just_pressed(INP_DPAD_U) && o->rope &&
            HERO_ROPE_LEN_MIN < rlen) {
            i32 time = time_now();
            if ((time - h->reel_in_dtap) <= 10) {
                h->reel_in = 1;
            }
            h->reel_in_dtap = time;
            break;
        }

        if (o->rope) {
            i32 dt_crank = inp_crank_dt_q12();
            u32 l_new_q4 = o->rope->len_max_q4 + ((dt_crank * 200) >> 12);
            l_new_q4     = clamp_i(l_new_q4,
                                   HERO_ROPE_LEN_MIN,
                                   hero_max_rope_len_q4(g));
            rope_set_len_max_q4(o->rope, l_new_q4);
        }
        break;
    }
    case HERO_ITEM_WEAPON: {
        if (h->attack_hold_tick) {
            if (inp_just_released(INP_B)) {
                h->attack_hold_tick = 0;
                h->attack_tick      = 1;
                h->attack_flipflop  = 1 - h->attack_flipflop;
                hitbox_s hb         = {0};
                hb.damage           = 1;
                hb.r.h              = 32;
                hb.r.w              = 48;
                hb.r.y              = o->pos.y - 5;
                hb.force_q8.x       = o->facing * 800;
                hb.force_q8.y       = -400;
                if (o->facing == 1) {
                    hb.r.x = o->pos.x + o->w;
                } else {
                    hb.r.x = o->pos.x - hb.r.w;
                }
                obj_game_player_attackbox(g, hb);
                rec_i32 rslash = {0, 384, 64, 64};
                v2_i32  dcpos  = {0, -40};
                i32     flip   = 0;

                if (o->facing == 1) {

                } else {
                    flip    = SPR_FLIP_X;
                    dcpos.x = -40;
                }

                if (h->attack_flipflop) {
                    rslash.y += 64;
                }
                spritedecal_create(g, 0xFFFF, o, dcpos, TEXID_MISCOBJ, rslash, 12, 5, flip);
            } else {
                h->attack_hold_tick++;
            }
        } else if (h->attack_tick == 0 || 2 <= h->attack_tick) {
            if (inp_just_pressed(INP_B)) {
                h->attack_hold_tick = 1;
            }
        }
        break;
    }
    }
}

static bool32 hero_is_sprinting(hero_s *h)
{
    return (h->sprint_ticks);
}

void hero_on_update(game_s *g, obj_s *o)
{
    inp_s   inp = inp_state();
    hero_s *h   = &g->hero_mem;
#if !GAME_JUMP_ATTACK && 0
    if (g->item_selector.item != 0 && herodata->rope_active &&
        g->herodata.upgrades[HERO_UPGRADE_WHIP]) {
        hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
    }
#endif

    const v2_i32 v_og   = o->vel_q8;
    const i32    state  = hero_determine_state(g, o, h);
    i32          dpad_x = inps_dpad_x(inp);
    i32          dpad_y = inps_dpad_y(inp);
    if (state == HERO_STATE_DEAD) {
        dpad_x = 0;
        dpad_y = 0;
    }

    if (!(state == HERO_STATE_GROUND && 0 < dpad_y)) {
        h->jump_boost_tick = 0;
    }

    o->drag_q8.x = 250;
    o->drag_q8.y = HERO_DRAG_Y;
    o->flags |= OBJ_FLAG_MOVER;
    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    o->moverflags |= OBJ_MOVER_SLOPES;
    o->moverflags &= ~OBJ_MOVER_GLUE_TOP;
    o->moverflags &= ~OBJ_MOVER_SLOPES_TOP;

    o->gravity_q8.y = HERO_GRAVITY;
    h->grabbingp    = h->grabbing;
    h->grabbing     = 0;
    h->trys_lifting = 0;
    h->thrustingp   = h->thrusting;
    h->thrusting    = 0;
    bool32 sliding  = (0 < dpad_y &&
                      h->sliding &&
                      (state == HERO_STATE_AIR || state == HERO_STATE_GROUND));
    if (sliding) {
        o->drag_q8.x = HERO_DRAG_SLIDING;
    } else {
        h->sliding = 0;
    }

    if (h->walljumpticks) {
        h->walljumpticks--;
    }

    if (h->attack_tick) {
        h->attack_tick++;
        if (HERO_ATTACK_TICKS <= h->attack_tick) {
            h->attack_tick = 0;
        }
    }

    h->jump_btn_buffer--;
    h->ground_impact_ticks--;

    if (!inps_pressed(inp, INP_A) || state != HERO_STATE_AIR) {
        h->gliding = 0;
    }

    if ((state != HERO_STATE_DEAD) && (state != HERO_STATE_SWIMMING || !h->diving)) {
        h->breath_ticks -= g->save.upgrades[HERO_UPGRADE_DIVE] ? 100 : 5;
        h->breath_ticks = max_i(h->breath_ticks, 0);
    }

    if (state != HERO_STATE_AIR && state != HERO_STATE_GROUND) {
        h->sprint_ticks = 0;
        h->edgeticks    = 0;
    }

    bool32 grabbing = h->grabbingp || h->trys_lifting;

    o->facing_locked = grabbing;
    if (h->attack_tick) {
        o->facing_locked = 1;
    }

    if (!o->facing_locked) {
        if (h->sliding) {
            o->facing = o->vel_q8.x ? sgn_i(o->vel_q8.x) : o->facing;
        } else if (dpad_x) {
            o->facing = dpad_x;
        }
    }
    if (!dpad_x) {
        h->sprint_ticks = 0;
    }

    hero_item_usage(g, o, state);

    bool32 rope_stretched = (o->rope && (o->rope->len_max_q4 * 254) <= (rope_length_q4(g, o->rope) << 8));

    if ((o->bumpflags & OBJ_BUMPED_Y) && !(o->bumpflags & OBJ_BUMPED_Y_BOUNDS)) {
        if (state == HERO_STATE_AIR && rope_stretched) {
            o->vel_q8.y = -(o->vel_q8.y >> 2);
        } else {
            if (1000 <= o->vel_q8.y) {
                f32 vol = (1.f * (f32)o->vel_q8.y) / 2000.f;
                snd_play_ext(SNDID_STEP, min_f(vol, 1.f) * 1.2f, 1.f);
            }

            if (1000 <= o->vel_q8.y && inp_pressed(INP_DPAD_D)) {
                h->jump_boost_tick = 1;
            }

            if (250 <= o->vel_q8.y) {
                h->ground_impact_ticks = min_i(o->vel_q8.y >> 8, 8);
                v2_i32 posc            = obj_pos_bottom_center(o);
                posc.x -= 16;
                posc.y -= 32;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                                   trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            o->vel_q8.y = 0;
        }
    }

    if (o->bumpflags & OBJ_BUMPED_ON_HEAD) {
        o->vel_q8.y = -1000;
    }

    if (o->bumpflags & OBJ_BUMPED_X) {
        h->sprint_ticks = 0;
        if (h->sliding) {
            o->vel_q8.x = -(o->vel_q8.x >> 2);
        } else if (state == HERO_STATE_AIR && rope_stretched) {
            if (dpad_x == sgn_i(o->vel_q8.x) || abs_i(o->vel_q8.x) < 600) {
                o->vel_q8.x = 0;
            } else {
                o->vel_q8.x = -(o->vel_q8.x >> 2);
            }

        } else {
            o->vel_q8.x = 0;
        }
    }
    o->bumpflags = 0;

    if (hero_has_upgrade(g, HERO_UPGRADE_WALLJUMP)) {
        // wall run tick
        bool32 wallr       = !game_traversable(g, obj_rec_right(o));
        bool32 walll       = !game_traversable(g, obj_rec_left(o));
        bool32 may_wallrun = (dpad_x == +1 && wallr) || (dpad_x == -1 && walll);

        switch (state) {
        case HERO_STATE_AIR:
            if (!h->runup_tick && may_wallrun) {
                u32 vlen = v2_lensq(v_og);
                if (200000U <= vlen && v_og.y <= 300)
                    h->runup_tick = min_i(vlen / 10000U, HERO_RUNUP_TICKS);
            }

            if (h->runup_tick && may_wallrun) {
                h->runup_tick--;
                o->vel_q8.y = -h->runup_tick * 20;
            } else {
                if (may_wallrun) {
                    o->drag_q8.y = 210;
                }
                h->runup_tick = 0;
            }
            break;
        }
    }

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_AIR: {
        if (!inps_just_pressed(inp, INP_DPAD_U) && !inps_just_pressed(inp, INP_DPAD_D)) break;
        if (hero_try_snap_to_ladder(g, o, dpad_y)) {
            return; // RETURN
        }
        break;
    }
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        h->sprint_ticks = 0;
        break;
    }

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_LADDER: {
        hero_restore_grounded_stuff(g, o);
        break;
    }
    }

    if (state != HERO_STATE_AIR) {
        // hero->runup_tick = 50;
    }

    switch (state) {
    case HERO_STATE_GROUND:
        if (h->trys_lifting) break;
        o->drag_q8.x = 250;
        hero_update_ground(g, o);
        break;
    case HERO_STATE_AIR:
        hero_update_air(g, o, rope_stretched);
        break;
    case HERO_STATE_SWIMMING:
        o->drag_q8.x            = 240;
        g->jump_ui.out_of_water = 1;
        h->sprint_dtap          = 0;
        h->sprint_ticks         = 0;
        hero_update_swimming(g, o);
        break;
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        h->sprint_dtap  = 0;
        h->sprint_ticks = 0;
        hero_update_ladder(g, o);
        break;
    case HERO_STATE_DEAD: {
        i32    wdepth   = water_depth_rec(g, obj_aabb(o));
        bool32 grounded = obj_grounded(g, o);
        bool32 inwater  = (wdepth && !grounded);
        o->drag_q8.x    = grounded || inwater ? 210 : 245;
        h->sprint_dtap  = 0;
        h->sprint_ticks = 0;

        if (inwater) {
            o->drag_q8.y    = 240;
            o->gravity_q8.y = 0;
        }
        break;
    }
    }

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook) {
        hook_update(g, o, ohook);
    }
}

static void hero_update_ladder(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    if (o->pos.x != h->ladderx) {
        h->onladder = 0;
        return;
    }

    if (inp_just_pressed(INP_A)) {
        h->onladder = 0;
        hero_start_jump(g, o, 0);
        i32 dpad_x  = inp_dpad_x();
        o->vel_q8.x = dpad_x * 200;
        return;
    }

    if (inp_just_pressed(INP_B)) {
        h->onladder = 0;
        return;
    }

    i32 dpad_y = inp_dpad_y();
    i32 dir_y  = dpad_y << 1;

    for (i32 m = abs_i(dir_y); m; m--) {
        rec_i32 aabb = obj_aabb(o);
        aabb.y += dpad_y;
        if (!game_traversable(g, aabb)) break;
        if (obj_grounded_at_offs(g, o, (v2_i32){0, dpad_y})) {
            o->pos.y += dpad_y;
            h->onladder = 0;
            break;
        }
        if (!tile_map_ladder_overlaps_rec(g, aabb, NULL)) break;
        o->pos.y += dpad_y;
    }

    if (dir_y) {
        o->animation++;
    }
}

bool32 hero_is_submerged(game_s *g, obj_s *o, i32 *water_depth)
{
    i32 wd = water_depth_rec(g, obj_aabb(o));
    if (water_depth) {
        *water_depth = wd;
    }
    return (o->h <= wd);
}

i32 hero_breath_tick(game_s *g)
{
    hero_s *h = &g->hero_mem;
    return h->breath_ticks;
}

i32 hero_breath_tick_max(game_s *g)
{
    return (g->save.upgrades[HERO_UPGRADE_DIVE] ? 2500 : 100);
}

static void hero_update_swimming(game_s *g, obj_s *o)
{
    hero_s *h        = &g->hero_mem;
    h->airjumps_left = 0;

    i32    dpad_x      = inp_dpad_x();
    i32    dpad_y      = inp_dpad_y();
    i32    water_depth = 0;
    bool32 submerged   = hero_is_submerged(g, o, &water_depth);

    if (!submerged) {
        h->diving = 0;
    }

    o->animation++;

    if (h->diving && hero_has_upgrade(g, HERO_UPGRADE_DIVE)) {
        o->drag_q8.y    = 230;
        o->gravity_q8.y = 0;
        // o->vel_q8.y -= HERO_GRAVITY / 2;

        if (dpad_y) {
            i32 i0 = (dpad_y == sgn_i(o->vel_q8.y) ? abs_i(o->vel_q8.y) : 0);
            i32 ay = (max_i(512 - i0, 0) * 128) >> 8;
            o->vel_q8.y += ay * dpad_y;
        }
    } else {
        o->drag_q8.y = 230;
        h->swimticks--; // swim ticks are reset when grounded later on
        if (0 < h->swimticks || hero_has_upgrade(g, HERO_UPGRADE_SWIM)) {
            i32 i0 = pow_i32(min_i(water_depth, 70), 2);
            i32 i1 = pow_i32(70, 2);
            i32 k0 = min_i(water_depth, 30);
            i32 k1 = 30;

            o->vel_q8.y -= lerp_i32(20, 70, k0, k1) + lerp_i32(0, 100, i0, i1);

        } else {
            o->vel_q8.y -= min_i(5 + water_depth, 40);
            h->diving = 1;
        }

        if (hero_has_upgrade(g, HERO_UPGRADE_DIVE) && inp_pressed(INP_DPAD_D)) {
            o->tomove.y += 10;
            o->vel_q8.y = +1000;
            h->diving   = 1;
        } else if (5 < water_depth && water_depth < 30 && inp_just_pressed(INP_A)) {
            o->tomove.y  = -water_depth / 2;
            o->drag_q8.y = 256;
            hero_start_jump(g, o, HERO_JUMP_WATER);
        }
    }

    if (dpad_x != sgn_i(o->vel_q8.x)) {
        o->vel_q8.x /= 2;
    }
    if (dpad_x) {
        i32 i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        i32 ax = (max_i(512 - i0, 0) * 32) >> 8;
        o->vel_q8.x += ax * dpad_x;
    }

    if (submerged && h->diving) {
        i32 breath_tm   = hero_breath_tick_max(g);
        h->breath_ticks = min_i(h->breath_ticks + 1, breath_tm);
        if (breath_tm <= h->breath_ticks) {
            hero_kill(g, o);
        }
    }
}

static void hero_update_ground(game_s *g, obj_s *o)
{
    hero_s *h    = &g->hero_mem;
    h->edgeticks = 6;

    if (h->sprint_dtap) {
        h->sprint_dtap += sgn_i32(h->sprint_dtap);
        if (HERO_SPRINT_DTAP_TICKS <= abs_i32(h->sprint_dtap)) {
            h->sprint_dtap = 0;
        }
    }

    i32 dpad_x = inp_dpad_x();
    i32 dpad_y = inp_dpad_y();

    v2_i32 posc         = obj_pos_center(o);
    obj_s *interactable = obj_closest_interactable(g, posc);
    h->interactable     = obj_handle_from_obj(interactable);

    if (0 < dpad_y && !o->rope) { // sliding
        h->sprint_dtap = 0;
        i32 accx       = 0;
        if (0) {
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+1, 0})) {
            accx = +85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-1, 0})) {
            accx = -85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+2, 0})) {
            accx = +40;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-2, 0})) {
            accx = -40;
        }

        if (accx) {
            h->sliding = 1;
        }

        if (h->sliding) {
            o->drag_q8.x = accx ? HERO_DRAG_SLIDING : 245;
            o->vel_q8.x += accx;
        }

        if (h->sliding && rngr_i32(0, 2)) {
            particle_desc_s prt = {0};
            v2_i32          bc  = obj_pos_bottom_center(o);
            bc.y -= 2;
            prt.p.p_q8      = v2_shl(bc, 8);
            prt.p.v_q8.x    = 0;
            prt.p.v_q8.y    = -rngr_i32(100, 200);
            prt.p.a_q8.y    = 20;
            prt.p.size      = 2;
            prt.p.ticks_max = 70;
            prt.ticksr      = 20;
            prt.pr_q8.x     = 2000;
            prt.pr_q8.y     = 2000;
            prt.vr_q8.x     = 100;
            prt.vr_q8.y     = 100;
            prt.ar_q8.y     = 4;
            prt.sizer       = 1;
            prt.p.gfx       = PARTICLE_GFX_CIR;
            particles_spawn(g, &g->particles, prt, 1);
        }
    }

    if ((inp_just_pressed(INP_A) || 0 < h->jump_btn_buffer)) {
        hero_start_jump(g, o, HERO_JUMP_GROUND);
        if (h->sliding) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SLIDE_Q8) >> 8;
        }
        if (hero_is_sprinting(h)) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SPRINT_Q8) >> 8;
            o->vel_q8.y -= HERO_VY_BOOST_SPRINT_ABS;
        }
        h->ground_impact_ticks = 6;
    }

    if (!h->sliding) {
        if (dpad_x != sgn_i(o->vel_q8.x)) {
            o->vel_q8.x /= 2;
        }

        if (inp_pressed(INP_DPAD_D) && h->jump_boost_tick) {
            h->jump_boost_tick++;
            if (100 <= h->jump_boost_tick) {
                h->jump_boost_tick = 0;
            }
        }

        if (inp_just_pressed(INP_DPAD_D)) {
            o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
            o->tomove.y += 1;
        }

        if (o->vel_prev_q8.x == 0 && o->vel_q8.x != 0) {
            o->animation = 0;
        }

        i32 dtap_s = 0;
        if (inp_just_pressed(INP_DPAD_L)) {
            dtap_s = -1;
        }
        if (inp_just_pressed(INP_DPAD_R)) {
            dtap_s = +1;
        }
        if (!hero_is_sprinting(h) && hero_has_upgrade(g, HERO_UPGRADE_SPRINT) &&
            dtap_s != 0) {

            if (h->sprint_dtap == 0) {
                h->sprint_dtap = dtap_s;
            } else if (sgn_i32(h->sprint_dtap) == dtap_s) {
                h->sprint_ticks = 1;
            }
        }

        if (dpad_x) {
            i32 vt = HERO_VX_WALK;
            if (h->sprint_ticks) {
                vt = lerp_i32(vt, HERO_VX_SPRINT, min_i32(h->sprint_ticks++, 10), 10);
            }
            i32 i0 = (dpad_x == sgn_i32(o->vel_q8.x) ? abs_i32(o->vel_q8.x) : 0);
            i32 ax = (max_i32(vt - i0, 0) * 256) >> 8;
            o->vel_q8.x += ax * dpad_x;
        } else {
            h->sprint_ticks = 0;
        }
    }
}

static void hero_update_air(game_s *g, obj_s *o, bool32 rope_stretched)
{
    hero_s *h         = &g->hero_mem;
    i32     dpad_x    = inp_dpad_x();
    i32     dpad_y    = inp_dpad_y();
    bool32  usinghook = o->rope != NULL;
    o->animation++;
    h->edgeticks--;

    rec_i32 rec_l = obj_rec_left(o);
    rec_i32 rec_r = obj_rec_right(o);
    rec_l.h -= 8;
    rec_r.h -= 8;
    rec_l.y += 4;
    rec_r.y += 4;

#if 0
    bool32 touch_l = !game_traversable(g, rec_l);
    bool32 touch_r = !game_traversable(g, rec_r);
    bool32 touch_t = !game_traversable(g, obj_rec_top(o));
    bool32 touch_s = touch_l || touch_r;

    touch_t = 0;

    if (touch_s || touch_t) {
        // o->moverflags |= OBJ_MOVER_GLUE_TOP;
        // o->moverflags |= OBJ_MOVER_SLOPES_TOP;
        o->moverflags &= ~OBJ_MOVER_SLOPES;
        o->gravity_q8.y = 0;
        o->vel_q8.x     = 0;
        o->vel_q8.y     = 0;

        if (touch_s) {
            if (inps_just_pressed(inp, INP_A)) {
                hero_start_jump(g, o, HERO_JUMP_WALL);
                i32 dir     = touch_r ? -1 : +1;
                o->vel_q8.x = dir * 1000;
            } else {
                o->tomove.y += dpad_y * 3;
            }
        }
        if (touch_t) {
            o->tomove.x += dpad_x * 2;
            if (inps_just_pressed(inp, INP_A)) {
                o->tomove.y = +1;
            }
        }
    }
#else
    bool32 touch_t = 0;
    bool32 touch_s = 0;
#endif

    if (rope_stretched) {
        o->drag_q8.x = 253;
        o->drag_q8.y = 256;

        if (inp_pressed(INP_A) && 0 < h->jumpticks) {
            h->jumpticks--;
            i32 vadd = lerp_i32(20, 50, h->jumpticks, HERO_ROPEWALLJUMP_TICKS);
            o->vel_q8.x += h->ropewalljump_dir * vadd;
        } else {
            h->jumpticks = 0;
        }

        if (inp_just_pressed(INP_A)) {
            h->jump_btn_buffer = 8;
        }

        v2_i32 rn_curr  = o->ropenode->p;
        v2_i32 rn_next  = ropenode_neighbour(o->rope, o->ropenode)->p;
        v2_i32 dtrope   = v2_sub(rn_next, rn_curr);
        i32    dtrope_s = sgn_i(dtrope.x);
        i32    dtrope_a = abs_i(dtrope.x);

        if (dpad_x) {
            if (dtrope_s == dpad_x) {
                o->vel_q8.x += 60 * dpad_x;
            } else {
                o->vel_q8.x += 15 * dpad_x;
            }
        }
    } else if (!touch_s && !touch_t) {
        o->drag_q8.x = h->sliding ? 253 : 240;

        // dynamic jump height
        if (0 < h->jumpticks && !inp_pressed(INP_A)) {
            h->jumpticks = 0;
        }

        if (0 < h->jumpticks) {
            hero_jumpvar_s jv = g_herovar[h->jump_index];
            i32            t0 = pow_i32(jv.ticks, 2);
            i32            ti = pow_i32(h->jumpticks, 2) - t0;
            o->vel_q8.y -= jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
        }

        if (inp_just_pressed(INP_A)) {
            if (hero_has_upgrade(g, HERO_UPGRADE_GLIDE) && inp_pressed(INP_DPAD_U)) {
                h->gliding      = 1;
                h->sprint_ticks = 0;
            } else {
                h->jump_btn_buffer = 12;
            }
        }

        if (h->gliding) {
            if (HERO_GLIDE_VY < o->vel_q8.y) {
                o->vel_q8.y -= HERO_GRAVITY * 2;
                o->vel_q8.y = max_i(o->vel_q8.y, HERO_GLIDE_VY);
            }
            h->jump_btn_buffer = 0;
        }

        h->jumpticks--;

        if (dpad_x) {
            i32 i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
            i32 ax = (max_i(HERO_VX_WALK - i0, 0) * 32) >> 8;
            o->vel_q8.x += ax * dpad_x;
            o->drag_q8.x = 256;
        }

        if (dpad_x != sgn_i(h->sprint_ticks)) {
            h->sprint_ticks = 0;
        }
    }

    if (!usinghook && 0 < dpad_y) {
        if (inp_just_pressed(INP_DPAD_D)) {
            o->vel_q8.y  = max_i(o->vel_q8.y, 500);
            h->thrusting = 1;
        } else if (h->thrustingp) {
            o->vel_q8.y += 70;
            h->thrusting = 1;
        }
    }

    if (0 < h->jump_btn_buffer) {
        i32    jump_wall   = rope_stretched ? 0 : hero_can_walljump(g, o);
        bool32 jump_ground = 0 < h->edgeticks;
        bool32 jump_midair = !usinghook &&           // not hooked
                             !jump_ground &&         // jump in air?
                             0 < h->airjumps_left && // air jumps left?
                             h->jumpticks <= -12;    // wait some ticks after last jump

        if (jump_midair && !jump_ground && !jump_wall) { // just above ground -> ground jump
            for (i32 y = 0; y < 8; y++) {
                rec_i32 rr = {o->pos.x, o->pos.y + o->h + y, o->w, 1};
                v2_i32  pp = {0, y + 1};
                if (game_traversable(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                    jump_ground = 1;
                    break;
                }
            }
        }

        if (jump_ground) {
            hero_restore_grounded_stuff(g, o);
            hero_start_jump(g, o, HERO_JUMP_GROUND);
        } else if (jump_wall && inp_dpad_x() == jump_wall) {
            o->vel_q8.x      = jump_wall * 520;
            h->walljumpticks = 12;
            hero_start_jump(g, o, HERO_JUMP_WALL);
        } else if (jump_midair) {
            i32 jumpindex = HERO_JUMP_AIR_1 + h->n_airjumps - h->airjumps_left;
            h->airjumps_left--;
            h->sprint_ticks = 0;
            hero_start_jump(g, o, jumpindex);

            rec_i32 rwind = {0, 0, 64, 64};
            v2_i32  dcpos = obj_pos_center(o);
            dcpos.x -= 32;
            dcpos.y += 0;
            i32 flip = rngr_i32(0, 1) ? 0 : SPR_FLIP_X;

            spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, dcpos, TEXID_WINDGUSH, rwind, 18, 6, flip);
        }
    }
}

void hero_post_update(game_s *g, obj_s *o)
{
    if (!o) return;
    assert(0 < o->health);

    v2_i32  hcenter      = obj_pos_center(o);
    hero_s *hero         = &g->hero_mem;
    rec_i32 heroaabb     = obj_aabb(o);
    bool32  herogrounded = obj_grounded(g, o);

    // hero touching other objects
    for (obj_each(g, it)) {
        switch (o->ID) {
        case OBJ_ID_SHROOMY: {
            if (herogrounded) break;
            rec_i32 rs = obj_aabb(it);
            rec_i32 ri;
            if (!intersect_rec(heroaabb, rs, &ri)) break;
            if (0 < o->vel_q8.y &&
                (heroaabb.y + heroaabb.h) < (rs.y + rs.h) &&
                o->posprev.y < o->pos.y) {
                o->vel_q8.y = -2000;
                o->tomove.y -= ri.h;
                shroomy_bounced_on(it);
            }
            break;
        }
        }
    }

    // touched hurting things?
    if (o->invincible_tick <= 0) {
        i32    hero_dmg       = 0;
        v2_i32 hero_knockback = {0};

        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_HURT_ON_TOUCH)) continue;
            if (!overlap_rec(heroaabb, obj_aabb(it))) continue;
            if (hero->thrusting) {
                hero->thrusting = 0;
                o->vel_q8.y     = -2000;
                continue;
            }
            continue; // disable
            v2_i32 ocenter   = obj_pos_center(it);
            v2_i32 dt        = v2_sub(hcenter, ocenter);
            hero_knockback.x = sgn_i(dt.x) * 1000;
            hero_knockback.y = -1000;
            hero_dmg         = max_i(hero_dmg, 1);

            switch (it->ID) {
            case OBJ_ID_CHARGER: {
                int pushs        = sgn_i(hcenter.x - ocenter.x);
                hero_knockback.x = pushs * 2000;
                break;
            }
            }
        }

        if (hero_dmg) {
            hero_hurt(g, o, hero_dmg);
            snd_play_ext(SNDID_SWOOSH, 0.5f, 0.5f);
            o->vel_q8 = hero_knockback;
            o->bumpflags &= ~OBJ_BUMPED_Y; // have to clr y bump
            g->events_frame |= EVENT_HERO_DAMAGE;
        }
    }

    // possibly enter new substates
    hero_check_rope_intact(g, o);
    if (o->health <= 0) {
        gameover_start(g);
    } else {
        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_HOVER_TEXT)) continue;
            v2_i32 ocenter = obj_pos_center(it);
            u32    dist    = v2_distancesq(hcenter, ocenter);

            if (dist < 3000 && it->hover_text_tick < OBJ_HOVER_TEXT_TICKS) {
                it->hover_text_tick++;
            } else if (it->hover_text_tick) {
                it->hover_text_tick--;
            }
        }

        i32 roomtilex = hcenter.x / SYS_DISPLAY_W;
        i32 roomtiley = hcenter.y / SYS_DISPLAY_H;
        hero_set_visited_tile(g, g->map_worldroom, roomtilex, roomtiley);

        bool32 collected_upgrade = 0;

        for (obj_each(g, it)) {
            if (it->ID != OBJ_ID_HEROUPGRADE) continue;
            if (!overlap_rec(heroaabb, obj_aabb(it))) continue;
            heroupgrade_on_collect(g, it);
            obj_delete(g, it);
            objs_cull_to_delete(g);
            collected_upgrade = 1;
            break;
        }

        if (!collected_upgrade) {
            bool32 t = maptransition_try_hero_slide(g);
            if (t == 0 && inp_just_pressed(INP_DPAD_U)) { // nothing happended
                obj_s *interactable = obj_from_obj_handle(hero->interactable);
                if (interactable && interactable->on_interact) {
                    interactable->on_interact(g, interactable);
                    hero->interactable = obj_handle_from_obj(NULL);
                }
            }
        }
    }
}