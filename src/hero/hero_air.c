// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_air(game_s *g, obj_s *o, bool32 rope_stretched)
{
    hero_s *h         = &g->hero_mem;
    i32     dpad_x    = inp_x();
    i32     dpad_y    = inp_y();
    bool32  usinghook = o->rope != NULL;
    o->animation++;
    h->edgeticks--;
    if (0 < h->low_grav_ticks) {
        h->low_grav_ticks--;
        i32 gt0         = pow2_i32(h->low_grav_ticks_0);
        i32 gt1         = pow2_i32(h->low_grav_ticks - h->low_grav_ticks_0);
        o->gravity_q8.y = lerp_i32(HERO_GRAVITY_LOW, HERO_GRAVITY, gt1, gt0);
    }

    rec_i32 rec_l = obj_rec_left(o);
    rec_i32 rec_r = obj_rec_right(o);
    rec_l.h -= 8;
    rec_r.h -= 8;
    rec_l.y += 4;
    rec_r.y += 4;

#if 0
    bool32 touch_l = !map_traversable(g, rec_l);
    bool32 touch_r = !map_traversable(g, rec_r);
    bool32 touch_t = !map_traversable(g, obj_rec_top(o));
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

        if (inp_action(INP_A) && 0 < h->jumpticks) {
            h->jumpticks--;
            i32 vadd = lerp_i32(20, 50, h->jumpticks, HERO_ROPEWALLJUMP_TICKS);
            o->vel_q8.x += h->ropewalljump_dir * vadd;
        } else {
            h->jumpticks = 0;
        }

        if (inp_action_jp(INP_A)) {
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
        if (0 < h->jumpticks && !inp_action(INP_A)) {
            h->jumpticks = 0;
        }

        if (0 < h->jumpticks) {

            hero_jumpvar_s jv    = g_herovar[h->jump_index];
            i32            ticki = jv.ticks - h->jumpticks;
            i32            tick0 = jv.ticks;
            i32            t0    = pow_i32(jv.ticks, 4);
            i32            ti    = pow_i32(h->jumpticks, 4) - t0;
            i32            va    = jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
            o->vel_q8.y -= va;
        }

        if (inp_action_jp(INP_A)) {
            if (hero_has_upgrade(g, HERO_UPGRADE_GLIDE) && inp_y() < 0) {
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
        if (inp_action_jp(INP_DD)) {
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
                if (map_traversable(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                    jump_ground = 1;
                    break;
                }
            }
        }

        if (jump_ground) {
            hero_restore_grounded_stuff(g, o);
            hero_start_jump(g, o, HERO_JUMP_GROUND);
        } else if (jump_wall && inp_x() == jump_wall) {
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
