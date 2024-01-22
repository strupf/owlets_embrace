// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H_
#define CAM_H_

#include "gamedef.h"

enum {
    CAM_MODE_DIRECT,
    CAM_MODE_FOLLOW_HERO,
    CAM_MODE_PIN_TO_TARGET,
};

typedef struct {
    v2_f32 pos;
    v2_f32 offs_shake;
    v2_f32 offs_textbox;
    int    mode;

    int shake_ticks;
    int shake_ticks_max;
    int shake_str;

    f32 addy;
    int addticks;

    bool32 locked_x;
    bool32 locked_y;
} cam_s;

void    cam_screenshake(cam_s *c, int ticks, int str);
v2_i32  cam_pos_px(game_s *g, cam_s *c);
rec_i32 cam_rec_px(game_s *g, cam_s *c);
void    cam_set_pos_px(cam_s *c, int x, int y);
void    cam_update(game_s *g, cam_s *c);

#endif