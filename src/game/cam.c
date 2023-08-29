// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"
#include "obj/obj.h"

enum cam_values {
        CAM_LERP_DISTANCESQ_FAST = 1000,
        CAM_LERP_DEN             = 8,
        CAM_LERP_DEN_FAST        = 4,
        CAM_ATTRACT_DIST         = 300,
        CAM_ATTRACT_DISTSQ       = CAM_ATTRACT_DIST * CAM_ATTRACT_DIST,
        CAM_ATTRACT_FACTOR       = 3,
        CAM_TEXTBOX_Y_OFFSET     = 80,
        CAM_LOOK_DOWN_OFFSET     = 80,
};

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

static void cam_attractors(game_s *g, cam_s *c)
{
        // gravitate towards points of interest
        v2_i32      attract    = {0};
        i32         nattract   = 0;
        obj_listc_s attractors = objbucket_list(g, OBJ_BUCKET_CAM_ATTRACTOR);
        for (int n = 0; n < attractors.n; n++) {
                v2_i32 ca   = attractors.o[n]->pos;
                i32    dist = v2_distancesq(ca, c->pos);
                if (dist < CAM_ATTRACT_DISTSQ) {
                        i32 weight = CAM_ATTRACT_DISTSQ - dist;
                        attract    = v2_add(attract, v2_mul(ca, weight));
                        nattract += weight;
                }
        }

        if (nattract == 0) return;

        attract   = v2_div(attract, nattract);
        i32 dist  = v2_distancesq(attract, c->pos);
        i32 num   = CAM_ATTRACT_DISTSQ - dist;
        i32 den   = CAM_ATTRACT_DISTSQ * CAM_ATTRACT_FACTOR;
        c->target = v2_lerp(c->target, attract, num, den);
}

static void cam_player_input(game_s *g, cam_s *c)
{
        obj_s *player;
        if (!try_obj_from_handle(g->hero.obj, &player)) return;

        c->target = obj_aabb_center(player);
        c->target.y -= c->h >> 3; // offset camera slightly upwards
        c->target.x += CLAMP(g->hero.facingticks, -25, +25);

        if (g->textbox.active) {
                c->target.y += CAM_TEXTBOX_Y_OFFSET;
        } else if (os_inp_dpad_y() == 1 &&
                   game_area_blocked(g, obj_rec_bottom(player)) &&
                   ABS(player->vel_q8.x) < 10) {
                c->target.y += CAM_LOOK_DOWN_OFFSET;
        }
}

void cam_update(game_s *g, cam_s *c)
{
        cam_player_input(g, c);
        cam_attractors(g, c);

        i32 lsq = v2_distancesq(c->pos, c->target);
        if (lsq < CAM_LERP_DISTANCESQ_FAST) {
                c->pos = v2_lerp(c->pos, c->target, 1, CAM_LERP_DEN);
        } else {
                c->pos = v2_lerp(c->pos, c->target, 1, CAM_LERP_DEN_FAST);
        }
        cam_constrain_to_room(g, c);
}