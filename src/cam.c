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

void cam_set_trg_px(cam_s *c, int x, int y)
{
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

        v2_f32 dt     = v2f_sub(trg, c->pos);
        f32    distsq = v2f_lensq(dt);
        f32    toadd  = distsq * 0.0005f;

        if (toadd >= 1.f) {
            dt     = v2f_setlen(dt, toadd);
            c->pos = v2f_add(c->pos, dt);
        }

        int py_bot = herop.y - 50;
        int py_top = herop.y + 40;

        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);
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
