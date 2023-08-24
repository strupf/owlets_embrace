// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BLOB_H
#define BLOB_H

#include "game/gamedef.h"

obj_s *blob_create(game_s *g);
void   blob_think(game_s *g, obj_s *o, void *arg);

#endif