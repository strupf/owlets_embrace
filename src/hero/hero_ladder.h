// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_LADDER_H
#define HERO_LADDER_H

#include "gamedef.h"

void   hero_update_ladder(game_s *g, obj_s *o);
bool32 hero_try_snap_to_ladder(game_s *g, obj_s *o, i32 diry);
bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout);
bool32 hero_rec_on_ladder(game_s *g, rec_i32 aabb, rec_i32 *rout);

#endif