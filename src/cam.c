// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_TB_TICKS 20
#define CAM_TB_Y     50

#define CAM_W  SYS_DISPLAY_W
#define CAM_H  SYS_DISPLAY_H
#define CAM_WH (SYS_DISPLAY_W >> 1)
#define CAM_HH (SYS_DISPLAY_H >> 1)

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center);

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
    v2_i32 r = cam_constrain_to_room(g, v);
    return r;
}

rec_i32 cam_rec_px(game_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px(g, c);
    rec_i32 r = {p.x - CAM_WH, p.y - CAM_HH, CAM_W, CAM_H};

    // avoid dither flickering? -> snap camera pos
    if (sys_reduced_flicker()) {
        r.x &= ~1;
        r.y &= ~1;
    }
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
        v2_f32 trg    = {(f32)herop.x, (f32)herop.y - 0.f};
        int    py_bot = herop.y - 55;
        int    py_top = herop.y + 20;

        c->pos.x += (f32)(trg.x - c->pos.x) * .05f;
        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);
    }

    if (g->textbox.state != TEXTBOX_STATE_INACTIVE)
        c->addticks = min_i(c->addticks + 1, CAM_TB_TICKS);
    else if (0 < c->addticks)
        c->addticks--;

    c->offs_textbox.y = (f32)ease_in_out_quad(0, CAM_TB_Y, c->addticks, CAM_TB_TICKS);
    c->offs_shake.x   = 0.f;
    c->offs_shake.y   = 0.f;
    if (0 < c->shake_ticks) {
        int s = (c->shake_str * c->shake_ticks--) / c->shake_ticks_max;

        c->offs_shake.x = (f32)rngr_sym_i32(s);
        c->offs_shake.y = (f32)rngr_sym_i32(s);
    }
}

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center)
{
    v2_i32 v = {clamp_i(p_center.x, CAM_WH, g->pixel_x - CAM_WH),
                clamp_i(p_center.y, CAM_HH, g->pixel_y - CAM_HH)};
    return v;
}
