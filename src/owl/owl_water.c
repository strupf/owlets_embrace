// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl/owl.h"

void owl_water(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h    = (owl_s *)o->heap;
    i32     dp_x = inps_x(inp);
    i32     dp_y = inps_y(inp);
    rec_i32 owlr = obj_aabb(o);

    v2_i32 owlc        = obj_pos_center(o);
    i32    water_depth = water_depth_rec(g, owlr);
    bool32 can_dive    = 1;

    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y      = 0;
        o->subpos_q12.y = 0;
    }
    o->bumpflags = 0;

    if (water_depth < OWL_H && o->health) {
        h->swim = OWL_SWIM_SURFACE;
    } else if (can_dive) {
    }

    if (OWL_H <= water_depth) {
        owl_stamina_modify(o, -OWL_STAMINA_DRAIN_DIVE);
    } else {
        owl_stamina_modify(o, +OWL_STAMINA_RESTORE_SWIM);
    }

    if (inps_btn_jp(inp, INP_A) && water_depth < OWL_H) { // jump out of water
        obj_move(g, o, 0, -water_depth);
        owl_jump_out_of_water(g, o);
    } else {
        switch (h->swim) {
        case OWL_SWIM_SURFACE: {
            i32     dty_snap_surf    = OWL_WATER_THRESHOLD - water_depth;                // delta to snap to surface
            i32     move_at_least    = OWL_H - water_depth;                              // distance to move downwards to be considered diving
            rec_i32 rdive            = {o->pos.x, o->pos.y + o->h, o->w, move_at_least}; // rectangle to move downwards to be considered diving
            bool32  may_dive_down    = can_dive && !map_blocked(g, rdive);
            bool32  can_snap_to_surf = o->v_q12.y <= OWL_GRAVITY &&
                                      abs_i32(dty_snap_surf) <= 1 &&
                                      dty_snap_surf &&
                                      !map_blocked(g, rdive);

            if (inps_btn_jp(inp, INP_A) && water_depth < OWL_H) { // jump out of water
                obj_move(g, o, 0, -water_depth);
                owl_jump_out_of_water(g, o);
            } else if (inps_btn_jp(inp, INP_DD) && may_dive_down) {
                obj_move(g, o, 0, move_at_least);
                h->swim    = OWL_SWIM_DIVE;
                o->v_q12.y = Q_VOBJ(2.0);
            } else if (can_snap_to_surf) {
                obj_move(g, o, 0, dty_snap_surf);
                o->v_q12.y = 0;
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
            if (dp_y && !h->stance_swap_tick) {
                o->v_q12.y += dp_y * Q_VOBJ(0.2);
            } else {
                obj_vy_q8_mul(o, Q_8(0.90));
            }
            o->v_q12.y = clamp_sym_i32(o->v_q12.y, Q_VOBJ(2.0));
            break;
        }
        }

        if (!dp_x || (dp_x != sgn_i32(o->v_q12.x) && !h->stance_swap_tick)) {
            o->v_q12.x = shr_balanced_i32((i32)o->v_q12.x, 1);
        } else if (OWL_VX_SWIM < abs_i32(o->v_q12.x)) {
            i32 vx_dampened = shr_balanced_i32((i32)o->v_q12.x * 246, 8);

            if (+OWL_VX_SWIM < o->v_q12.x) {
                o->v_q12.x = max_i32(vx_dampened, +OWL_VX_SWIM);
            }
            if (-OWL_VX_SWIM > o->v_q12.x) {
                o->v_q12.x = min_i32(vx_dampened, -OWL_VX_SWIM);
            }
        }

        if (dp_x && !h->stance_swap_tick) {
            i32 i0 = (dp_x == sgn_i32(o->v_q12.x) ? abs_i32(o->v_q12.x) : 0);
            i32 ax = (max_i32(OWL_VX_SWIM - i0, 0) * 32) >> 8;

            o->v_q12.x += ax * dp_x;
            if (!h->swim_sideways) {
                h->swim_sideways = 1;
                h->swim_anim     = 0;
            }
        } else if (h->swim_sideways) {
            h->swim_sideways = 0;
            h->swim_anim     = 0;
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
    sfx_cuef(SFXID_WATER_OUT_OF, 0.35f, 1.f);
}