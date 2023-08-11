// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H
#define CAM_H
#include "gamedef.h"

struct cam_s {
        int    w;
        int    h;
        int    wh;
        int    hh;
        v2_i32 target;
        v2_i32 offset;
        v2_i32 pos;
};

void cam_update(game_s *g, cam_s *c);

#endif