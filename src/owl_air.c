// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl.h"

void owl_air(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h    = (owl_s *)o->heap;
    i32    dp_x = inps_x(inp);
    i32    dp_y = inps_y(inp);

    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y           = 0;
        o->subpos_q12.y      = 0;
        h->jump_ticks        = 0;
        h->jump_index        = 0;
        h->jump_snd_iID      = 0;
        h->jump_ground_ticks = 0;
    }
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    }
    o->bumpflags = 0;

    // gravity
    if (!h->air_stomp) {
        // if (h->walljump_tick) break;
        // low gravity at peak of jump curve
        // lerp from normal gravity to lowest gravity and back again
#define LOW_GRAV_Q8 +Q_8(0.63)
#define LOW_GRAV_LO -Q_VOBJ(0.80) // v q8 when low gravity starts
#define LOW_GRAV_HI +Q_VOBJ(0.35) // v q8 when low gravity ends
#define LOW_GRAV_PK +0            // v q8 with lowest gravity

        i32 grav_k_q8 = 256;
        if (LOW_GRAV_LO < o->v_q12.y && o->v_q12.y <= LOW_GRAV_PK) {
            i32 k1    = o->v_q12.y - LOW_GRAV_LO;
            i32 k2    = LOW_GRAV_PK - LOW_GRAV_LO;
            grav_k_q8 = lerp_i32(256, LOW_GRAV_Q8, k1, k2);
        }
        if (LOW_GRAV_PK <= o->v_q12.y && o->v_q12.y < LOW_GRAV_HI) {
            i32 k1    = o->v_q12.y - LOW_GRAV_PK;
            i32 k2    = LOW_GRAV_HI - LOW_GRAV_PK;
            grav_k_q8 = lerp_i32(LOW_GRAV_Q8, 256, k1, k2);
        }
        // grav_k_q8 = 256;
        o->v_q12.y += (OWL_GRAVITY * grav_k_q8) >> 8;
    }

    if (h->jump_edgeticks) {
        h->jump_edgeticks--;
    }

    bool32 do_x_movement = 1;
    i32    x_movement_q8 = 256; // only relevant for rope

    v2_i32 v_q12_post_rope = o->v_q12;
    if (o->rope) {
        // lerps between rope and normal inair movement around len max
#define ROPE_L1 -256
#define ROPE_L2 -16
        // positive if longer than max len
        // negative if slack
        i32 rope_l_dt = (i32)rope_len_q4(g, o->rope) - (i32)o->rope->len_max_q4;
        rope_l_dt     = clamp_i32(rope_l_dt, ROPE_L1, ROPE_L2);
        x_movement_q8 = lerp_i32(256, 0, rope_l_dt - ROPE_L1, ROPE_L2 - ROPE_L1);
        do_x_movement = 1;

        v2_i32 rn_curr  = o->ropenode->p;
        v2_i32 rn_next  = ropenode_neighbour(o->rope, o->ropenode)->p;
        v2_i32 dtrope   = v2_i32_sub(rn_next, rn_curr);
        i32    dtrope_s = sgn_i32(dtrope.x);
        i32    dtrope_a = abs_i32(dtrope.x);

        // apply swinging force tangentially to the swinging arc
        v2_i32 ptang = {-dtrope.y, +dtrope.x}; // tangent counter clockwise

        if (dp_x) {
            if (dp_x < 0) {
                ptang = v2_i32_inv(ptang);
            }
            ptang           = v2_i32_setlen(ptang, Q_VOBJ(0.15));
            v_q12_post_rope = v2_i32_add(v_q12_post_rope, ptang);
            // v_q12_post_rope.x += dp_x * Q_VOBJ(0.05);
        }
    } else {
        bool32 jumped = 0;
        if (inps_btn_jp(inp, INP_A) && 0 < dp_y) {
            do_x_movement = 0;
            o->v_q12.x    = 0;
            o->v_q12.y    = 0;
            owl_cancel_air(g, o);
            h->air_stomp = 1;
        } else if (inps_btn_jp(inp, INP_A)) {
            bool32 jump_ground = h->jump_edgeticks;

            for (i32 y = 1; y < 6; y++) {
                rec_i32 rr = {o->pos.x, o->pos.y + o->h - 1 + y, o->w, y};
                v2_i32  pp = {0, y};
                if (!map_blocked(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                    obj_move(g, o, 0, y);
                    jump_ground = 1;
                    break;
                }
            }

            if (jump_ground) {
                do_x_movement = 0;
                owl_on_touch_ground(g, o);
                owl_cancel_air(g, o);
                owl_jump_ground(g, o);
                jumped = 1;
            } else if (h->stamina) {
                owl_jump_air(g, o);
                owl_stamina_modify(o, -OWL_STAMINA_AIR_JUMP_INIT);
                jumped = 1;
            }
        }

        if (jumped) {
        } else if (h->air_stomp) {
            do_x_movement = 0;
            if (h->air_stomp < OWL_AIR_STOMP_MAX) {
                h->air_stomp++;
            }
            if (OWL_AIR_STOMP_MAX / 2 <= h->air_stomp) {
                i32 t1     = h->air_stomp - OWL_AIR_STOMP_MAX / 2;
                i32 t2     = OWL_AIR_STOMP_MAX - OWL_AIR_STOMP_MAX / 2;
                o->v_q12.y = lerp_i32(0, Q_VOBJ(7.0), t1, t2);
            }
        } else if (0 < h->jump_ticks) { // dynamic jump height
            if (inps_btn(inp, INP_A)) {
                owl_jumpvar_s jv = g_owl_jumpvar[h->jump_index];
                i32           t0 = (pow_i32(jv.ticks, 4) >> 8);
                i32           ti = (pow_i32(h->jump_ticks, 4) >> 8) - t0;
                i32           ch = jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
                o->v_q12.y -= ch;
                h->jump_ticks--;
                if (h->jump_index == OWL_JUMP_FLY) {
                    owl_stamina_modify(o, -OWL_STAMINA_AIR_JUMP_HOLD);
                    if (!h->stamina) {
                        h->jump_ticks = 0;
                    }
                }
            } else { // cut jump short
                snd_instance_stop_fade(h->jump_snd_iID, 200, 256);
                obj_vy_q8_mul(o, Q_8(0.5));
                h->jump_ticks = 0;
                // h->air_block_ticks >>= 1;
            }
        }
    }

    v2_i32 v_q12_post_x = o->v_q12;
    if (do_x_movement) {
        i32 vs = sgn_i32(v_q12_post_x.x);
        i32 va = abs_i32(v_q12_post_x.x);
        i32 ax = 0;

        if (vs == 0) {
            ax = Q_VOBJ(0.78);
        } else if (dp_x == +vs && va < OWL_VX_WALK) { // press same dir as velocity
            ax = lerp_i32(Q_VOBJ(0.78), Q_VOBJ(0.08), va, OWL_VX_WALK);
            ax = min_i32(ax, OWL_VX_WALK - va);
        } else if (dp_x == -vs) {
            ax = min_i32(Q_VOBJ(0.39), va);
        }

#if 0
        if (h->air_block_ticks && sgn_i32(h->air_block_ticks) == -dpad_x) {
            i32 i0 = h->air_block_ticks_og - abs_i32(h->air_block_ticks);
            i32 i1 = h->air_block_ticks_og;
            ax     = lerp_i32(0, ax, pow2_i32(i0), pow2_i32(i1));
        }
#endif

        if (dp_x != vs) {
            obj_vx_q8_mul(o, Q_8(0.95));
        }
        v_q12_post_x.x += ax * dp_x;
    }

    o->v_q12.x = shr_balanced_i32(
        v_q12_post_x.x * x_movement_q8 + v_q12_post_rope.x * (256 - x_movement_q8),
        8);
    o->v_q12.y = shr_balanced_i32(
        o->v_q12.y * x_movement_q8 + v_q12_post_rope.y * (256 - x_movement_q8),
        8);

    if (o->v_q12.y >= Q_VOBJ(7.0)) {
        o->animation++;
        g->cam.cowl.force_higher_floor = min_i32(o->animation >> 1, 24);
    } else {
        o->animation = 0;
    }
    if (o->v_q12.y >= Q_VOBJ(8.0)) {
        o->v_q12.y = Q_VOBJ(8.0);
    }
    obj_move_by_v_q12(g, o);
}

void owl_jump_air(g_s *g, obj_s *o)
{
    owl_s        *h   = (owl_s *)o->heap;
    owl_jumpvar_s jv  = g_owl_jumpvar[OWL_JUMP_FLY];
    h->jump_index     = OWL_JUMP_FLY;
    h->jump_ticks     = (u8)jv.ticks;
    h->jump_edgeticks = 0;

    if (o->v_q12.y <= -jv.vy) {
        o->v_q12.y -= Q_VOBJ(0.5);
    } else {
        o->v_q12.y = -jv.vy;
    }

    snd_play(SNDID_JUMP, 0.5f, rngr_f32(0.9f, 1.1f));
    v2_i32 prtp = obj_pos_bottom_center(o);
    prtp.y -= 4;
    particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP, prtp);
    h->jump_snd_iID = snd_play(SNDID_WING, 1.4f, rngr_f32(0.9f, 1.1f));
}