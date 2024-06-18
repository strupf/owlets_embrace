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
    if (h->low_grav_ticks) {
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
    const i32 flytimep = h->flytime;

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
        i32    dtrope_s = sgn_i32(dtrope.x);
        i32    dtrope_a = abs_i32(dtrope.x);

        if (dpad_x) {
            if (dtrope_s == dpad_x) {
                o->vel_q8.x += 60 * dpad_x;
            } else {
                o->vel_q8.x += 15 * dpad_x;
            }
        }
    } else {
        o->drag_q8.x = h->sliding ? 253 : 240;

        // dynamic jump height
        if (0 < h->jumpticks && !inp_action(INP_A)) {
            h->jumpticks = 0;
        }

        if (h->flytime <= 0 && h->jump_index != HERO_JUMP_WATER) {
            h->jumpticks = 0;
        }

        if (0 < h->jumpticks) {
            hero_jumpvar_s jv = g_herovar[h->jump_index];
            i32            t0 = pow_i32(jv.ticks, 4);
            i32            ti = pow_i32(h->jumpticks, 4) - t0;
            o->vel_q8.y -= jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
            h->flytime -= (h->jump_index == HERO_JUMP_FLY);
        }
        h->jumpticks--;

        if (inp_action_jp(INP_A)) {
            h->jump_btn_buffer = 12;
        }

        if (dpad_x) {
            i32 i0 = (dpad_x == sgn_i32(o->vel_q8.x) ? abs_i32(o->vel_q8.x) : 0);
            i32 ax = (max_i32(HERO_VX_WALK - i0, 0) * 32) >> 8;
            o->vel_q8.x += ax * dpad_x;
            o->drag_q8.x = 256;
        }

        if (dpad_x != sgn_i(h->sprint_ticks)) {
            h->sprint_ticks = 0;
        }
    }

    if (0 < h->jump_btn_buffer) {
        bool32 jump_ground = 0 < h->edgeticks;
        bool32 jump_midair = !usinghook &&         // not hooked
                             !jump_ground &&       // jump in air?
                             h->jumpticks <= -8 && // wait some ticks after last jump
                             0 < h->flytime;

        if (jump_midair && !jump_ground) { // just above ground -> ground jump
            for (i32 y = 0; y < 6; y++) {
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
        } else if (jump_midair) {
            h->flytime--;
            h->sprint_ticks = 0;
            hero_start_jump(g, o, HERO_JUMP_FLY);

            rec_i32 rwind = {0, 0, 64, 64};
            v2_i32  dcpos = obj_pos_center(o);
            dcpos.x -= 32;
            dcpos.y += 0;
            i32 flip = rngr_i32(0, 1) ? 0 : SPR_FLIP_X;

            spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, dcpos, TEXID_WINDGUSH, rwind, 18, 6, flip);
        }
    }

    if (h->flytime == flytimep) {
        if (0 < h->flytime && h->flytime < g->save.flytime) {
            h->flytime_recover++;
            h->flytime += (h->flytime_recover & 7) == 0;
        }
    } else {
        h->flytime_recover = 0;
    }
}
