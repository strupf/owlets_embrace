// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_W                     PLTF_DISPLAY_W
#define CAM_H                     PLTF_DISPLAY_H
#define CAM_WH                    (PLTF_DISPLAY_W >> 1)
#define CAM_HH                    (PLTF_DISPLAY_H >> 1)
#define CAM_FACE_OFFS_X           50
#define CAM_Y_NEW_BASE_MIN_SPEED  (1 << 7)
#define CAM_X_LOOKAHEAD_MIN_SPEED (1 << 7)
#define CAM_HERO_Y_BOT            172                   // lower = camera positioned lower
#define CAM_HERO_Y_TOP            (CAM_HERO_Y_BOT - 74) // tune for jump height
#define CAM_OFFS_Q8_TOP           ((-120 + CAM_HERO_Y_TOP) * 256)
#define CAM_OFFS_Q8_BOT           ((-120 + CAM_HERO_Y_BOT) * 256)

// a, b: half width/height of ellipse, p considered relative to center
static v2_f32 v2f_truncate_to_ellipse(v2_f32 p, f32 a, f32 b);
static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center);

void cam_screenshake_xy(cam_s *c, i32 ticks, i32 str_x, i32 str_y)
{
    if (!pltf_reduce_flashing()) {
        c->shake_ticks     = ticks;
        c->shake_ticks_max = ticks;
        c->shake_str_x     = str_x;
        c->shake_str_y     = str_y;
    }
}

void cam_screenshake(cam_s *c, i32 ticks, i32 str)
{
    cam_screenshake_xy(c, ticks, str, str);
}

v2_i32 cam_pos_px_top_left(g_s *g, cam_s *c)
{
    v2_i32 pos = v2_i32_shr(c->pos_q8, 8);
    pos.x += (i32)c->attract.x + c->offs_x;
    pos.y += (i32)c->attract.y;

    if (0 < c->lookdownup_q8) {
        pos.y += ease_in_out_quad(0, 75, +c->lookdownup_q8, 256);
    } else {
        pos.y -= ease_in_out_quad(0, 60, -c->lookdownup_q8, 256);
    }

    if (c->trg_fade_q12) {
        pos.x = ease_in_out_sine(pos.x, c->trg.x,
                                 c->trg_fade_q12, CAM_TRG_FADE_MAX);
        pos.y = ease_in_out_sine(pos.y, c->trg.y,
                                 c->trg_fade_q12, CAM_TRG_FADE_MAX);
    }

    pos = cam_constrain_to_room(g, pos);
    pos = v2_i32_add(pos, c->shake);
    return pos;
}

v2_i32 cam_pos_px_center(g_s *g, cam_s *c)
{
    v2_i32 pos        = cam_pos_px_top_left(g, c);
    v2_i32 pos_center = {pos.x + CAM_WH, pos.y + CAM_HH};
    return pos_center;
}

rec_i32 cam_rec_px(g_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px_top_left(g, c);
    rec_i32 r = {p.x - CAM_WH, p.y - CAM_HH, CAM_W, CAM_H};
    return r;
}

void cam_init_level(g_s *g, cam_s *c)
{
    // update all the panning etc. which could happen
    for (i32 n = 0; n < 128; n++) {
        cam_update(g, c);
    }
}

void cam_update(g_s *g, cam_s *c)
{
    v2_i32 ppos         = c->pos_q8;
    v2_i32 padd         = {0};
    obj_s *hero         = obj_get_hero(g);
    bool32 look_up_down = 0;

    if (hero) {
        hero_s *h           = (hero_s *)hero->heap;
        c->can_align_x      = 0;
        c->can_align_y      = 0;
        v2_i32 herop        = obj_pos_bottom_center(hero);
        bool32 herogrounded = obj_grounded(g, hero);
        i32    hero_bot_q8  = herop.y << 8;

        if (herogrounded && 0 <= hero->v_q8.y) {
            // move camera upwards if hero landed on new platform
            // "new base height"
            i32 cam_y_bot_q8 = c->pos_q8.y + CAM_OFFS_Q8_BOT; // align bottom with platform
            i32 diffy_q8     = hero_bot_q8 - cam_y_bot_q8;
            i32 y_add        = (diffy_q8 * 15) >> 8;
            if (diffy_q8 < 0) {
                y_add = min_i32(y_add, -CAM_Y_NEW_BASE_MIN_SPEED);
            }
            if (diffy_q8 > 0) {
                y_add = max_i32(y_add, +CAM_Y_NEW_BASE_MIN_SPEED);
            }
            c->pos_q8.y += y_add;

            if (h->crouched && inp_btn(INP_DD)) {
                look_up_down = +1;
            } else if (inp_btn(INP_DU)) {
                look_up_down = -1;
            }
        }

        i32 cam_y_min = hero_bot_q8 - CAM_OFFS_Q8_BOT;
        i32 cam_y_max = hero_bot_q8 - CAM_OFFS_Q8_TOP;
        if (c->pos_q8.y <= cam_y_min) {
            c->pos_q8.y    = cam_y_min;
            c->can_align_y = 1;
        }
        if (c->pos_q8.y >= cam_y_max) {
            c->pos_q8.y    = cam_y_max;
            c->can_align_y = 1;
        }

        c->pos_q8.x = herop.x << 8;

        // cam attractors
        i32    n_attract = 0;
        v2_f32 attract   = {0};

        v2_i32 pos_px = v2_i32_shr(c->pos_q8, 8);
        // pos_px.x += c->offs_x + clamp_sym_i32(hero->v_q8.x >> 4, 64);
        pos_px.x += c->offs_x;
        pos_px = cam_constrain_to_room(g, pos_px);

        for (obj_each(g, o)) {
            if (!o->cam_attract_r) continue;

            v2_i32 pattr;
            if (o->ID == OBJID_CAMATTRACTOR) {
                pattr = camattractor_static_closest_pt(o, pos_px);
            } else {
                pattr = obj_pos_center(o);
            }

            v2_i32 attr = v2_i32_sub(pattr, pos_px);
            u32    ds   = pow2_u32(o->cam_attract_r);
            u32    ls   = v2_i32_lensq(attr);
            if (ds <= ls) continue;

            attr.x = (attr.x * (i32)(ds - ls)) / (i32)ds;
            attr.y = (attr.y * (i32)(ds - ls)) / (i32)ds;
            attract.x += (f32)attr.x;
            attract.y += (f32)attr.y;
            n_attract++;
        }

        if (n_attract) {
            v2_f32 attract_move = {
                (attract.x / (f32)n_attract - c->attract.x) * 0.05f,
                (attract.y / (f32)n_attract - c->attract.y) * 0.05f};
            attract_move = v2f_truncate(attract_move, 25.f);
            c->attract   = v2f_add(c->attract, attract_move);
            c->attract   = v2f_truncate_to_ellipse(c->attract, 80.f, 40.f);
            c->offs_x -= sgn_i32(c->offs_x);
        } else {
            c->attract.x *= 0.96f;
            c->attract.y *= 0.96f;
            c->offs_x += hero->facing + sgn_i32(hero->v_q8.x);
            c->offs_x      = clamp_sym_i32(c->offs_x, CAM_FACE_OFFS_X);
            c->can_align_x = abs_i32(c->offs_x) == CAM_FACE_OFFS_X;
        }
    }

    if (look_up_down) {
        if (25 <= c->lookdownup_tick) {
            c->lookdownup_q8 += look_up_down << 3;
            c->lookdownup_q8 = clamp_i32(c->lookdownup_q8, -256, +256);
        } else {
            c->lookdownup_tick++;
        }
    } else {
        i32 dldu = min_i32(abs_i32(c->lookdownup_q8), 8);
        c->lookdownup_q8 -= dldu * sgn_i32(c->lookdownup_q8);
        if (c->lookdownup_q8 == 0) {
            c->lookdownup_tick = 0;
        }
    }

    if (c->shake_ticks) {
        i32 shakex = (c->shake_str_x * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
        i32 shakey = (c->shake_str_y * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
        c->shake_ticks--;
        if (c->shake_str_x) {
            shakex = max_i32(shakex, 1);
        }
        if (c->shake_str_y) {
            shakey = max_i32(shakey, 1);
        }
        c->shake.x = rngr_sym_i32(shakex);
        c->shake.y = rngr_sym_i32(shakey);
    }

    if (c->has_trg) {
        c->trg_fade_q12 =
            min_i32(c->trg_fade_q12 + c->trg_fade_spd, CAM_TRG_FADE_MAX);
    } else {
        c->trg_fade_q12 =
            max_i32(c->trg_fade_q12 - c->trg_fade_spd, 0);
    }

    if (c->locked_x) {
        c->pos_q8.x = ppos.x;
    }
    if (c->locked_y) {
        c->pos_q8.y = ppos.y;
    }
}

static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center)
{
    v2_i32 v = {clamp_i32(p_center.x, CAM_WH, g->pixel_x - CAM_WH),
                clamp_i32(p_center.y, CAM_HH, g->pixel_y - CAM_HH)};
    return v;
}

v2_i32 cam_offset_max(g_s *g, cam_s *c)
{
    v2_i32 o = {g->pixel_x - CAM_W,
                g->pixel_y - CAM_H};
    return o;
}

f32 cam_snd_scale(g_s *g, v2_i32 p, u32 dst_max)
{
    f32 dsq = sqrt_f32(pow_f32((f32)p.x - (f32)g->cam_prev_world.x, 2) +
                       pow_f32((f32)p.y - (f32)g->cam_prev_world.y, 2));
    f32 vol = ((f32)dst_max - min_f32(dsq, (f32)dst_max)) / (f32)dst_max;
    return vol;
}

static v2_f32 v2f_truncate_to_ellipse(v2_f32 p, f32 a, f32 b)
{
    f32 d = a * b / sqrt_f32(b * b * p.x * p.x + a * a * p.y * p.y);
    if (d < 1.f) {
        v2_f32 r = {p.x * d, p.y * d};
        return r;
    } else {
        return p;
    }
}