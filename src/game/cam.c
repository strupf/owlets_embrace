// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"
#include "obj.h"

enum cam_values {
        CAM_TARGET_SNAP_THRESHOLD = 1,
        CAM_LERP_DISTANCESQ_FAST  = 1000,
        CAM_LERP_DEN              = 8,
        CAM_LERP_DEN_FAST         = 4,
};

void cam_constrain_to_room(game_s *g, cam_s *c)
{
        int x1 = c->pos.x - c->wh;
        int y1 = c->pos.y - c->hh;
        if (x1 < 0) {
                c->pos.x = c->wh;
        }
        if (y1 < 0) {
                c->pos.y = c->hh;
        }

        // avoids round errors on uneven camera sizes
        int x2 = (c->pos.x - c->wh) + c->w;
        int y2 = (c->pos.y - c->hh) + c->h;
        if (x2 > g->pixel_x) {
                c->pos.x = g->pixel_x - c->w + c->wh;
        }
        if (y2 > g->pixel_y) {
                c->pos.y = g->pixel_y - c->h + c->hh;
        }
}

void cam_update(game_s *g, cam_s *c)
{
        obj_s *player;
        bool32 offset_modified = 0;
        if (try_obj_from_handle(g->hero.obj, &player)) {
                v2_i32 targ = obj_aabb_center(player);
                targ.y -= c->h >> 3; // offset camera slightly upwards
                c->target = targ;

                if (os_inp_dpad_y() == 1 &&
                    game_area_blocked(g, obj_rec_bottom(player)) &&
                    ABS(player->vel_q8.x) < 10) {
                        offset_modified = 1;
                        c->offset.y += 5;
                        c->offset.y = MIN(c->offset.y, 100);
                }
        }

        v2_i32 target = v2_add(c->target, c->offset);
        if (!offset_modified) c->offset.y >>= 1;
        v2_i32 dt  = v2_sub(target, c->pos);
        i32    lsq = v2_lensq(dt);
        if (lsq <= CAM_TARGET_SNAP_THRESHOLD) {
                c->pos = target;
        } else if (lsq < CAM_LERP_DISTANCESQ_FAST) {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN);
        } else {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN_FAST);
        }
        cam_constrain_to_room(g, c);
}