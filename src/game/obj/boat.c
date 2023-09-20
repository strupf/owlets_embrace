// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"
#include "game/water.h"

void boat_think(game_s *g, obj_s *o)
{
        /*
        static int boatticks = 0;

        v2_i32 cc = obj_aabb_center(o);
        int    cy = cc.y + 10;
        int    yy = ocean_base_amplitude(&g->ocean, cc.x >> 2);

        obj_s *ohero;
        if (try_obj_from_handle(g->hero.obj, &ohero) &&
            overlap_rec_excl(obj_rec_bottom(ohero), obj_aabb(o))) {
                boatticks += 12;

                o->vel_q8.x  = boatticks;
                o->drag_q8.x = 256;
        } else {
                boatticks -= 1;
                o->drag_q8.x = min_i(boatticks, 254);
        }

        boatticks = clamp_i(boatticks, 0, 400);

        if (cy > yy) {
                int dt = (cy - yy);
                int vv = max_i((dt * dt) >> 2, 2);
                o->vel_q8.y -= vv;
                o->drag_q8.y = (dt < 8 ? 222 : 192);
        } else {
                o->drag_q8.y = 256;
        }
        */
}

obj_s *boat_create(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_set_flags(g, o,
                      OBJ_FLAG_SOLID |
                          OBJ_FLAG_MOVABLE |
                          OBJ_FLAG_SOLID |
                          OBJ_FLAG_THINK_1);
        o->think_1      = boat_think;
        o->pos.x        = 100;
        o->pos.y        = 800 + 8;
        o->w            = 100;
        o->h            = 20;
        o->ID           = 10;
        o->gravity_q8.y = 20;
        return o;
}