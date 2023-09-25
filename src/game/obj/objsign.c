// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void sign_interact(game_s *g, obj_s *o);

obj_s *sign_create(game_s *g)
{
        obj_s  *o     = obj_create(g);
        flags64 flags = OBJ_FLAG_INTERACT;
        obj_apply_flags(g, o, flags);
        o->oninteract = sign_interact;
        return o;
}

static void sign_interact(game_s *g, obj_s *o)
{
}