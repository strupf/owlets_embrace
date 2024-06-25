// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "app.h"
#include "game.h"
#include "hero_hook.h"

const hero_jumpvar_s g_herovar[NUM_HERO_JUMP] = {
    {800, 30, 100, 30},  // out of water
    {1200, 40, 70, 25},  // ground
    {600, 50, 140, 0},   // fly
    {1000, 25, 80, 30}}; // wall jump

void hero_on_update(game_s *g, obj_s *o);
void hero_item_usage(game_s *g, obj_s *o, i32 state);
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
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_GLUE_GROUND |
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

    if (!hero_has_upgrade(g, HERO_UPGRADE_WALLJUMP)) return 0;

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
    if (hero_has_upgrade(g, HERO_UPGRADE_HOOK_LONG))
        return HERO_ROPE_LEN_LONG;
    return HERO_ROPE_LEN_SHORT;
}

void hero_on_squish(game_s *g, obj_s *o)
{
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
    if (g->substate != SUBSTATE_GAMEOVER) {
        gameover_start(g);
    }
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

bool32 hero_unhook(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    if (!obj_handle_valid(h->hook)) return 0;

    hook_destroy(g, o, obj_from_obj_handle(h->hook));
    i32 mulx = 280;
    i32 muly = 320;

    if (abs_i32(o->vel_q8.x) < 600) {
        mulx = 300;
    }

    if (abs_i32(o->vel_q8.y) < 600) {
        muly = 350;
    }

    i32 addx = sgn_i32(o->vel_q8.x) * 200;
    i32 addy = 0;
    if (o->vel_q8.y < 0) {
        addy -= 200;
    }

    o->vel_q8.x = ((o->vel_q8.x * mulx) >> 8) + addx;
    o->vel_q8.y = ((o->vel_q8.y * muly) >> 8) + addy;

    if (!obj_grounded(g, o)) { // low gravity mode
        hero_s *h = &g->hero_mem;
#define LOW_G_TICKS_VEL 4000
        i32 low_g_v         = min_i32(abs_i32(o->vel_q8.y), LOW_G_TICKS_VEL);
        h->low_grav_ticks_0 = HERO_LOW_GRAV_TICKS;
        h->low_grav_ticks   = lerp_i32(0, HERO_LOW_GRAV_TICKS, low_g_v, LOW_G_TICKS_VEL);
    }

    return 1;
}

void hero_start_jump(game_s *g, obj_s *o, i32 ID)
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
    if (ID == HERO_JUMP_FLY) {
        h->show_jump_ui = 1;
    }
}

void hero_use_hook(game_s *g, obj_s *o)
{
    i32 dirx = inp_x();
    i32 diry = inp_y();

    if (dirx == 0 && diry == 0) return;
    // throw new hook

    hero_s *h = &g->hero_mem;
    snd_play_ext(SNDID_HOOK_THROW, 1.f, 1.f);

    v2_i32 center  = obj_pos_center(o);
    v2_i32 vlaunch = {dirx * 3000, diry * 2300};

    if (diry < 0) {
        vlaunch.y = (vlaunch.y * 5) / 4;
    } else if (diry == 0) {
        vlaunch.y = -600;
        vlaunch.x = (vlaunch.x * 5) / 4;
    }

    g->rope.active = 1;
    h->reel_in     = 0;
    rope_s *rope   = &g->rope;
    obj_s  *hook   = hook_create(g, rope, center, vlaunch);
    h->hook        = obj_handle_from_obj(hook);
    o->rope        = rope;
    o->ropenode    = rope->head;

    v2_i32 pcurr = v2_shl(center, 8);
    // "hint" the direction to the verlet sim
    v2_i32 pprev = {pcurr.x - (dirx << 9), pcurr.y - (diry << 9)};
    for (i32 n = 0; n < ROPE_VERLET_N; n++) {
        rope->ropept[n].p  = pcurr;
        rope->ropept[n].pp = pprev;
    }
}

void hero_item_usage(game_s *g, obj_s *o, i32 state)
{
    hero_s *h      = &g->hero_mem;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();

    if (state == HERO_STATE_SWIMMING) {
        h->attack_tick = 0;
        hero_unhook(g, o);
        return;
    }

    if (state == HERO_STATE_LADDER) return;

    bool32 grab_state = !h->carrying &&
                        state == HERO_STATE_GROUND &&
                        !o->rope &&
                        inp_action(INP_B);
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

                if (0 < dpad_y) {
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
        obj_s *ocarry = carryable_present(g);
        if (ocarry && inp_action_jp(INP_B)) {
            carryable_on_drop(g, ocarry);
            h->carrying      = 0;
            ocarry->vel_q8.y = -1000;
            ocarry->vel_q8.x = o->facing * 1000;
        }
        return;
    }

    h->idle_anim  = 0;
    h->idle_ticks = 0;

#define HOOK_HOLD_TICK 12

    if (h->b_hold_tick) {
        if (inp_action_jr(INP_B)) {
            if (HOOK_HOLD_TICK <= h->b_hold_tick) {
                h->attack_tick = 0;
                if (inp_x() | inp_y()) {
                    hero_use_hook(g, o);
                    obj_s *ohook = obj_from_obj_handle(h->hook);
                }
            } else if (!h->item_only_hook) {
                h->attack_tick = 1;
                hitbox_s hb    = {0};
                hb.damage      = 1;
                hb.r.h         = 32;
                hb.r.w         = 48;
                hb.r.y         = o->pos.y - 5;
                hb.force_q8.x  = o->facing * 800;
                hb.force_q8.y  = -400;
                if (o->facing == 1) {
                    hb.r.x = o->pos.x + o->w;
                } else {
                    hb.r.x = o->pos.x - hb.r.w;
                }

                bool32 hit       = obj_game_player_attackbox(g, hb);
                i32    knockback = obj_grounded(g, o) ? 3000 : 500;
                o->vel_q8.x      = -hit * o->facing * knockback;

                rec_i32 rslash = {0, 1024, 64, 64};
                v2_i32  dcpos  = {-10, -40};
                i32     flip   = 0;
                if (o->facing < 0) {
                    flip    = SPR_FLIP_X;
                    dcpos.x = -40;
                }
                if (h->attack_flipflop) {
                    rslash.x += 512;
                }

                // slash sprite
                spritedecal_create(g, 0xFFFF, o, dcpos, TEXID_HERO, rslash, 20, 8, flip);
            }
            h->hook_cancel_tick = 0;
            h->attack_hold_tick = 0;
            h->b_hold_tick      = 0;
            h->item_only_hook   = 0;
        } else {
            h->b_hold_tick++;
        }
    } else if (inp_action_jp(INP_B)) {
        if (o->rope) {
            hero_unhook(g, o);
            h->item_only_hook = 1;
        } else if (h->attack_tick == 0 || 6 <= h->attack_tick) {
            h->attack_flipflop = 1 - h->attack_flipflop;
            h->item_only_hook  = 0;
            if (0 < h->attack_tick) {
                if (h->attack_tick <= 12) {
                    h->attack_hold_tick = 10;
                } else {
                    h->attack_hold_tick = 6;
                }
            } else {
                h->attack_hold_tick = 1;
            }
        }
        h->b_hold_tick = 1;
    }
}

bool32 hero_is_sprinting(hero_s *h)
{
    return (h->sprint_ticks);
}

void hero_on_update(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;

    const v2_i32 v_og   = o->vel_q8;
    const i32    state  = hero_determine_state(g, o, h);
    i32          dpad_x = inp_x();
    i32          dpad_y = inp_y();
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

    o->gravity_q8.y = HERO_GRAVITY;
    h->crawlingp    = h->crawling;
    h->crawling     = 0;
    h->grabbingp    = h->grabbing;
    h->grabbing     = 0;
    h->trys_lifting = 0;
    bool32 sliding  = (0 < dpad_y &&
                      h->sliding &&
                      (state == HERO_STATE_AIR || state == HERO_STATE_GROUND));
    if (sliding) {
        o->drag_q8.x = HERO_DRAG_SLIDING;
    } else {
        h->sliding = 0;
    }

    hero_flytime_update_ui(g, o, 1);
#ifdef PLTF_DEBUG
    assert(0 <= hero_flytime_left(g, o) &&
           hero_flytime_left(g, o) <= g->save.flytime);
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

    if ((state != HERO_STATE_DEAD) && (state != HERO_STATE_SWIMMING || !h->diving)) {
        h->breath_ticks -= hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 100 : 5;
        h->breath_ticks = max_i(h->breath_ticks, 0);
    }

    if (state != HERO_STATE_AIR && state != HERO_STATE_GROUND) {
        h->sprint_ticks = 0;
        h->edgeticks    = 0;
    }

    if (state != HERO_STATE_AIR) {
        h->low_grav_ticks = 0;
    }

    bool32 facing_locked = 0;
    facing_locked |= h->grabbingp;
    facing_locked |= h->trys_lifting;
    facing_locked |= h->attack_tick;
    facing_locked |= h->crawlingp;

    if (!facing_locked) {
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
    rope_s *r              = o->rope;
    bool32  rope_stretched = (r && (r->len_max_q4 * 254) <= (rope_len_q4(g, r) << 8));

    if ((o->bumpflags & OBJ_BUMPED_Y) && !(o->bumpflags & OBJ_BUMPED_Y_BOUNDS)) {
        if (o->bumpflags & OBJ_BUMPED_Y_NEG) {
            h->jumpticks = 0;
        }

        if (state == HERO_STATE_AIR && rope_stretched) {
            o->vel_q8.y = -(o->vel_q8.y >> 2);
        } else {
            if (1000 <= o->vel_q8.y) {
                f32 vol = (1.f * (f32)o->vel_q8.y) / 2000.f;
                snd_play_ext(SNDID_STEP, min_f(vol, 1.f) * 1.2f, 1.f);
            }

            if (1000 <= o->vel_q8.y && 0 < inp_y()) {
                h->jump_boost_tick = 1;
            }

            if (500 <= o->vel_q8.y) {
                h->ground_impact_ticks = min_i32(o->vel_q8.y >> 9, 8);
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
        i32 d = min_i32(dt, g->save.flytime - ft);
        h->flytime += d;
    } else if (dt < 0) { // remove flytime
        i32 d = -dt;
        if (h->flytime_added) {
            i32 x = min_i32(h->flytime_added, d);
            h->flytime_added -= x;
            d -= x;
        }
        h->flytime = max_i32(0, h->flytime - d);
    }
}

void hero_flytime_add_ui(game_s *g, obj_s *ohero, i32 dt)
{
    hero_s *h        = (hero_s *)&g->hero_mem;
    h->flytime_added = min_i32(dt, g->save.flytime - h->flytime);
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
