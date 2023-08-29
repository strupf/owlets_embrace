// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef NPC_H
#define NPC_H

#include "game/gamedef.h"

obj_s *npc_create(game_s *g);
void   npc_think(game_s *g, obj_s *o, void *arg);
void   npc_interact(game_s *g, obj_s *o, void *arg);

#endif