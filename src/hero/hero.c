// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"
#include "hero_hook.h"

const hero_jumpvar_s g_herovar[6] = {
    {825, 25, 70, 30},   // out of water
    {1150, 40, 100, 50}, // ground
    {300, 25, 160, 40},  // airj 1
    {250, 25, 150, 40},  // airj 2
    {100, 25, 130, 0},   // airj 3
    {1000, 25, 80, 30}}; // wall jum

void hero_on_update(game_s *g, obj_s *o);
void hero_start_jump(game_s *g, obj_s *o, i32 ID);
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
               OBJ_FLAG_TILE_COLLISION |
               OBJ_FLAG_ACTOR |
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

bool32 hero_unhook(game_s *g, obj_s *o)
{
    if (!obj_handle_valid(o->obj_handles[0])) return 0;

    hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
    i32 mulx = 300;
    i32 muly = 400;

    if (abs_i32(o->vel_q8.x) < 600) {
        mulx = 400;
    }

    if (abs_i32(o->vel_q8.y) < 600) {
        muly = 450;
    }

    i32 addx = sgn_i32(o->vel_q8.x) * 200;
    i32 addy = 0;
    if (o->vel_q8.y < 0) {
        addy -= 350;
    }

    o->vel_q8.x = ((o->vel_q8.x * mulx) >> 8) + addx;
    o->vel_q8.y = ((o->vel_q8.y * muly) >> 8) + addy;

    if (!obj_grounded(g, o)) { // low gravity mode
        hero_s *h = &g->hero_mem;
#define LOW_G_TICKS_VEL 2000
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
}

void hero_use_hook(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    snd_play_ext(SNDID_HOOK_THROW, 1.f, 1.f);
    // throw new hook
    i32 dirx = inp_x();
    i32 diry = inp_y();

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

void hero_item_usage(game_s *g, obj_s *o, i32 state)
{
    hero_s *h      = &g->hero_mem;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();

    if (state == HERO_STATE_SWIMMING) {
        h->attack_tick = 0;
        hero_unhook(g, o);
    }

    if (state == HERO_STATE_LADDER || state == HERO_STATE_SWIMMING) return;

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

    g->item_select.n_items = 2;

    switch (g->item_select.item) {
    case HERO_ITEM_HOOK: {
        assert(hero_has_upgrade(g, HERO_UPGRADE_HOOK));
        if (inp_action_jp(INP_B)) {
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

        if (inp_action_jp(INP_DU) && o->rope &&
            HERO_ROPE_LEN_MIN < rlen) {
            i32 time = gameplay_time(g);
            if ((time - h->reel_in_dtap) <= 10) {
                h->reel_in = 1;
            }
            h->reel_in_dtap = time;
            break;
        }

        if (o->rope) {
            i32 dt_crank = inp_crank_dt_q16() >> 4;
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
            if (inp_action_jr(INP_B)) {
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
            if (inp_action_jp(INP_B)) {
                h->attack_hold_tick = 1;
            }
        }
        break;
    }
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

    if (!inp_action(INP_A) || state != HERO_STATE_AIR) {
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

    if (state != HERO_STATE_AIR) {
        h->low_grav_ticks = 0;
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

    if (hero_has_upgrade(g, HERO_UPGRADE_WALLJUMP)) {
        // wall run tick
        bool32 wallr       = !map_traversable(g, obj_rec_right(o));
        bool32 walll       = !map_traversable(g, obj_rec_left(o));
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

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook) {
        hook_update(g, o, ohook);
    }
}
