// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

obj_s *blob_create(game_s *g)
{
        obj_s     *o     = obj_create(g);
        objflags_s flags = objflags_create(OBJ_FLAG_HURTABLE,
                                           OBJ_FLAG_ACTOR,
                                           OBJ_FLAG_MOVABLE,
                                           OBJ_FLAG_THINK_1,
                                           OBJ_FLAG_HURTS_PLAYER,
                                           OBJ_FLAG_ENEMY);
        obj_apply_flags(g, o, flags);
        o->pos.x        = 200;
        o->pos.y        = 100;
        o->w            = 20;
        o->h            = 20;
        o->think_1      = blob_think;
        o->gravity_q8.y = 30;
        o->drag_q8.x    = 256;
        o->drag_q8.y    = 256; // no drag
        o->ID           = 13;
        return o;
}

void blob_think(game_s *g, obj_s *o)
{
        static int groundticks = 0;
        bool32     grounded    = game_area_blocked(g, obj_rec_bottom(o));

        if (grounded) {
                o->vel_q8.x = 0;
                groundticks++;
                if (groundticks == 60) {
                        groundticks = 0;
                        o->vel_q8.y = -700;
                        o->vel_q8.x = rng_range(-400, +400);
                }
        }
}