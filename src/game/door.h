// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DOOR_H
#define DOOR_H
#include "gamedef.h"

void   door_think(game_s *g, obj_s *o);
void   door_trigger(game_s *g, obj_s *o, int triggerID);
obj_s *door_create(game_s *g);
#endif