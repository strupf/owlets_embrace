// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_air(g_s *g, obj_s *o, bool32 rope_stretched)
{
    hero_s *h         = (hero_s *)o->heap;
    i32     dpad_x    = inp_x();
    i32     dpad_y    = inp_y();
    bool32  usinghook = o->rope != NULL;
    o->animation++;
    if (h->edgeticks) {
        h->edgeticks--;
    }
    if (h->walljump_tick) {
        h->walljump_tick -= sgn_i32(h->walljump_tick);
    }

    if (h->low_grav_ticks) {
        h->low_grav_ticks--;
        i32 gt0 = pow2_i32(h->low_grav_ticks_0);
        i32 gt1 = pow2_i32(h->low_grav_ticks - h->low_grav_ticks_0);
        o->v_q8.y += lerp_i32(HERO_GRAVITY_LOW, HERO_GRAVITY, gt1, gt0);
    } else {
        o->v_q8.y += HERO_GRAVITY;
    }

    i32    staminap  = hero_stamina_left(g, o);
    bool32 can_stomp = hero_has_upgrade(g, HERO_UPGRADE_STOMP) &&
                       !h->holds_weapon &&
                       !usinghook;

    if (rope_stretched) {
        v2_i32 rn_curr  = o->ropenode->p;
        v2_i32 rn_next  = ropenode_neighbour(o->rope, o->ropenode)->p;
        v2_i32 dtrope   = v2_sub(rn_next, rn_curr);
        i32    dtrope_s = sgn_i32(dtrope.x);
        i32    dtrope_a = abs_i32(dtrope.x);

        if (sgn_i32(o->v_q8.x) != dpad_x) {
            obj_vx_q8_mul(o, 254);
        }

        o->v_q8.x += ((dtrope_s == dpad_x) ? 40 : 5) * dpad_x;
    } else if (h->stomp) {
        o->flags &= ~OBJ_FLAG_MOVER;
        if (h->stomp < U8_MAX) {
            h->stomp++;
        }
        if (HERO_TICKS_STOMP_INIT <= h->stomp) {
            obj_move(g, o,
                     dpad_x,
                     min_i32((h->stomp - HERO_TICKS_STOMP_INIT) * 1, 10));
        }
    } else if (can_stomp && inp_action_jp(INP_DD)) {
        h->stomp     = 1;
        h->jumpticks = 0;
        o->v_q8.x    = 0;
        o->v_q8.y    = 0;
        snd_play(SNDID_WING, 1.f, 1.f);
    } else {
        i32 jumpticksp = h->jumpticks;

        // dynamic jump height
        if (0 < h->jumpticks && !h->action_jump) {
            obj_vy_q8_mul(o, 128); // short hop
            h->jumpticks = 0;
            h->walljump_tick >>= 1;
        }

        if (staminap <= 0 && h->jump_index == HERO_JUMP_FLY) {
            h->jumpticks = 0;
        }

        if (0 < h->jumpticks) {
            hero_jumpvar_s jv = g_herovar[h->jump_index];
            i32            t0 = pow_i32(jv.ticks, 4);
            i32            ti = pow_i32(h->jumpticks, 4) - t0;
            i32            ch = jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
            o->v_q8.y -= ch;
            if (h->jump_index == HERO_JUMP_FLY) {
                hero_stamina_modify(g, o, -64);
            }
            if (h->jump_index == HERO_JUMP_FLY && h->jumpticks == jv.ticks - 8) {
                snd_play(SNDID_WING1, 1.8f, 0.5f);
            }
            h->jumpticks--;
        }

        i32 vs = sgn_i32(o->v_q8.x);
        i32 va = abs_i32(o->v_q8.x);
        i32 ax = 0;

        if (dpad_x != vs) {
            obj_vx_q8_mul(o, 246);
        }

        if (vs == 0) {
            ax = 200;
        } else if (dpad_x == +vs && va < HERO_VX_WALK) { // press same dir as velocity
            ax = lerp_i32(200, 20, va, HERO_VX_WALK);
            ax = min_i32(ax, HERO_VX_WALK - va);
        } else if (dpad_x == -vs) {
            ax = min_i32(100, va);
        }

        if (h->walljump_tick) {
            ax = lerp_i32(ax, 0, abs_i32(h->walljump_tick), WALLJUMP_MOM_TICKS);
        }
        o->v_q8.x += ax * dpad_x;
    }

    o->v_q8.y = min_i32(o->v_q8.y, 1792);

    bool32 start_climbing = 0;
    if (hero_has_upgrade(g, HERO_UPGRADE_CLIMB) && dpad_x) {
        if (hero_is_climbing(g, o, dpad_x)) {
            start_climbing  = 1;
            o->animation    = 0;
            h->impact_ticks = 5;
        }
    }

    if (start_climbing) {
        if (inp_action_jp(INP_A)) {
            hero_walljump(g, o, -dpad_x);
        } else {
            o->v_q8.y       = 0;
            o->v_q8.x       = 0;
            o->facing       = dpad_x;
            h->impact_ticks = 4;
            h->climbing     = 1;
        }
    } else if (0 < h->jump_btn_buffer) {
        bool32 jump_ground = 0 < h->edgeticks;
        bool32 jump_midair = !usinghook &&   // not hooked
                             !jump_ground && // jump in air?
                             !h->holds_weapon &&
                             !h->jumpticks &&
                             !h->stomp &&
                             hero_has_upgrade(g, HERO_UPGRADE_FLY) &&
                             hero_stamina_left(g, o);
        bool32 jump_wall = 0;
        for (i32 n = 0; n < 4; n++) {
            if (hero_is_climbing_offs(g, o, -dpad_x, -dpad_x * n, 0)) {
                jump_wall = 1;
                break;
            }
        }

        if (jump_midair && !jump_ground && !jump_wall) { // just above ground -> ground jump
            for (i32 y = 0; y < 6; y++) {
                rec_i32 rr = {o->pos.x, o->pos.y + o->h + y, o->w, 1};
                v2_i32  pp = {0, y + 1};
                if (map_traversable(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                    jump_ground = 1;
                    break;
                }
            }
        }

        if (jump_ground || jump_midair || jump_wall) {
            h->jump_btn_buffer = 0;
        }

        if (jump_ground) {
            hero_restore_grounded_stuff(g, o);
            hero_start_jump(g, o, HERO_JUMP_GROUND);
        } else if (jump_wall) {
            hero_walljump(g, o, dpad_x);
        } else if (jump_midair) {
            hero_stamina_modify(g, o, -192);
            hero_start_jump(g, o, HERO_JUMP_FLY);

            rec_i32 rwind = {0, 0, 64, 64};
            v2_i32  dcpos = obj_pos_center(o);
            dcpos.x -= 32;
            dcpos.y += 0;
            i32 flip = rngr_i32(0, 1) ? 0 : SPR_FLIP_X;

            spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, dcpos, TEXID_WINDGUSH, rwind, 18, 6, flip);
        }
    }
}
