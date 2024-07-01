// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FOREGROUND_PROP_H
#define FOREGROUND_PROP_H

#include "gamedef.h"

typedef struct {
    v2_i16   pos;
    texrec_s tr;
} foreground_prop_s;

#define NUM_FOREGROUND_PROPS 1024

void foreground_props_draw(game_s *g, v2_i32 cam);

#endif