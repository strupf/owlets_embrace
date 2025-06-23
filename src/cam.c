// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_USE_NEW_BEHAVIOUR 0
#define CAM_FACE_OFFS_X       54
#define CAM_HERO_Y_BOT        172                   // lower = camera positioned lower
#define CAM_HERO_Y_TOP        (CAM_HERO_Y_BOT - 70) // tune for jump height
#define CAM_OFFS_TOP          (-120 + CAM_HERO_Y_TOP)
#define CAM_OFFS_BOT          (-120 + CAM_HERO_Y_BOT)

// a, b: half width/height of ellipse, p considered relative to center
static v2_f32 v2f_truncate_to_ellipse(v2_f32 p, f32 a, f32 b);
static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center);
static v2_i32 cam_constrain_to_rec(v2_i32 p_center, rec_i32 r);

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
    v2_i32 pos_center = cam_pos_px_center(g, c);
    v2_i32 pos        = {pos_center.x - CAM_WH, pos_center.y - CAM_HH};
    return pos;
}

v2_i32 cam_pos_px_center(g_s *g, cam_s *c)
{
    v2_i32 pos = c->pos;
    pos.x += (i32)(c->attr_q12.x >> 8) + c->offs_x;
    pos.y += (i32)(c->attr_q12.y >> 8);

    if (0 < c->lookdownup_q8) {
        pos.y += ease_in_out_quad(0, 78, +c->lookdownup_q8, 256);
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
    if (c->clamp_rec_fade_q8) {
        v2_i32 p_clamped = cam_constrain_to_rec(pos, c->clamp_rec);
        pos              = v2_i32_ease(pos, p_clamped,
                                       c->clamp_rec_fade_q8, 256, ease_in_out_quad);
    }
    pos = v2_i32_add(pos, c->shake);
    return pos;
}

rec_i32 cam_rec_px(g_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px_top_left(g, c);
    rec_i32 r = {p.x, p.y, CAM_W, CAM_H};
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
    v2_i32 ppos         = c->pos;
    v2_i32 padd         = {0};
    obj_s *hero         = obj_get_hero(g);
    i32    look_up_down = 0;

    if (hero) {
        hero_s *h           = (hero_s *)hero->heap;
        c->can_align_x      = 0;
        c->can_align_y      = 0;
        v2_i32 herop        = obj_pos_bottom_center(hero);
        bool32 herogrounded = obj_grounded(g, hero);
        i32    hero_bot     = herop.y;

        if ((herogrounded && 0 <= hero->v_q12.y)) {
            // move camera upwards if hero landed on new platform
            // "new base height"
            i32 cam_y_bot = c->pos.y + CAM_OFFS_BOT; // align bottom with platform
            i32 diffy     = hero_bot - cam_y_bot;
            i32 y_add     = (diffy * 20) >> 8;
            ;
            c->pos.y += y_add;

            if (h->crouched) {
                look_up_down = +1;
            } else if (inp_btn(INP_DU) && !inp_x()) {
                look_up_down = -1;
            }
        }

        i32 cam_y_min = hero_bot - CAM_OFFS_BOT;
        i32 cam_y_max = hero_bot - CAM_OFFS_TOP;
        cam_y_max -= (i32)c->force_lower_ceiling;
        cam_y_min += (i32)c->force_higher_floor;
        if (c->pos.y <= cam_y_min) {
            c->pos.y       = cam_y_min;
            c->can_align_y = 1;
        }
        if (c->pos.y >= cam_y_max) {
            c->pos.y       = cam_y_max;
            c->can_align_y = 1;
        }

#if !CAM_USE_NEW_BEHAVIOUR
        c->pos.x = herop.x;
#endif

        // cam attractors
        v2_i32 pos_px        = c->pos;
        pos_px               = cam_constrain_to_room(g, pos_px);
        bool32 attract_found = 0;

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
            attract_found = 1;
            attr          = v2_i32_shl(attr, 8);

            i32 k0 = sqrt_i32(ds) - sqrt_i32(ls);
            i32 k1 = sqrt_i32(ds);

            c->attr_q12   = v2_i32_lerp((v2_i32){0}, attr, k0, k1);
            c->attr_q12.x = clamp_i32(c->attr_q12.x, -100 << 8, +100 << 8);
            c->attr_q12.y = clamp_i32(c->attr_q12.y, -60 << 8, +60 << 8);
        }

        if (!attract_found) {
            if (v2_i32_lensq(c->attr_q12) < 256) {
                c->attr_q12.x = 0;
                c->attr_q12.y = 0;
            } else {
                c->attr_q12 = v2_i32_lerp(c->attr_q12, (v2_i32){0}, 1, 8);
            }
        }

#if CAM_USE_NEW_BEHAVIOUR && 1
        // c->align = hero->facing;

        i32 camx = (c->pos_q8.x >> 8) + c->offs_x;
        i32 hx   = herop.x;
        i32 ddd  = camx - hx;

        switch (c->align) {
        case +0:
        case +1: { // looks to right
            if (hero->facing == +1 || 1) {
                c->offs_x = min_i32(c->offs_x + 2, +60);
                if (0 < hero->v_q12.x) {
                    c->offs_x = min_i32(c->offs_x + 1, +60);
                }
            } else {
                c->pos_q8.x = herop.x << 8;
                c->offs_x   = camx - herop.x;
            }
            if (c->offs_x == +60) {
                c->can_align_x = 1;
            }
            break;
        }
        case -1: { // looks to left
            if (hero->facing == -1 || 1) {
                c->offs_x = max_i32(c->offs_x - 2, -60);
                if (0 > hero->v_q12.x) {
                    c->offs_x = max_i32(c->offs_x - 1, -60);
                }
            } else {
                c->pos_q8.x = herop.x << 8;
                c->offs_x   = camx - herop.x;
            }
            if (c->offs_x == -60) {
                c->can_align_x = 1;
            }
            break;
        }
        }

        switch (c->align) {
        case +0:
        case +1: { // looks to right
            if (ddd > +84) {
                c->align    = -1;
                c->offs_x   = ddd;
                c->pos_q8.x = herop.x << 8;
            } else if (ddd <= 60) {
                c->pos_q8.x = herop.x << 8;
            }
            break;
        }
        case -1: { // looks to right
            if (ddd < -84) {
                c->align    = +1;
                c->offs_x   = ddd;
                c->pos_q8.x = herop.x << 8;
            } else if (ddd >= -60) {
                c->pos_q8.x = herop.x << 8;
            }
            break;
        }
        }

#else
        c->offs_x += hero->facing * 2 + sgn_i32(hero->v_q12.x);
        c->offs_x      = clamp_sym_i32(c->offs_x, CAM_FACE_OFFS_X);
        c->can_align_x = abs_i32(c->offs_x) == CAM_FACE_OFFS_X &&
                         c->attr_q12.x == 0;
#endif
    }

    if (c->has_clamp_rec) {
        c->clamp_rec_fade_q8 = min_i32(c->clamp_rec_fade_q8 + 4, 256);
    } else {
        c->clamp_rec_fade_q8 = max_i32(c->clamp_rec_fade_q8 - 4, 0);
    }

    c->force_lower_ceiling = 0;
    c->force_higher_floor  = 0;

    if (look_up_down) {
        if (1 <= c->lookdownup_tick) {
            c->lookdownup_q8 += 0 < look_up_down ? 8 : -6;
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
        c->pos.x = ppos.x;
    }
    if (c->locked_y) {
        c->pos.y = ppos.y;
    }
}

static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center)
{
    rec_i32 r = {0, 0, g->pixel_x, g->pixel_y};
    return cam_constrain_to_rec(p_center, r);
}

static v2_i32 cam_constrain_to_rec(v2_i32 p_center, rec_i32 r)
{
    v2_i32 v = {clamp_i32(p_center.x, r.x + CAM_WH, r.x + r.w - CAM_WH),
                clamp_i32(p_center.y, r.y + CAM_HH, r.y + r.h - CAM_HH)};
    return v;
}

v2_i32 cam_offset_max(g_s *g, cam_s *c)
{
    v2_i32 o = {g->pixel_x - CAM_W,
                g->pixel_y - CAM_H};
    return o;
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