// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H_
#define CAM_H_

#include "gamedef.h"

#define CAM_TRG_FADE_MAX 4096

typedef struct cam_s {
    v2_i32 prev_gfx_offs;
    v2_i32 pos_q8;
    v2_i32 shake;
    v2_f32 attract;
    v2_i32 trg;
    v2_i32 hero_off;
    i32    lookdown;
    u16    shake_ticks;
    u16    shake_ticks_max;
    u16    shake_str_x;
    u16    shake_str_y;
    u16    shake_str_x2;
    u16    shake_str_y2;
    i16    offs_x;
    u16    trg_fade_q12; // [0, 4096]
    b8     locked_x;
    b8     locked_y;
    b8     can_align_x;
    b8     can_align_y;
    b8     has_trg;
    u8     trg_fade_spd; // speed to fade (per tick)
} cam_s;

// either lock to position or to obj if non-null
void    cam_screenshake(cam_s *c, i32 ticks, i32 str);
void    cam_screenshake_xy(cam_s *c, i32 ticks, i32 str_x, i32 str_y);
v2_i32  cam_pos_px_top_left(g_s *g, cam_s *c);
v2_i32  cam_pos_px_center(g_s *g, cam_s *c);
rec_i32 cam_rec_px(g_s *g, cam_s *c);
void    cam_init_level(g_s *g, cam_s *c);
void    cam_update(g_s *g, cam_s *c);
v2_i32  cam_offset_max(g_s *g, cam_s *c);
f32     cam_snd_scale(g_s *g, v2_i32 p, u32 dst_max);

#endif