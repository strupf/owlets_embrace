// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H_
#define CAM_H_

#include "gamedef.h"

#define CAM_TRG_FADE_MAX 4096
#define CAM_W            PLTF_DISPLAY_W
#define CAM_H            PLTF_DISPLAY_H
#define CAM_WH           (PLTF_DISPLAY_W >> 1)
#define CAM_HH           (PLTF_DISPLAY_H >> 1)
#define CAM_X_PAN_V_Q2   (3 << 3)

typedef struct cam_owl_s {
    ALIGNAS(32)
    v2_i32 pos; // in pixels
    b16    was_grounded;
    i16    was_ground_y;
    i16    lookdownup_tick;
    i16    lookdownup_q8;
    i8     x_pan_v;             // left/right pan movement
    i8     offs_x;              // offset from owl position
    u8     force_lower_ceiling; // push camera up earlier; resets every frame
    u8     force_higher_floor;  // push camera down earlier; resets every frame
    b8     can_align_x;         // align owl to multiple of 2px?
    b8     can_align_y;         // align owl to multiple of 2px?
    u8     touched_top_tick;
} cam_owl_s;

typedef struct cam_s {
    v2_i32    prev_gfx_offs;
    cam_owl_s cowl;
    v2_i32    shake;
    v2_i32    trg;
    v2_i32    attr_q12;
    rec_i32   clamp_rec;
    b32       has_clamp_rec;
    i32       clamp_rec_fade_q8;
    u16       shake_ticks;
    u16       shake_ticks_max;
    u16       shake_str_x;
    u16       shake_str_y;
    u16       shake_str_x2;
    u16       shake_str_y2;
    u16       trg_fade_q12; // [0, 4096]
    b8        locked_x;
    b8        locked_y;
    b8        has_trg;
    u8        trg_fade_spd; // speed to fade (per tick)
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
void    cam_owl_do_x_shift(cam_s *c, i32 sx);

#endif