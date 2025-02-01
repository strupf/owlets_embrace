// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

enum {
    CAM_LOCKON_NONE,
    CAM_LOCKON_POS,
    CAM_LOCKON_OBJ,
};

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

static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center);

void cam_lockon(cam_s *c, v2_i32 p, obj_s *lockon_obj)
{
    c->lockon     = p;
    c->lockon_obj = obj_handle_from_obj(lockon_obj);
    c->has_lockon = lockon_obj ? CAM_LOCKON_OBJ : CAM_LOCKON_POS;
}

void cam_lockon_rem(cam_s *c)
{
    c->has_lockon = CAM_LOCKON_NONE;
}

void cam_screenshake_xy(cam_s *c, i32 ticks, i32 str_x, i32 str_y)
{
    c->shake_ticks     = ticks;
    c->shake_ticks_max = ticks;
    c->shake_str_x     = str_x;
    c->shake_str_y     = str_y;
}

void cam_screenshake(cam_s *c, i32 ticks, i32 str)
{
    cam_screenshake_xy(c, ticks, str, str);
}

v2_i32 cam_pos_px_top_left(g_s *g, cam_s *c)
{
    v2_i32 pos_q8 = v2_i32_add(c->pos_q8, c->attract);
    v2_i32 pos    = v2_i32_shr(pos_q8, 8);
    pos.x += c->offs_x;

    if (40 <= c->lookdown) {
        pos.y += min_i32((c->lookdown - 40), 40) * 2;
    }

    if (c->has_lockon) {
        obj_s *o = obj_from_obj_handle(c->lockon_obj);
        if (o) {
            c->lockon = obj_pos_center(o);
        }
        pos = v2_i32_lerp(pos, c->lockon, c->lock_fade_q8, 256);
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
    v2_i32    ppos          = c->pos_q8;
    v2_i32    padd          = {0};
    const i32 lookdown_prev = c->lookdown;

    if (c->has_lockon == CAM_LOCKON_OBJ &&
        !obj_from_obj_handle(c->lockon_obj)) {
        c->has_lockon = CAM_LOCKON_NONE;
    }
    if (c->has_lockon) {
        c->lock_fade_q8 = min_i32(c->lock_fade_q8 + 4, 256);
    } else if (c->lock_fade_q8) {
        c->lock_fade_q8 = max_i32(c->lock_fade_q8 - 4, 256);
    }

    obj_s *hero = obj_get_hero(g);

    if (hero) {
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

            if (hero->v_q8.x == 0 && inp_btn(INP_DD)) {
                c->lookdown += 2;
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
        v2_i32 attract   = {0};
        if (!c->has_lockon) { // ignore attractors if locked on
            v2_i32 pos_px = v2_i32_shr(c->pos_q8, 8);
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

                attr.x  = (attr.x * (i32)(ds - ls)) / (i32)ds;
                attr.y  = (attr.y * (i32)(ds - ls)) / (i32)ds;
                attract = v2_i32_add(attract, attr);

                n_attract++;
            }
        }

        if (n_attract) {
            attract = v2_i32_shl(attract, 8);
            attract.x /= n_attract;
            attract.y /= n_attract;
            c->attract.x += ((attract.x - c->attract.x) * 16) / 256;
            c->attract.y += ((attract.y - c->attract.y) * 16) / 256;
            c->attract = v2_i32_truncatel(c->attract, 16000);
            c->offs_x -= sgn_i32(c->offs_x);
        } else {
            c->attract.x = (c->attract.x * 253) / 256;
            c->attract.y = (c->attract.y * 253) / 256;

            c->offs_x += hero->facing + sgn_i32(hero->v_q8.x);
            c->offs_x      = clamp_sym_i32(c->offs_x, CAM_FACE_OFFS_X);
            c->can_align_x = abs_i32(c->offs_x) == CAM_FACE_OFFS_X;
        }
    }

    if (0 < lookdown_prev && lookdown_prev == c->lookdown) {
        c->lookdown = min_i32(c->lookdown, 256);
        c->lookdown = max_i32(c->lookdown - 2, 0);
    }

    if (c->shake_ticks) {
        i32 shakex = (c->shake_str_x * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
        i32 shakey = (c->shake_str_y * c->shake_ticks + (c->shake_ticks_max >> 1)) / c->shake_ticks_max;
        c->shake_ticks--;
        c->shake.x = rngr_sym_i32(shakex);
        c->shake.y = rngr_sym_i32(shakey);
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