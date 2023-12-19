// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H
#define CAM_H

#include "gamedef.h"

enum {
    CAM_MODE_DIRECT,
    CAM_MODE_FOLLOW_HERO,
    CAM_MODE_PIN_TO_TARGET,
};

typedef struct {
    v2_f32 pos;
    int    w;
    int    h;
    int    mode;

    f32 addy;
    int addticks;
} cam_s;

v2_i32  cam_pos_px(cam_s *c);
rec_i32 cam_rec_px(cam_s *c);
void    cam_set_pos_px(cam_s *c, int x, int y);
void    cam_update(game_s *g, cam_s *c);
void    cam_constrain_to_room(game_s *g, cam_s *c);

#endif