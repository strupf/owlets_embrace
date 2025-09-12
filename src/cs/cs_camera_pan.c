// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 pt[24];
    v2_i32 p_cam_q4;
    v2_i32 p_comp_og;
    i32    x_q4;
    i16    v_q4;
    i16    n_at;
    i16    offx;
    i16    offy;
    i8     n_pt;
    b8     circ;
    b8     controllable;
} cs_camera_pan_s;

enum {
    CS_CAMERA_PAN_PHASE_STARTING,
    CS_CAMERA_PAN_PHASE_ACTIVE,
    CS_CAMERA_PAN_PHASE_ENDING
};

void        cs_camera_pan_update(g_s *g, cs_s *cs, inp_s inp);
static void cs_camera_pan_stop(g_s *g, cs_s *cs);
static void cs_camera_pan_update_between(g_s *g, cs_s *cs, inp_s inp,
                                         v2_i32 p0, v2_i32 p1, i32 num, i32 den);

void cs_camera_pan_enter(g_s *g, cs_camera_pan_config_s *pan_config)
{
    cs_s            *cs = &g->cs;
    cs_camera_pan_s *cp = (cs_camera_pan_s *)cs->mem;

    cs->on_update    = cs_camera_pan_update;
    cp->n_pt         = pan_config->n_pt;
    cp->controllable = pan_config->controllable;
    cp->circ         = pan_config->circ;
    for (i32 n = 0; n < pan_config->n_pt; n++) {
        cp->pt[n] = pan_config->pt[n];
    }
}

void cs_camera_pan_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs_camera_pan_s *cp = (cs_camera_pan_s *)cs->mem;

    switch (cs->phase) {
    case CS_CAMERA_PAN_PHASE_STARTING: {
        g->flags |= GAME_FLAG_BLOCK_UPDATE;
        cs->phase++;
        cs->tick = 0;

        g->cam.trg_fade_spd = 128;
        g->cam.has_trg      = 1;
        cp->x_q4            = 0;
        cp->n_at            = 0;
        cp->p_cam_q4        = v2_i32_shl(cp->pt[0], 4);

        obj_s *ocomp = obj_get_comp(g);
        if (cp->controllable && ocomp) {
            app_crank_requested(1);
            cs->p_comp = puppet_companion_put(g, ocomp);
            puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, ocomp->facing);
            cp->p_comp_og = cs->p_comp->pos;
        }
        break;
    }
    case CS_CAMERA_PAN_PHASE_ACTIVE: {

        if (cp->controllable && inps_btn_jp(inp, INP_B)) {
            cs_camera_pan_stop(g, cs);
            break;
        }

        i32 v_q4 = 0;
        if (cs->tick < 50) {
            v_q4 = 48;
        } else {
            if (cp->controllable) {
#if 1
                v_q4 = inps_btn(inp, INP_A) ? 96 : 32;
#else
                v_q4 = inps_crankdt_q16(inp) >> 4;
#endif
                i32 dx = inps_x(inp);
                i32 dy = inps_y(inp);
                if (dx != sgn_i32(cp->offx)) {
                    cp->offx = (cp->offx * 240) / 256;
                }
                if (dx) {
                    cp->offx = clamp_sym_i32(cp->offx + dx * 4, 64);
                }

                if (dy != sgn_i32(cp->offy)) {
                    cp->offy = (cp->offy * 240) / 256;
                }
                if (dy) {
                    cp->offy = clamp_sym_i32(cp->offy + dy * 4, 64);
                }
            } else {
                v_q4 = cp->v_q4;
            }
        }

        cp->x_q4 += v_q4;

        i32    num         = 0;
        i32    den         = 0;
        bool32 reached_end = 0;

        while (1) {
            i32 ln = v2_i32_distance(v2_i32_shl(cp->pt[cp->n_at + 0], 4),
                                     v2_i32_shl(cp->pt[cp->n_at + 1], 4));

            if (0) {
            } else if (cp->x_q4 < 0) {
                if (cp->n_at == 0) {
                    reached_end = 1;
                    break;
                }

                cp->n_at--;
                cp->x_q4 += v2_i32_distance(v2_i32_shl(cp->pt[cp->n_at + 0], 4),
                                            v2_i32_shl(cp->pt[cp->n_at + 1], 4));
            } else if (ln <= cp->x_q4) {
                if (cp->n_at == cp->n_pt - 2) {
                    reached_end = 1;
                    break;
                }

                cp->n_at++;
                cp->x_q4 -= ln;
            } else {
                num = cp->x_q4;
                den = ln;
                break;
            }
        }

        if (reached_end) {
            cs_camera_pan_update_between(g, cs, inp,
                                         cp->pt[cp->n_pt - 2],
                                         cp->pt[cp->n_pt - 1], 1, 1);
            cs->tick = 0;
            cs->phase++;
        } else {
            cs_camera_pan_update_between(g, cs, inp,
                                         cp->pt[cp->n_at + 0],
                                         cp->pt[cp->n_at + 1], num, den);
        }
        break;
    }
    case CS_CAMERA_PAN_PHASE_ENDING: { // smoothing out the exit somewhat
        if (16 <= cs->tick) {
            cs_camera_pan_stop(g, cs);
        } else {
            cs_camera_pan_update_between(g, cs, inp,
                                         cp->pt[cp->n_pt - 2],
                                         cp->pt[cp->n_pt - 1], 1, 1);
        }
        break;
    }
    }
}

static void cs_camera_pan_stop(g_s *g, cs_s *cs)
{
    cs_camera_pan_s *cp = (cs_camera_pan_s *)cs->mem;

    app_crank_requested(0);
    g->cam.has_trg      = 0;
    g->cam.trg_fade_spd = 128;
    g->flags &= ~GAME_FLAG_BLOCK_UPDATE;

    if (cs->p_comp) {
        obj_s *ocomp    = obj_get_comp(g);
        cs->p_comp->pos = v2_i32_shr(cp->p_cam_q4, 4);
        puppet_companion_replace_and_del(g, ocomp, cs->p_comp);
        cs->p_comp = 0;
    }
    cs_reset(g);
}

// updates to move to the position between p0 and p1, with num/den being
// the ratio along the length
static void cs_camera_pan_update_between(g_s *g, cs_s *cs, inp_s inp, v2_i32 p0, v2_i32 p1, i32 num, i32 den)
{
    cs_camera_pan_s *cp        = (cs_camera_pan_s *)cs->mem;
    v2_i32           p_curr_q4 = v2_i32_shl(p0, 4);
    v2_i32           p_next_q4 = v2_i32_shl(p1, 4);
    v2_i32           p_q4      = v2_i32_lerp(p_curr_q4, p_next_q4, num, den);

    cp->p_cam_q4 = v2_i32_lerp(cp->p_cam_q4, p_q4, 1, 12);
    v2_i32 p_cam = v2_i32_shr(cp->p_cam_q4, 4);
    g->cam.trg   = p_cam;
    g->cam.trg.x += cp->offx;
    g->cam.trg.y += cp->offy;

    if (cs->p_comp) {
        if (cs->phase != 2 && cs->tick <= 32) {
            cs->p_comp->pos = v2_i32_lerp(cp->p_comp_og, p_cam, cs->tick, 32);
        } else {
            // align to match camera
            cs->p_comp->pos = p_cam;
            cs->p_comp->pos.x &= ~1;
            cs->p_comp->pos.y &= ~1;
        }

        i32 dx = inps_x(inp);
        if (dx) {
            cs->p_comp->facing = dx;
        } else if (p_next_q4.x < p_curr_q4.x) {
            cs->p_comp->facing = -1;
        } else if (p_next_q4.x > p_curr_q4.x) {
            cs->p_comp->facing = +1;
        }
    }
}
