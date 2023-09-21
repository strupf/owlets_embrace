// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void npc_think(game_s *g, obj_s *o);
static void npc_interact(game_s *g, obj_s *o);

obj_s *npc_create(game_s *g)
{
        obj_s  *o     = obj_create(g);
        flags64 flags = OBJ_FLAG_ACTOR |
                        OBJ_FLAG_INTERACT |
                        OBJ_FLAG_THINK_1;
        obj_apply_flags(g, o, flags);
        o->think_1      = npc_think;
        o->w            = 16;
        o->h            = 32;
        o->oninteract   = npc_interact;
        o->drag_q8.x    = 0;
        o->drag_q8.y    = 255;
        o->gravity_q8.y = 30;
        o->actorflags   = ACTOR_FLAG_CLIMB_SLOPES;
        return o;
}

static void npc_think(game_s *g, obj_s *o)
{
}

static void npc_interact(game_s *g, obj_s *o)
{
}