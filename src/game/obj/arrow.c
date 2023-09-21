// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void arrow_think(game_s *g, obj_s *o);
static void arrow_on_delete(game_s *g, obj_s *o);

obj_s *arrow_create(game_s *g, v2_i32 p, v2_i32 v_q8)
{
        obj_s  *arrow = obj_create(g);
        flags64 flags = OBJ_FLAG_ACTOR |
                        OBJ_FLAG_MOVABLE |
                        OBJ_FLAG_THINK_2 |
                        OBJ_FLAG_KILL_OFFSCREEN |
                        OBJ_FLAG_HURTS_ENEMIES;
        obj_apply_flags(g, arrow, flags);
        arrow->think_2      = arrow_think;
        arrow->ID           = 2;
        arrow->w            = 8;
        arrow->h            = 8;
        arrow->pos.x        = p.x - arrow->w / 2;
        arrow->pos.y        = p.y - arrow->h / 2;
        arrow->facing       = sgn_i(v_q8.x);
        arrow->vel_q8       = v_q8;
        arrow->gravity_q8.y = 12;
        arrow->drag_q8.x    = 256;
        arrow->drag_q8.y    = 256;
        arrow->ondelete     = arrow_on_delete;
        // arrow->onsqueeze    = obj_squeeze_delete;
        return arrow;
}

static void arrow_think(game_s *g, obj_s *o)
{
        bool32 deleteme = 0;
        if (o->squeezed != 0) {
                deleteme = 1;
        } else {
                for (int i = 0; i < o->n_colliders; i++) {
                        obj_s *c = o->colliders[i];
                        if (c->ID == 13) {
                                deleteme = 1;
                                break;
                        }
                }
        }

        if (deleteme) {
                obj_delete(g, o);
                return;
        }

        if (o->vel_q8.x != 0) {
                o->facing = sgn_i(o->vel_q8.x);
        }
}

static void arrow_on_delete(game_s *g, obj_s *o)
{
}