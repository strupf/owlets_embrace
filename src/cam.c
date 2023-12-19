// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

v2_i32 cam_pos_px(cam_s *c)
{
    v2_i32 r = {(int)(c->pos.x + 0.5f), (int)(c->pos.y + 0.5f)};
    return r;
}

rec_i32 cam_rec_px(cam_s *c)
{
    v2_i32  p = cam_pos_px(c);
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
        v2_i32 herop = obj_pos_bottom_center(hero);
        v2_f32 trg   = {(f32)herop.x, (f32)herop.y - 20.f};

        i32 dtx = trg.x - c->pos.x;
        c->pos.x += (f32)dtx * 0.05f;

        int py_bot = herop.y - 50;
        int py_top = herop.y + 10;

        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);

        if (g->textbox.state != TEXTBOX_STATE_INACTIVE) {
            c->addticks = min_i(c->addticks + 1, 60);
        } else if (c->addticks > 0) {
            c->addticks--;
        }

        c->pos.y += (f32)c->addticks * 0.05f;
    }

    cam_constrain_to_room(g, c);
}

void cam_constrain_to_room(game_s *g, cam_s *c)
{
    v2_i32 p  = cam_pos_px(c);
    int    w2 = c->w / 2;
    int    h2 = c->h / 2;
    p.x       = clamp_i(p.x, w2, g->pixel_x - w2);
    p.y       = clamp_i(p.y, h2, g->pixel_y - h2);
    cam_set_pos_px(c, p.x, p.y);
}
