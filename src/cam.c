// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_W           PLTF_DISPLAY_W
#define CAM_H           PLTF_DISPLAY_H
#define CAM_WH          (PLTF_DISPLAY_W >> 1)
#define CAM_HH          (PLTF_DISPLAY_H >> 1)
#define CAM_FACE_OFFS_X 50

static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center);

void cam_screenshake(cam_s *c, i32 ticks, i32 str)
{
    c->shake_ticks     = ticks;
    c->shake_ticks_max = ticks;
    c->shake_str       = str;
}

v2_i32 cam_pos_px(g_s *g, cam_s *c)
{
    v2_i32 pos_q8 = c->pos_q8;
    pos_q8        = v2_add(pos_q8, c->attract);
    v2_i32 pos    = v2_shr(pos_q8, 8);
    pos           = v2_add(pos, c->shake);
    pos.x += c->offs_x;

    if (40 <= c->lookdown) {
        pos.y += min_i32((c->lookdown - 40), 40) * 2;
    }

    pos = cam_constrain_to_room(g, pos);
    return pos;
}

rec_i32 cam_rec_px(g_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px(g, c);
    rec_i32 r = {p.x - CAM_WH, p.y - CAM_HH, CAM_W, CAM_H};
    return r;
}

void cam_set_pos_px(cam_s *c, i32 x, i32 y)
{
    c->pos_q8.x = x << 8;
    c->pos_q8.y = y << 8;
}

void cam_init_level(g_s *g, cam_s *c)
{
    for (i32 n = 0; n < 128; n++) {
        cam_update(g, c);
    }
}

#define CAM_Y_NEW_BASE_MIN_SPEED  (1 << 7)
#define CAM_X_LOOKAHEAD_MIN_SPEED (1 << 7)
#define CAM_HERO_Y_BOT            172                   // lower = camera positioned lower
#define CAM_HERO_Y_TOP            (CAM_HERO_Y_BOT - 74) // tune for jump height
#define CAM_OFFS_Q8_TOP           ((-120 + CAM_HERO_Y_TOP) * 256)
#define CAM_OFFS_Q8_BOT           ((-120 + CAM_HERO_Y_BOT) * 256)

void cam_update(g_s *g, cam_s *c)
{
    obj_s    *hero          = obj_get_tagged(g, OBJ_TAG_HERO);
    v2_i32    ppos          = c->pos_q8;
    v2_i32    padd          = {0};
    const i32 lookdown_prev = c->lookdown;
    c->can_align_x          = 0;
    c->can_align_y          = 0;
    c->hero_align_x         = 0;
    c->hero_align_y         = 0;

    switch (c->mode) {
    case CAM_MODE_FOLLOW_HERO: {
        if (!hero) break;

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

            if (hero->v_q8.x == 0 && inp_action(INP_DD)) {
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

        i32 va = abs_i32(hero->v_q8.x);
        i32 vs = sgn_i32(hero->v_q8.x);

        if (vs == 0) {
            c->offs_x += hero->facing;
        } else {
            c->offs_x += vs * (-sgn_i32(c->offs_x) == vs ? 1 : 1);
        }

        if (c->offs_x <= -CAM_FACE_OFFS_X) {
            c->offs_x      = -CAM_FACE_OFFS_X;
            c->can_align_x = 1;
        }
        if (c->offs_x >= +CAM_FACE_OFFS_X) {
            c->offs_x      = +CAM_FACE_OFFS_X;
            c->can_align_x = 1;
        }

        c->pos_q8.x = herop.x << 8;

        // cam attractors
        v2_i32 pos_px    = v2_shr(c->pos_q8, 8);
        v2_i32 attract   = {0};
        i32    n_attract = 0;

        for (obj_each(g, o)) {
            if (!o->cam_attract_r) continue;

            v2_i32 pattr;
            if (o->ID == OBJ_ID_CAMATTRACTOR) {
                pattr = camattractor_static_closest_pt(o, pos_px);
            } else {
                pattr = obj_pos_center(o);
            }
            v2_i32 attr = v2_sub(pattr, pos_px);
            u32    ds   = pow2_u32(o->cam_attract_r);
            u32    ls   = v2_lensq(attr);
            if (ds <= ls) continue;

            attr.x  = (attr.x * (i32)(ds - ls)) / (i32)ds;
            attr.y  = (attr.y * (i32)(ds - ls)) / (i32)ds;
            attract = v2_add(attract, attr);

            n_attract++;
        }

        if (n_attract) {
            attract = v2_shl(attract, 8);
            attract.x /= n_attract;
            attract.y /= n_attract;
            c->attract.x = c->attract.x + (((attract.x - c->attract.x) * 3) >> 8);
            c->attract.y = c->attract.y + (((attract.y - c->attract.y) * 3) >> 8);
            c->attract   = v2_truncatel(c->attract, 10000);
        } else {
            c->attract.x = (c->attract.x * 250) >> 8;
            c->attract.y = (c->attract.y * 250) >> 8;
        }

        break;
    }
    }

    if (g->substate == SUBSTATE_TEXTBOX) {
        // c->textboxdown += 3;
    }

    if (0 < lookdown_prev && lookdown_prev == c->lookdown) {
        c->lookdown = min_i32(c->lookdown, 256);
        c->lookdown = max_i32(c->lookdown - 2, 0);
    }

    if (c->shake_ticks) {
        c->shake_ticks--;
        i32 shakes = (c->shake_str * c->shake_ticks) / c->shake_ticks_max;
        c->shake.x = rngr_sym_i32(shakes);
        c->shake.y = rngr_sym_i32(shakes);
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