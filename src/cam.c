// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

// shifts on points of interests
#define CAM_ATTRACT_Y_NEG 20 // max pixels up
#define CAM_ATTRACT_Y_POS 50 // max pixels down
#define CAM_ATTRACT_X     50 // max pixels left and right
#define CAM_FACE_OFFS_X   80
#define CAM_HERO_Y_BOT    166                   // lower = camera positioned lower
#define CAM_HERO_Y_TOP    (CAM_HERO_Y_BOT - 72) // tune for jump height
#define CAM_OFFS_TOP      (-120 + CAM_HERO_Y_TOP)
#define CAM_OFFS_BOT      (-120 + CAM_HERO_Y_BOT)

// a, b: half width/height of ellipse, p considered relative to center
static v2_f32 v2f_truncate_to_ellipse(v2_f32 p, f32 a, f32 b);
static v2_i32 cam_constrain_to_room(g_s *g, v2_i32 p_center);
static v2_i32 cam_constrain_to_rec(v2_i32 p_center, rec_i32 r);
void          cam_update_owl(g_s *g, cam_owl_s *c, obj_s *o);

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

v2_i32 cam_pos_px_center_hero(cam_owl_s *ch)
{
    v2_i32 p = ch->pos;
    p.x += ch->offs_x;
    if (0 < ch->lookdownup_q8) {
        p.y += ease_in_out_quad(0, 78, +ch->lookdownup_q8, 256);
    } else {
        p.y -= ease_in_out_quad(0, 60, -ch->lookdownup_q8, 256);
    }
#if 0
    if (ch->can_align_x) {
        p.x++;
        p.x &= ~1;
    }
    if (ch->can_align_y) {
        p.y &= ~1;
    }
#else
    p.x++;
#endif
    return p;
}

v2_i32 cam_pos_px_center(g_s *g, cam_s *c)
{
    v2_i32 phero = cam_pos_px_center_hero(&c->cowl);
    v2_i32 pos   = phero;

    if (c->trg_fade_q12) {
        pos.x = ease_in_out_sine(pos.x, c->trg.x, c->trg_fade_q12, CAM_TRG_FADE_MAX);
        pos.y = ease_in_out_sine(pos.y, c->trg.y, c->trg_fade_q12, CAM_TRG_FADE_MAX);
    }

    pos = cam_constrain_to_room(g, pos);
    if (c->clamp_rec_fade_q8) {
        v2_i32 p_clamped = cam_constrain_to_rec(pos, c->clamp_rec);
        pos              = v2_i32_ease(pos, p_clamped, c->clamp_rec_fade_q8, 256, ease_in_out_quad);
    }
    pos.x += c->attr_q12.x >> 12;
    pos.y += c->attr_q12.y >> 12;
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
    obj_s *owl = obj_get_owl(g);

    if (owl) {
        cam_update_owl(g, &c->cowl, owl);
    }

    if (c->has_clamp_rec) {
        c->clamp_rec_fade_q8 = min_i32(c->clamp_rec_fade_q8 + 4, 256);
    } else {
        c->clamp_rec_fade_q8 = max_i32(c->clamp_rec_fade_q8 - 4, 0);
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
        c->trg_fade_q12 = min_i32(c->trg_fade_q12 + c->trg_fade_spd, CAM_TRG_FADE_MAX);
    } else {
        c->trg_fade_q12 = max_i32(c->trg_fade_q12 - c->trg_fade_spd, 0);
    }

    v2_i32 pc0     = cam_pos_px_center(g, c);
    v2_i32 pc0_q12 = v2_i32_shl(pc0, 12);
    v2_i32 pc_q12  = v2_i32_add(pc0_q12, c->attr_q12);

    i32    n_attr = 0;
    v2_i32 pattr  = {0};
    for (obj_each(g, o)) {
        if (o->cam_attract_r == 0) continue;

        v2_i32 p1 = (o->ID == OBJID_CAMATTRACTOR ? camattractor_closest_pt(o, pc0)
                                                 : obj_pos_center(o));

        if (v2_i32_distance_appr(p1, pc0) > o->cam_attract_r) continue;

        v2_i32 dt = v2_i32_sub(v2_i32_shl(p1, 12), pc_q12);
        n_attr++;
        pattr = v2_i32_add(pattr, dt);
    }

    if (n_attr) {
        pattr.x /= n_attr;
        pattr.y /= n_attr;
        pattr.x   = clamp_sym_i32(pattr.x, CAM_ATTRACT_X << 12);
        pattr.y   = clamp_i32(pattr.y, -(CAM_ATTRACT_Y_NEG << 12), +(CAM_ATTRACT_Y_POS << 12));
        v2_i32 pp = {pc_q12.x + ((pattr.x * 4) >> 8),
                     pc_q12.y + ((pattr.y * 4) >> 8)};

        c->attr_q12   = v2_i32_sub(pp, pc0_q12);
        c->attr_q12.x = clamp_sym_i32(c->attr_q12.x, CAM_ATTRACT_X << 12);
        c->attr_q12.y = clamp_i32(c->attr_q12.y, -(CAM_ATTRACT_Y_NEG << 12), +(CAM_ATTRACT_Y_POS << 12));
    } else {
        c->attr_q12.x = (c->attr_q12.x * 248) >> 8;
        c->attr_q12.y = (c->attr_q12.y * 248) >> 8;
    }
}

void cam_update_owl(g_s *g, cam_owl_s *c, obj_s *o)
{
    i32    look_up_down = 0;
    owl_s *h            = (owl_s *)o->heap;
    c->can_align_x      = 0;
    c->can_align_y      = 0;
    v2_i32 owlp         = obj_pos_bottom_center(o);
    bool32 herogrounded = 0 <= o->v_q12.y && obj_grounded(g, o);
    i32    hero_bot     = owlp.y;

    // hard move camera_y if standing on a moving platform
    if (c->was_grounded && herogrounded) {
        c->pos.y += hero_bot - c->was_ground_y;
    }
    c->was_grounded = herogrounded;
    c->was_ground_y = hero_bot;

    if (c->touched_top_tick && c->touched_top_tick < 80) {
        if (c->touched_top_tick < 64) {
            c->touched_top_tick += 2;
        } else {
            c->touched_top_tick += 1;
        }
    }

    if ((herogrounded)) {
        // move camera upwards if hero landed on new platform
        // "new base height"
        // align bottom with platform
        c->touched_top_tick = 0;
        i32 diffy           = hero_bot - CAM_OFFS_BOT - c->pos.y;
        i32 y_add           = (diffy * 20) >> 8;

        if (abs_i32(diffy) < 4 && 0) {
            c->pos.y = hero_bot - CAM_OFFS_BOT;
        } else {
            c->pos.y += y_add;
        }

        if (h->ground_crouch) {
            look_up_down = +1;
        } else if (inp_btn(INP_DU) && !inp_x()) {
            look_up_down = -1;
        }
    }

    i32 cam_y_min = hero_bot - CAM_OFFS_BOT;
    i32 cam_y_max = hero_bot - CAM_OFFS_TOP - (i32)(c->touched_top_tick >> 1);
    cam_y_max -= (i32)c->force_lower_ceiling & ~1;
    cam_y_min += (i32)c->force_higher_floor & ~1;
    if (c->pos.y <= cam_y_min) {
        c->pos.y       = cam_y_min;
        c->can_align_y = 1;
    }
    if (c->pos.y >= cam_y_max) {
        c->touched_top_tick = max_i32(c->touched_top_tick, 2);
        c->pos.y            = cam_y_max;
        c->can_align_y      = 1;
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

    // new x axis code
    if (h->climb == OWL_CLIMB_LADDER && h->climb_tick) {
        if (0) {
        } else if (c->offs_x < 0) {
            c->offs_x = min_i32(c->offs_x + 2, 0);
        } else if (c->offs_x > 0) {
            c->offs_x = max_i32(c->offs_x - 2, 0);
        }
        c->x_pan_v = 0;
    } else {
        // normal camera movement on x
        i32 offs_x_cur = c->pos.x - owlp.x + c->offs_x; // offset considering the new owl position and the old camera position
        c->pos.x       = owlp.x;

        if (0) {
        } else if (offs_x_cur < -CAM_FACE_OFFS_X) {
            c->offs_x  = max_i32(c->offs_x, -CAM_FACE_OFFS_X);
            c->x_pan_v = +3;
        } else if (offs_x_cur > +CAM_FACE_OFFS_X) {
            c->offs_x  = min_i32(c->offs_x, +CAM_FACE_OFFS_X);
            c->x_pan_v = -3;
        }

        if (o->facing != sgn_i32(c->x_pan_v)) {
            c->x_pan_v = 0;
        }

        if (c->x_pan_v) {
            i32 v = abs_i32(sgn_i32(c->x_pan_v) * CAM_FACE_OFFS_X - c->offs_x);
            c->offs_x += v < 10 ? sgn_i32(c->x_pan_v) : c->x_pan_v; // slow down before end of panning
        } else {
            c->offs_x = offs_x_cur;
        }
    }

    c->offs_x      = clamp_sym_i32(c->offs_x, CAM_FACE_OFFS_X);
    c->can_align_x = abs_i32(c->offs_x) == CAM_FACE_OFFS_X;
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
    v2_i32 o = {g->pixel_x - CAM_W, g->pixel_y - CAM_H};
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

void cam_owl_do_x_shift(cam_s *c, i32 sx)
{
    c->cowl.x_pan_v = sgn_i32(sx) * 3;
}