// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void savepoint_interact(game_s *g, obj_s *o);

obj_s *savepoint_create(game_s *g)
{
        obj_s *o      = obj_create(g);
        o->ID         = OBJ_ID_SAVEPOINT;
        flags64 flags = OBJ_FLAG_INTERACT;
        obj_apply_flags(g, o, flags);
        o->oninteract = savepoint_interact;
        PRINTF("savepoint created");
        return o;
}

static void savepoint_interact(game_s *g, obj_s *o)
{
        g->savepointID = o->tiledID;
        game_savefile_save(g);
}