// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_TB_TICKS 20
#define CAM_TB_Y     50

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center, int w, int h);

void cam_screenshake(cam_s *c, int ticks, int str)
{
    c->shake_ticks     = ticks;
    c->shake_ticks_max = ticks;
    c->shake_str       = str;
}

v2_i32 cam_pos_px(game_s *g, cam_s *c)
{
    v2_f32 p = c->pos;
    p        = v2f_add(p, c->offs_shake);
    p        = v2f_add(p, c->offs_textbox);
    v2_i32 v = {(i32)(p.x + .5f), (i32)(p.y + .5f)};
    v2_i32 r = cam_constrain_to_room(g, v, c->w, c->h);
    return r;
}

rec_i32 cam_rec_px(game_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px(g, c);
    rec_i32 r = {p.x - (c->w / 2),
                 p.y - (c->h / 2),
                 c->w,
                 c->h};
    return r;
}

void cam_set_pos_px(cam_s *c, int x, int y)
{
    c->pos.x = (f32)x;
    c->pos.y = (f32)y;
}

void cam_update(game_s *g, cam_s *c)
{
    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);

    if (c->mode == CAM_MODE_FOLLOW_HERO && hero) {
        v2_i32 herop  = obj_pos_bottom_center(hero);
        v2_f32 trg    = {(f32)herop.x, (f32)herop.y - 20.f};
        int    py_bot = herop.y - 80;
        int    py_top = herop.y - 30;

        c->pos.x += (f32)(trg.x - c->pos.x) * 0.05f;
        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);
    }

    if (g->textbox.state != TEXTBOX_STATE_INACTIVE)
        c->addticks = min_i(c->addticks + 1, CAM_TB_TICKS);
    else if (0 < c->addticks)
        c->addticks--;

    c->offs_textbox.y = (f32)ease_in_out_quad(CAM_TB_Y, 0,
                                              c->addticks, CAM_TB_TICKS);

    c->offs_shake.x = 0.f;
    c->offs_shake.y = 0.f;
    if (0 < c->shake_ticks) {
        int s = (c->shake_str * c->shake_ticks--) / c->shake_ticks_max;

        c->offs_shake.x = (f32)rngr_i32(-s, +s);
        c->offs_shake.y = (f32)rngr_i32(-s, +s);
    }
}

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center, int w, int h)
{
    int    w2 = w / 2;
    int    h2 = h / 2;
    v2_i32 v  = {clamp_i(p_center.x, w2, g->pixel_x - w2),
                 clamp_i(p_center.y, h2, g->pixel_y - h2)};
    return v;
}
