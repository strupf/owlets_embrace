// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "gamedef.h"

i32  water_tile_get(i32 x, i32 y, i32 tick);
void water_prerender_tiles();

#endif