// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"
#include "obj/obj.h"

enum cam_values {
        CAM_TARGET_SNAP_THRESHOLD = 1,
        CAM_LERP_DISTANCESQ_FAST  = 1000,
        CAM_LERP_DEN              = 8,
        CAM_LERP_DEN_FAST         = 4,
        CAM_ATTRACT_DIST          = 300,
        CAM_ATTRACT_DISTSQ        = CAM_ATTRACT_DIST * CAM_ATTRACT_DIST,
        CAM_ATTRACT_FACTOR        = 3,
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
        c->target = v2_lerp(c->target, attract,
                            CAM_ATTRACT_DISTSQ - dist,
                            CAM_ATTRACT_DISTSQ * CAM_ATTRACT_FACTOR);
}

void cam_update(game_s *g, cam_s *c)
{
        obj_s *player;
        bool32 offset_modified = 0;
        if (try_obj_from_handle(g->hero.obj, &player)) {
                c->target = obj_aabb_center(player);
                c->target.y -= c->h >> 3; // offset camera slightly upwards

                if (player->facing == +1 && c->offset.x < +25) {
                        c->offset.x += c->offset.x < 0 ? 2 : 1;
                } else if (player->facing == -1 && c->offset.x > -25) {
                        c->offset.x -= c->offset.x > 0 ? 2 : 1;
                }

                if (os_inp_dpad_y() == 1 &&
                    game_area_blocked(g, obj_rec_bottom(player)) &&
                    ABS(player->vel_q8.x) < 10) {
                        offset_modified = 1;
                        c->offset.y += 5;
                        c->offset.y = MIN(c->offset.y, 100);
                }
        }

        if (!offset_modified)
                c->offset.y >>= 1;

        cam_attractors(g, c);

        v2_i32 target = v2_add(c->target, c->offset);
        v2_i32 dt     = v2_sub(target, c->pos);
        i32    lsq    = v2_lensq(dt);
        if (lsq <= CAM_TARGET_SNAP_THRESHOLD) {
                c->pos = target;
        } else if (lsq < CAM_LERP_DISTANCESQ_FAST) {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN);
        } else {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN_FAST);
        }
        cam_constrain_to_room(g, c);
}