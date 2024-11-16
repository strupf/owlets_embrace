// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gamedef.h"

#define NUM_LADDERS 16

typedef struct {
    u16 tx;        // position in tiles
    u16 ty;        //
    u8  th;        // height in tiles
    u8  t_touched; // tile index to be animated - "stepped on"
    u8  anim;      // animation timer
} ladder_s;