// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl.h"

void owl_water(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h           = (owl_s *)o->heap;
    i32     dp_x        = inps_x(inp);
    i32     dp_y        = inps_y(inp);
    rec_i32 owlr        = obj_aabb(o);
    v2_i32  owlc        = obj_pos_center(o);
    i32     water_depth = water_depth_rec(g, owlr);

    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y      = 0;
        o->subpos_q12.y = 0;
    }
    o->bumpflags = 0;

    if (h->swim == OWL_SWIM_DIVE && water_depth < OWL_H) { // not submerged anymore
        h->swim = OWL_SWIM_SURFACE;
    }

    if (inps_btn_jp(inp, INP_A) && water_depth < OWL_H) { // jump out of water
        obj_move(g, o, 0, -water_depth);
        owl_jump_out_of_water(g, o);
    } else {
        switch (h->swim) {
        case OWL_SWIM_SURFACE: {
            if (inps_btn_jp(inp, INP_A) && water_depth < OWL_H) { // jump out of water
                obj_move(g, o, 0, -water_depth);
                owl_jump_out_of_water(g, o);
                break;
            }
            if (inps_btn_jp(inp, INP_DD)) {
                i32     move_at_least = OWL_H - water_depth;
                rec_i32 rdive         = {o->pos.x, o->pos.y + o->h, o->w, move_at_least};
                if (!map_blocked(g, rdive)) {
                    obj_move(g, o, 0, move_at_least);
                    h->swim    = OWL_SWIM_DIVE;
                    o->v_q12.y = Q_VOBJ(2.0);
                    break;
                }
            }

            bool32 can_swim         = 1;
            i32    dty_snap_surface = OWL_WATER_THRESHOLD - water_depth;
            if (o->v_q12.y <= OWL_GRAVITY && abs_i32(dty_snap_surface) <= 1) {
                if (dty_snap_surface) {
                    rec_i32 rdive = {o->pos.x, o->pos.y + o->h, o->w, dty_snap_surface};
                    if (!map_blocked(g, rdive)) {
                        obj_move(g, o, 0, dty_snap_surface);
                        o->v_q12.y = 0;
                    }
                }
            } else {
                i32 acc_upwards = lerp_i32(OWL_GRAVITY,
                                           OWL_GRAVITY + Q_VOBJ(0.2),
                                           water_depth - OWL_WATER_THRESHOLD,
                                           (OWL_H - OWL_WATER_THRESHOLD));
                obj_vy_q8_mul(o, Q_8(0.90));
                o->v_q12.y += OWL_GRAVITY;
                o->v_q12.y -= acc_upwards;
            }

            break;
        }
        case OWL_SWIM_DIVE: {
            if (inps_btn(inp, INP_A)) {
                dp_y = -1;
            }
            if (inps_btn(inp, INP_B)) {
                dp_y = +1;
            }
            if (dp_y) {
                o->v_q12.y += dp_y * Q_VOBJ(0.2);
                o->v_q12.y = clamp_sym_i32(o->v_q12.y, Q_VOBJ(2.0));
            } else {
                obj_vy_q8_mul(o, Q_8(0.90));
            }

            break;
        }
        }

        if (dp_x != sgn_i32(o->v_q12.x)) {
            o->v_q12.x /= 2;
        }
        if (dp_x) {
            i32 i0 = (dp_x == sgn_i32(o->v_q12.x) ? abs_i32(o->v_q12.x) : 0);
            i32 ax = (max_i32(OWL_VX_SWIM - i0, 0) * 32) >> 8;
            o->v_q12.x += ax * dp_x;
            if (!h->swim_sideways) {
                h->swim_sideways = 1;
                h->swim_anim     = 0;
            }
        } else {
            if (h->swim_sideways) {
                h->swim_sideways = 0;
                h->swim_anim     = 0;
            }
        }
        obj_move_by_v_q12(g, o);
    }
}

i32 owl_swim_frameID(i32 animation)
{
    return ((animation / 6) % 6);
}

i32 owl_swim_frameID_idle(i32 animation)
{
    return ((animation >> 4) & 7);
}

void owl_jump_out_of_water(g_s *g, obj_s *o)
{
    rec_i32 owlr = obj_aabb(o);
    v2_i32  owlc = obj_pos_center(o);
    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa = &g->fluid_areas[n];
        rec_i32       fr = {fa->x, fa->y - 4, fa->w, 16};
        if (!overlap_rec(fr, owlr)) continue;

        fluid_area_impact(fa, owlc.x - fa->x, 12, -150, FLUID_AREA_IMPACT_FLAT);
    }

    owl_cancel_swim(g, o);
    owl_jump_ground(g, o);
    particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_WATER_SPLASH, owlc);
    snd_play(SNDID_WATER_OUT_OF, 0.35f, 1.f);
}