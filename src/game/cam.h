// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H
#define CAM_H

#include "game_def.h"

enum {
        CAM_MODE_FOLLOW_HERO,
        CAM_MODE_TARGET,
};

struct cam_s {
        int    mode;
        v2_i32 pos; // center pos
        int    w;
        int    h;
        int    wh;
        int    hh;
        int    facetick;
        int    offsety_tick;
        v2_i32 target;
        bool32 lockedx;
        bool32 lockedy;
};

void cam_set_mode(cam_s *c, int mode);
void cam_constrain_to_room(game_s *g, cam_s *c);
void cam_update(game_s *g, cam_s *c);

#endif