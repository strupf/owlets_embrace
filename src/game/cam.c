// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"
#include "obj.h"

enum {
        CAM_LERP_DISTANCESQ_FAST = 1500,
        CAM_LERP_DEN             = 6,
        CAM_LERP_DEN_FAST        = 3,
        CAM_ATTRACT_DIST         = 300,
        CAM_ATTRACT_DISTSQ       = CAM_ATTRACT_DIST * CAM_ATTRACT_DIST,
        CAM_ATTRACT_FACTOR       = 3,
        CAM_TEXTBOX_Y_OFFSET     = 80,
        CAM_LOOK_DOWN_OFFSET     = 85,
};

void cam_set_mode(cam_s *c, int mode)
{
        c->mode = mode;
}

void cam_constrain_to_room(game_s *g, cam_s *c)
{
        int x1 = c->pos.x - c->wh;
        int y1 = c->pos.y - c->hh;
        if (x1 < 0)
                c->pos.x = c->wh;
        if (y1 < 0)
                c->pos.y = c->hh;

        // avoids round errors on uneven camera sizes
        int x2 = (c->pos.x - c->wh) + c->w;
        int y2 = (c->pos.y - c->hh) + c->h;
        if (x2 > g->pixel_x)
                c->pos.x = g->pixel_x - c->w + c->wh;
        if (y2 > g->pixel_y)
                c->pos.y = g->pixel_y - c->h + c->hh;
}

static void cam_player_input(game_s *g, cam_s *c)
{
        obj_s *player = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!player) return;

        c->facetick = clamp_i(c->facetick + player->facing, -64, +64);

        int targetx = obj_aabb_center(player).x;
        targetx += (c->facetick >> 1);
        c->pos.x = c->pos.x + divr_i32(targetx - c->pos.x, 6);

        /*
        if (textbox_blocking(&g->textbox) && g->textbox.type == TEXTBOX_TYPE_STATIC_BOX) {
                c->offsety_tick = lerp_i32(c->offsety_tick, 50, 1, 4);
                // c->offsety_tick += 4;
                c->offsety_tick = min_i(c->offsety_tick, 50);
        } else if (os_inp_pressed(INP_DOWN) && room_area_blocked(g, obj_rec_bottom(player)) && player->vel_q8.x == 0) {
                // c->offsety_tick += c->offsety_tick < 60 ? 12 : 4;
                c->offsety_tick = lerp_i32(c->offsety_tick, 100, 1, 4);
                c->offsety_tick = min_i(c->offsety_tick, 100);
        } else if (c->offsety_tick > 0) {
                c->offsety_tick = (c->offsety_tick * 210) >> 8;
        }
        */

        int player_bot = player->pos.y + player->h;
        int py_bot     = player_bot - 40;
        int py_top     = player_bot + 20;

        c->pos.y = clamp_i(c->pos.y, py_bot, py_top);
}

void cam_update(game_s *g, cam_s *c)
{
        v2_i32 ppos = c->pos;

        switch (c->mode) {
        case CAM_MODE_FOLLOW_HERO:
                cam_player_input(g, c);
                break;
        case CAM_MODE_TARGET:
                u32 dsq = v2_distancesq(c->target, c->pos);
                int den = dsq < CAM_LERP_DISTANCESQ_FAST ? 2 : 4;

                c->pos.x = c->pos.x + lerp_i32(c->target.x, c->pos.x, 1, den);
                c->pos.y = c->pos.y + lerp_i32(c->target.y, c->pos.y, 1, den);
                break;
        }

        if (c->lockedx) c->pos.x = ppos.x;
        if (c->lockedy) c->pos.y = ppos.y;
        cam_constrain_to_room(g, c);
}