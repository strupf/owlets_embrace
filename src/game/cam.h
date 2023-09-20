// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H
#define CAM_H

#include "game_def.h"

struct cam_s {
        v2_i32 pos; // center pos
        int    w;
        int    h;
        int    wh;
        int    hh;
        int    facetick;
        v2_i32 target;
        bool32 lockedx;
        bool32 lockedy;
};

void cam_constrain_to_room(game_s *g, cam_s *c);
void cam_update(game_s *g, cam_s *c);

#endif