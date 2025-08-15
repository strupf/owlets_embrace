// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl/owl.h"

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
        h->air_walljump_ticks = 0;
        o->v_q12.x            = 0;
        o->subpos_q12.x       = 0;
    }
    o->bumpflags = 0;

    // gravity
    if (!h->air_stomp) {
        // low gravity at peak of jump curve
        // lerp from normal gravity to lowest gravity and back again

        i32 grav = OWL_GRAVITY;
        if (OWL_GRAVITY_V_BEG < o->v_q12.y && o->v_q12.y <= OWL_GRAVITY_V_PEAK) {
            i32 k1 = o->v_q12.y - OWL_GRAVITY_V_BEG;
            i32 k2 = OWL_GRAVITY_V_PEAK - OWL_GRAVITY_V_BEG;
            grav   = lerp_i32(OWL_GRAVITY, OWL_GRAVITY_LOW, k1, k2);
        }
        if (OWL_GRAVITY_V_PEAK <= o->v_q12.y && o->v_q12.y < OWL_GRAVITY_V_END) {
            i32 k1 = o->v_q12.y - OWL_GRAVITY_V_PEAK;
            i32 k2 = OWL_GRAVITY_V_END - OWL_GRAVITY_V_PEAK;
            grav   = lerp_i32(OWL_GRAVITY_LOW, OWL_GRAVITY, k1, k2);
        }
        o->v_q12.y += grav;
    }

    if (h->jump_edgeticks) {
        h->jump_edgeticks--;
    }
    if (h->jump_anim_ticks) {
        h->jump_anim_ticks++;
        if (OWL_AIR_JUMP_FLAP_TICKS <= h->jump_anim_ticks) {
            h->jump_anim_ticks = 0;
        }
    }
    if (h->air_walljump_ticks) {
        h->air_walljump_ticks--;
    }

    bool32 do_x_movement = 1;
    i32    x_movement_q8 = 256; // only relevant for rope

    v2_i32 v_q12_post_rope = o->v_q12;
    if (o->wire) {
        // lerps between rope and normal inair movement around len max
#define ROPE_L1 -256
#define ROPE_L2 -16
        // positive if longer than max len
        // negative if slack
        i32 rope_l_dt = (i32)wire_len_qx(g, o->wire, 4) - (i32)g->ghook.len_max_q4;
        rope_l_dt     = clamp_i32(rope_l_dt, ROPE_L1, ROPE_L2);
        x_movement_q8 = lerp_i32(256, 0, rope_l_dt - ROPE_L1, ROPE_L2 - ROPE_L1);
        do_x_movement = 1;

        v2_i32 rn_curr  = o->wirenode->p;
        v2_i32 rn_next  = wirenode_neighbour_of_end_node(o->wire, o->wirenode)->p;
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
        if (inps_btn_jp(inp, INP_A) && 0 < dp_y && !h->aim_ticks) {
            do_x_movement = 0;
            o->v_q12.x    = 0;
            o->v_q12.y    = 0;
            owl_cancel_air(g, o);
            h->air_stomp = 1;
        } else if (inps_btn_jp(inp, INP_A) && !h->aim_ticks) {
            bool32 jump_ground = 0;

            if (h->jump_edgeticks) {
                jump_ground = 1;
            } else {
                for (i32 y = 1; y < 6; y++) {
                    rec_i32 rr = {o->pos.x, o->pos.y + o->h - 1 + y, o->w, y};
                    v2_i32  pp = {0, y};
                    if (!map_blocked(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                        obj_move(g, o, 0, y);
                        jump_ground = 1;
                        break;
                    }
                }
            }

            if (jump_ground) { // jump off ground?
                owl_on_touch_ground(g, o);
                owl_cancel_air(g, o);
                owl_jump_ground(g, o);
                jumped        = 1;
                do_x_movement = 0;
            } else if (dp_x && owl_jump_wall(g, o, dp_x)) { // jump off wall?
                jumped        = 1;
                do_x_movement = 0;
#if OWL_USE_ALT_AIR_JUMPS
            } else {
#else
            } else if (h->stamina) { // can jump in air?
#endif
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
                i32 t0 = (pow_i32(h->jump_ticks_max, 4) >> 8);
                i32 ti = (pow_i32(h->jump_ticks, 4) >> 8) - t0;
                i32 j0 = h->jump_vy0;
                i32 j1 = h->jump_vy1;
                i32 ch = j0 - ((j1 - j0) * ti) / t0;
                o->v_q12.y -= ch;
                h->jump_ticks--;
#if !OWL_USE_ALT_AIR_JUMPS
                if (h->jump_index == OWL_JUMP_AIR_0) {
                    owl_stamina_modify(o, -OWL_STAMINA_AIR_JUMP_HOLD);
                    if (!h->stamina) {
                        h->jump_ticks = 0;
                    }
                }
#endif
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

        if (dp_x != vs) {
            v_q12_post_x.x = shr_balanced_i32(v_q12_post_x.x * 240, 8);
        }

        if (vs == 0) {
            ax = Q_VOBJ(0.80);
        } else if (dp_x == +vs && va < OWL_VX_WALK) { // press same dir as velocity
            ax = lerp_i32(Q_VOBJ(0.80), Q_VOBJ(0.08), va, OWL_VX_WALK);
            ax = min_i32(ax, OWL_VX_WALK - va);
        } else if (dp_x == -vs) {
            ax = min_i32(Q_VOBJ(0.27), va);
        }

        if (h->air_walljump_ticks && o->facing == -dp_x) {
            i32 i0 = OWL_WALLJUMP_TICKS_BLOCK - h->air_walljump_ticks;
            // ax     = lerp_i32(0, ax, pow2_i32(i0), POW2(OWL_WALLJUMP_TICKS_BLOCK));
        }

#if 0
        if (h->air_block_ticks && sgn_i32(h->air_block_ticks) == -dpad_x) {
            i32 i0 = h->air_block_ticks_og - abs_i32(h->air_block_ticks);
            i32 i1 = h->air_block_ticks_og;
            ax     = lerp_i32(0, ax, pow2_i32(i0), pow2_i32(i1));
        }
#endif
        v_q12_post_x.x += ax * dp_x;
    }
#if 1
    o->v_q12.x = shr_balanced_i32(
        v_q12_post_x.x * x_movement_q8 + v_q12_post_rope.x * (256 - x_movement_q8),
        8);
#endif
    o->v_q12.y = shr_balanced_i32(
        o->v_q12.y * x_movement_q8 + v_q12_post_rope.y * (256 - x_movement_q8),
        8);

    if (o->v_q12.y >= Q_VOBJ(7.0)) {
        o->animation++;
        g->cam.cowl.force_higher_floor = min_i32(o->animation, 24);
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
    owl_jumpvar_s jv  = g_owl_jumpvar[OWL_JUMP_AIR_0];
    h->jump_index     = OWL_JUMP_AIR_0;
    h->jump_edgeticks = 0;
#if OWL_USE_ALT_AIR_JUMPS
#if 1
    h->n_air_jumps_max = 2;
    if (h->n_air_jumps + 1 == h->n_air_jumps_max) {
        h->jump_index = OWL_JUMP_AIR_1;
    } else if (h->n_air_jumps >= h->n_air_jumps_max) {
        h->jump_index = OWL_JUMP_AIR_2;
    } else {
        h->jump_index = OWL_JUMP_AIR_0;
    }
    jv = g_owl_jumpvar[h->jump_index];

    i32 vy             = jv.vy;
    h->jump_ticks_max  = jv.ticks;
    h->jump_ticks      = h->jump_ticks_max;
    h->jump_vy0        = jv.v0;
    h->jump_vy1        = jv.v1;
    h->jump_anim_ticks = 1;
#else
    i32 vy            = max_i32(jv.vy - Q_VOBJ(0.25) * h->n_air_jumps, 0);
    h->jump_ticks_max = max_i32(jv.ticks - 5 * h->n_air_jumps, 0);
    h->jump_ticks     = h->jump_ticks_max;
    h->jump_vy0       = jv.v0;
    h->jump_vy1       = jv.v1;
#endif

#else
    i32 vy            = jv.vy;
    h->jump_ticks_max = jv.ticks;
    h->jump_ticks     = h->jump_ticks_max;
    h->jump_vy0       = jv.v0;
    h->jump_vy1       = jv.v1;
#endif

    h->n_air_jumps++;
    if (o->v_q12.y <= -vy) {
        o->v_q12.y -= Q_VOBJ(0.5);
    } else {
        o->v_q12.y = -vy;
    }

    snd_play(SNDID_JUMP, 0.5f, rngr_f32(0.9f, 1.1f));
    v2_i32 prtp = obj_pos_bottom_center(o);
    prtp.y -= 4;
    particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP, prtp);
    h->jump_snd_iID = snd_play(SNDID_WING, 1.4f, rngr_f32(0.9f, 1.1f));
}
