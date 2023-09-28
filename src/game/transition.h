// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TRANSITION_H
#define TRANSITION_H

#include "cam.h"
#include "fading.h"
#include "game_def.h"

typedef struct {
        rec_i32 r;
        int     dir; // into which direction to slide
} transition_area_s;

struct transition_s {
        bool32 inprogress;
        char   map[64]; // next map to load
        v2_i32 teleportto;
        v2_i32 vel;
        int    dir_slide;
};

void   transition_update(game_s *g);
void   transition_start(game_s *g, char *filename, v2_i32 location, int dir_slide);
bool32 transition_active(game_s *g);

#endif