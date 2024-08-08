// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "app.h"
#include "game.h"
#include "hero_hook.h"

typedef struct {
    i32 index;
    i32 ticks;
} hero_jump_s;

const hero_jumpvar_s g_herovar[NUM_HERO_JUMP] = {
    {800, 30, 100, 30},  // out of water
    {1300, 35, 50, 20},  // ground
    {1500, 40, 50, 20},  // ground boosted
    {600, 50, 140, 20},  // fly
    {1000, 25, 80, 30}}; // wall jump

void hero_on_update(game_s *g, obj_s *o);
void hero_on_animate(game_s *g, obj_s *o);
void hero_update_ground(game_s *g, obj_s *o);
void hero_update_climb(game_s *g, obj_s *o);
void hero_update_ladder(game_s *g, obj_s *o);
void hero_update_swimming(game_s *g, obj_s *o);
void hero_update_air(game_s *g, obj_s *o, bool32 rope_stretched);
i32  hero_can_walljump(game_s *g, obj_s *o);

obj_s *hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->on_update       = hero_on_update;
    o->on_animate      = hero_on_animate;
    o->render_priority = 1000;

    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_CLAMP_TO_ROOM |
               OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLIDE_Y_NEG;
    o->health_max      = g->save.health;
    o->health          = o->health_max;
    o->render_priority = RENDER_PRIO_HERO;
    o->grav_q8.y       = HERO_GRAVITY;
    o->v_cap_y_q8_pos  = +1792; // multiple of 256
    o->v_cap_y_q8_neg  = -2500;
    o->v_cap_x_q8      = 3000;
    o->w               = 12;
    o->h               = HERO_HEIGHT;
    o->facing          = 1;
    o->n_sprites       = 1;
    return o;
}

void hero_check_rope_intact(game_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    hero_s *h     = &g->hero_mem;
    obj_s  *ohook = obj_from_obj_handle(h->hook);
    if (!ohook) return;

    rope_s *r = o->rope;
    if (!rope_intact(g, r)) {
        hook_destroy(g, o, ohook);
    }
}

i32 hero_can_walljump(game_s *g, obj_s *o)
{
#define WALLJUMP_MAX_DST 5
    i32 s = hero_determine_state(g, o, &g->hero_mem);
    if (s != HERO_STATE_AIR) return 0;

    i32 dst_l = 0xFF;
    i32 dst_r = 0xFF;
    for (i32 x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rl = {o->pos.x - 1 - x, o->pos.y, 1, o->h};
        if (!map_traversable(g, rl)) {
            dst_l = x;
            break;
        }
    }
    for (i32 x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rr = {o->pos.x + o->w + x, o->pos.y, 1, o->h};
        if (!map_traversable(g, rr)) {
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
    return HERO_ROPE_LEN_LONG;
}

void hero_on_squish(game_s *g, obj_s *o)
{
}

void hero_hurt(game_s *g, obj_s *o, i32 damage)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    if (h->invincibility_ticks) return;
    o->health = max_i32((i32)o->health - damage, 0);

    if (o->health) {
        h->invincibility_ticks = ticks_from_ms(1000);
    } else {
        hero_kill(g, o);
    }
}

void hero_kill(game_s *g, obj_s *o)
{

    o->health = 0;
    o->flags &= ~OBJ_FLAG_CLAMP_ROOM_Y; // let hero fall through floor
    hero_action_ungrapple(g, o);
    if (g->substate != SUBSTATE_GAMEOVER) {
        gameover_start(g);
    }
}

i32 hero_determine_state(game_s *g, obj_s *o, hero_s *h)
{
    if (o->health == 0) return HERO_STATE_DEAD;

    i32 water_depth = water_depth_rec(g, obj_aabb(o));
    if (18 <= water_depth) return HERO_STATE_SWIMMING;

    if (obj_grounded(g, o) && 0 <= o->v_q8.y) return HERO_STATE_GROUND;

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

void hero_start_jump(game_s *g, obj_s *o, i32 ID)
{
    hero_s *h = &g->hero_mem;

    i32 jID = ID;
    if (ID == HERO_JUMP_GROUND && HERO_VX_SPRINT <= abs_i32(o->v_q8.x)) {
        jID = HERO_JUMP_GROUND_BOOSTED;
    }

    hero_jumpvar_s jv   = g_herovar[jID];
    h->jump_index       = jID;
    h->edgeticks        = 0;
    h->jump_btn_buffer  = 0;
    o->v_q8.y           = -jv.vy;
    h->jumpticks        = jv.ticks;
    h->jump_fly_snd_iID = 0;
    if (ID == HERO_JUMP_GROUND) {
        snd_play(SNDID_SPEAK, 1.f, 0.5f);
        v2_i32 posc = obj_pos_bottom_center(o);
        posc.x -= 16;
        posc.y -= 32;
        rec_i32 trp = {0, 284, 32, 32};
        spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                           trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
    }
    if (ID == HERO_JUMP_FLY) {
        snd_play(SNDID_WING1, 2.f, rngr_f32(0.8f, 1.2f));
    }
}

void hero_on_update(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    hero_handle_input(g, o);

    const v2_i16 v_og   = o->v_q8;
    i32          state  = hero_determine_state(g, o, h);
    i32          dpad_x = inp_x();
    i32          dpad_y = inp_y();
    if (state == HERO_STATE_DEAD) {
        dpad_x = 0;
        dpad_y = 0;
    }

    o->flags |= OBJ_FLAG_MOVER;
    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;

    o->grav_q8.y    = HERO_GRAVITY;
    h->crawlingp    = h->crawling;
    h->crawling     = 0;
    h->grabbingp    = h->grabbing;
    h->grabbing     = 0;
    h->climbingp    = h->climbing;
    h->climbing     = 0;
    h->trys_lifting = 0;

    if (h->invincibility_ticks) {
        h->invincibility_ticks--;
    }

    bool32 sliding = (0 < dpad_y &&
                      h->sliding &&
                      (state == HERO_STATE_AIR || state == HERO_STATE_GROUND));

    if (sliding) {

    } else {
        h->sliding = 0;
    }

    if (h->jump_ui_may_hide) {
        h->jump_ui_fade_out = max_i32((i32)h->jump_ui_fade_out - 1, 0);
    } else {
        h->jump_ui_fade_out = min_i32((i32)h->jump_ui_fade_out + 3, JUMP_UI_TICKS_HIDE);
    }

    if (h->jump_ui_collected_tick) {
        h->jump_ui_collected_tick--;
    }

#if 0
    if (h->climbingp) {
        if (hero_is_climbing(g, o, o->facing)) {
            h->climbing = 1;
            state       = HERO_STATE_CLIMB;
        }
    } else if (state == HERO_STATE_AIR) {
        if (hero_is_climbing(g, o, dpad_x)) {
            o->facing   = dpad_x;
            h->climbing = 1;
            state       = HERO_STATE_CLIMB;
        }
    }

    if (state == HERO_STATE_CLIMB) {
        h->climbing        = 1;
        h->jumpticks       = 0;
        h->jump_btn_buffer = 0;
        h->b_hold_tick     = 0;
        h->attack_tick     = 0;
        h->crawling        = 0;
        o->gravity_q8.y    = 0;
        o->vel_q8.x        = 0;
        o->vel_q8.y        = 0;
    }
#endif

    if (state == HERO_STATE_AIR) {
        hero_flytime_update_ui(g, o, !h->jump_ui_collected_tick);
    } else {
        h->jump_ui_collected_tick = 0;
        hero_flytime_update_ui(g, o, g->save.flyupgrades << 1);
    }

#ifdef PLTF_DEBUG
    assert(0 <= hero_flytime_left(g, o) &&
           hero_flytime_left(g, o) <= hero_flytime_max(g, o));
#endif

    if (h->attack_tick) {
        h->attack_tick++;
        if (HERO_ATTACK_TICKS <= h->attack_tick) {
            h->attack_tick = 0;
        }
    }

    if (h->jump_btn_buffer) {
        h->jump_btn_buffer--;
    }
    if (h->ground_impact_ticks) {
        h->ground_impact_ticks--;
    }

    if (state != HERO_STATE_GROUND) {
        h->skidding = 0;
    }

    if (h->skidding) {
        h->skidding--;
    }

    if ((state != HERO_STATE_DEAD) && (state != HERO_STATE_SWIMMING || !h->diving)) {
        i32 dt          = hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 100 : 5;
        h->breath_ticks = max_i32(h->breath_ticks - dt, 0);
    }

    if (state != HERO_STATE_AIR && state != HERO_STATE_GROUND) {
        h->edgeticks = 0;
    }

    if (state != HERO_STATE_AIR) {
        h->low_grav_ticks = 0;
    }

    bool32 facing_locked = 0;
    facing_locked |= h->grabbingp;
    facing_locked |= h->trys_lifting;
    facing_locked |= h->attack_tick;
    facing_locked |= h->crawlingp;
    facing_locked |= h->climbing;
    facing_locked |= h->skidding;

    if (!facing_locked) {
        if (h->sliding) {
            o->facing = o->v_q8.x ? sgn_i(o->v_q8.x) : o->facing;
        } else if (dpad_x) {
            o->facing = dpad_x;
        }
    }

    // hero_item_usage(g, o, state);
    rope_s *r              = o->rope;
    bool32  rope_stretched = (r && (r->len_max_q4 * 254) <= (rope_len_q4(g, r) << 8));

    if ((o->bumpflags & OBJ_BUMPED_Y) && !(o->bumpflags & OBJ_BUMPED_Y_BOUNDS)) {
        if (o->bumpflags & OBJ_BUMPED_Y_NEG) {
            h->jumpticks = 0;
        }

        if (state == HERO_STATE_AIR && rope_stretched) {
            o->v_q8.y = -(o->v_q8.y >> 2);
        } else {
            if (1000 <= o->v_q8.y) {
                f32 vol = (1.f * (f32)o->v_q8.y) / 2000.f;
                snd_play(SNDID_STEP, min_f(vol, 1.f) * 1.2f, 1.f);
            }

            if (500 <= o->v_q8.y) {
                h->ground_impact_ticks = min_i32(o->v_q8.y >> 9, 8);
                v2_i32 posc            = obj_pos_bottom_center(o);
                posc.x -= 16;
                posc.y -= 32;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                                   trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            o->v_q8.y = 0;
        }
    }

    if (o->bumpflags & OBJ_BUMPED_ON_HEAD) {
        o->v_q8.y = -1000;
    }

    if (o->bumpflags & OBJ_BUMPED_X) {
        if (h->sliding) {
            o->v_q8.x = -(o->v_q8.x >> 2);
        } else if (state == HERO_STATE_AIR && rope_stretched) {
            if (dpad_x == sgn_i(o->v_q8.x) || abs_i(o->v_q8.x) < 600) {
                o->v_q8.x = 0;
            } else {
                o->v_q8.x = -(o->v_q8.x >> 2);
            }
        } else {
            o->v_q8.x = 0;
        }
    }
    o->bumpflags = 0;

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_AIR: {
        if (!inp_action_jp(INP_DU) && !inp_action_jp(INP_DD)) break;
        if (hero_try_snap_to_ladder(g, o, dpad_y)) {
            return; // RETURN
        }
        break;
    }
    }

    switch (state) {
    case HERO_STATE_GROUND:
        hero_update_ground(g, o);
        break;
    case HERO_STATE_AIR:
        hero_update_air(g, o, rope_stretched);
        break;
    case HERO_STATE_SWIMMING:
        hero_update_swimming(g, o);
        break;
    case HERO_STATE_LADDER:
        hero_update_ladder(g, o);
        break;
    case HERO_STATE_CLIMB:
        hero_update_climb(g, o);
        break;
    case HERO_STATE_DEAD: {
        i32    wdepth   = water_depth_rec(g, obj_aabb(o));
        bool32 grounded = obj_grounded(g, o);
        bool32 inwater  = (wdepth && !grounded);

        if (inwater) {
            o->grav_q8.y = 0;
        }
        break;
    }
    }

    if (h->crawlingp && !h->crawling && o->h == HERO_HEIGHT_CRAWL) {
        if (!hero_stand_up(g, o)) {
            h->crawling = 1;
        }
    }
}

bool32 hero_stand_up(game_s *g, obj_s *o)
{
    assert(o->h == HERO_HEIGHT_CRAWL);
    assert(!o->rope);
    rec_i32 aabbcur    = obj_aabb(o);
    rec_i32 aabb_stand = aabbcur;
    i32     hdt        = HERO_HEIGHT - HERO_HEIGHT_CRAWL;
    aabb_stand.y -= hdt;
    aabb_stand.h = HERO_HEIGHT;
    if (!map_traversable(g, aabb_stand))
        return 0;
    v2_i32 dt = {0, -hdt};
    obj_move(g, o, dt);
    o->h = HERO_HEIGHT;
    return 1;
}

void hero_flytime_update_ui(game_s *g, obj_s *ohero, i32 amount)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    if (!h->flytime_added) return;

    i32 d = min_i32(h->flytime_added, amount);
    h->flytime_added -= d;
    h->flytime += d;
}

void hero_flytime_modify(game_s *g, obj_s *ohero, i32 dt)
{
    hero_s *h  = (hero_s *)&g->hero_mem;
    i32     ft = hero_flytime_left(g, ohero);

    if (0 < dt) { // add flytime
        i32 d = min_i32(dt, hero_flytime_max(g, ohero) - ft);
        h->flytime += d;
    } else if (dt < 0) { // remove flytime
        i32 d = -dt;
        if (h->flytime) {
            i32 x = min_i32(h->flytime, d);
            h->flytime -= x;
            d -= x;
        }
        h->flytime_added = max_i32(0, h->flytime_added - d);
    }
    if (hero_flytime_left(g, ohero) < ft) {
        h->jump_ui_may_hide = 0;
    }
}

void hero_flytime_add_ui(game_s *g, obj_s *ohero, i32 dt)
{
    hero_s *h  = (hero_s *)&g->hero_mem;
    i32     ft = hero_flytime_left(g, ohero);
    h->flytime_added += min_i32(dt, hero_flytime_max(g, ohero) - ft);
    h->jump_ui_collected_tick = 20;
}

i32 hero_flytime_left(game_s *g, obj_s *ohero)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    return (h->flytime + h->flytime_added);
}

i32 hero_flytime_ui_full(game_s *g, obj_s *ohero)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    return h->flytime;
}

i32 hero_flytime_ui_added(game_s *g, obj_s *ohero)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    return h->flytime_added;
}

i32 hero_flytime_max(game_s *g, obj_s *ohero)
{
    return g->save.flyupgrades * HERO_TICKS_PER_JUMP_UPGRADE;
}

bool32 hero_present_and_alive(game_s *g, obj_s **ohero)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o) return 0;
    if (ohero) {
        *ohero = o;
    }
    return (o->health);
}

void hero_momentum_change(game_s *g, obj_s *o, i32 dt)
{
    hero_s *h   = (hero_s *)&g->hero_mem;
    h->momentum = clamp_i32(h->momentum + dt, 0, HERO_MOMENTUM_MAX);
}

void hero_action_throw_grapple(game_s *g, obj_s *o)
{
    i32 dirx = inp_x();
    i32 diry = inp_y();

    if (dirx == 0 && diry == 0) return;
    // throw new hook

    hero_s *h = &g->hero_mem;
    snd_play(SNDID_HOOK_THROW, 1.f, 1.f);

    v2_i32 center = obj_pos_center(o);
    center.y -= 8;

    if (dirx && diry == 0) {
        diry = -1;
    }

    v2_i32 vlaunch = {dirx * 3000, diry * 2300};

    if (diry < 0) {
        vlaunch.y = (vlaunch.y * 5) / 4;
    } else if (diry == 0) {
        vlaunch.y = -600;
        vlaunch.x = (vlaunch.x * 5) / 4;
    }

    g->rope.active = 1;
    rope_s *rope   = &g->rope;
    obj_s  *hook   = hook_create(g, rope, center, vlaunch);
    h->hook        = obj_handle_from_obj(hook);
    o->rope        = rope;
    o->ropenode    = rope->head;

    v2_i32 pcurr = v2_shl(center, 8);

    for (i32 n = 0; n < ROPE_VERLET_N; n++) {
        rope->ropept[n].p    = pcurr;
        // "hint" the direction to the verlet sim
        i32 k                = ROPE_VERLET_N - 1 - n;
        rope->ropept[n].pp.x = pcurr.x - ((dirx << 12) * k) / ROPE_VERLET_N;
        rope->ropept[n].pp.y = pcurr.y - ((diry << 12) * k) / ROPE_VERLET_N;
    }
}

bool32 hero_action_ungrapple(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    if (!obj_handle_valid(h->hook)) return 0;

    obj_s *ohook;
    if (!obj_try_from_obj_handle(h->hook, &ohook)) return 0;
    bool32 attached = hook_is_attached(ohook);
    hook_destroy(g, o, ohook);

    if (obj_grounded(g, o) || !attached) return 1;

    i32 mulx = 280;
    i32 muly = 320;

    if (abs_i32(o->v_q8.x) < 600) {
        mulx = 300;
    }

    if (abs_i32(o->v_q8.y) < 600) {
        muly = 350;
    }

    i32 addx = sgn_i32(o->v_q8.x) * 200;
    i32 addy = 0;
    if (o->v_q8.y < 0) {
        addy -= 200;
    }

    o->v_q8.x = ((o->v_q8.x * mulx) >> 8) + addx;
    o->v_q8.y = ((o->v_q8.y * muly) >> 8) + addy;

#define LOW_G_TICKS_VEL 4000
    i32 low_g_v         = min_i32(abs_i32(o->v_q8.y), LOW_G_TICKS_VEL);
    h->low_grav_ticks_0 = HERO_LOW_GRAV_TICKS;
    h->low_grav_ticks   = lerp_i32(0, HERO_LOW_GRAV_TICKS, low_g_v, LOW_G_TICKS_VEL);

    return 1;
}

void hero_action_jump(game_s *g, obj_s *o)
{
}

void hero_action_attack(game_s *g, obj_s *o)
{
    hero_s *h      = (hero_s *)&g->hero_mem;
    h->attack_tick = 1;

    if (obj_grounded(g, o)) {
        o->v_q8.x = 0;
    }

    hitbox_s hb   = {0};
    hb.damage     = 1;
    hb.r.h        = 32;
    hb.r.w        = 48;
    hb.r.y        = o->pos.y - 5;
    hb.force_q8.x = o->facing * 800;
    hb.force_q8.y = -400;
    if (o->facing == 1) {
        hb.r.x = o->pos.x + o->w;
    } else {
        hb.r.x = o->pos.x - hb.r.w;
    }

    bool32 did_hit = obj_game_player_attackbox(g, hb);
    snd_play(SNDID_OWLET_ATTACK_1, 1.3f, rngr_f32(0.8f, 1.2f));

    // slash sprite
    rec_i32 rslash = {0, 1024 + 64, 64, 64};
    v2_i32  dcpos  = {-10, -40};
    i32     flip   = 0;
    if (o->facing < 0) {
        flip    = SPR_FLIP_X;
        dcpos.x = -40;
    }
    if (h->attack_flipflop) {
        rslash.x += 512;
    }

    spritedecal_create(g, 0xFFFF, o, dcpos, TEXID_HERO, rslash, 18, 8, flip);
}