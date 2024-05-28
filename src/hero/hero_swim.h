// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_SWIM_H
#define HERO_SWIM_H

#include "gamedef.h"

void   hero_update_swimming(game_s *g, obj_s *o);
bool32 hero_is_submerged(game_s *g, obj_s *o, i32 *water_depth);
i32    hero_breath_tick(game_s *g);
i32    hero_breath_tick_max(game_s *g);

#endif