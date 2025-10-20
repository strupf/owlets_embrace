// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CAM_H_
#define CAM_H_

#include "gamedef.h"

#define CAM_TRG_FADE_MAX    4096
#define CAM_W               PLTF_DISPLAY_W
#define CAM_H               PLTF_DISPLAY_H
#define CAM_WH              (PLTF_DISPLAY_W >> 1)
#define CAM_HH              (PLTF_DISPLAY_H >> 1)
#define CAM_CLAMP_REC_TICKS 50

// data for the camera around the owl's behaviour
typedef struct cam_owl_s {
    ALIGNAS(32)
    v2_i32 pos; // in pixels
    i16    lookdownup_tick;
    i16    lookdownup_q8;
    i16    was_ground_y;
    b8     was_grounded;
    i8     x_pan_v;             // left/right pan movement
    i8     offs_x;              // offset from owl position
    u8     force_lower_ceiling; // push camera up earlier; resets every frame
    u8     force_higher_floor;  // push camera down earlier; resets every frame
    b8     may_align_x;         // may align owl to multiple of 2px?
    b8     may_align_y;         // may align owl to multiple of 2px?
    b8     do_align_x;          // definitely do align owl
    b8     do_align_y;          // definitely do align owl
    u8     touched_top_tick;
    b8     center_req; // should center; reset every frame
} cam_owl_s;

typedef struct cam_clamp_coord_s {
    i32 dst;
    i32 cur;
} cam_clamp_coord_s;

enum {
    CAM_CLAMP_X1,
    CAM_CLAMP_Y1,
    CAM_CLAMP_X2,
    CAM_CLAMP_Y2
};

typedef struct cam_s {
    cam_owl_s cowl;
    v2_i32    shake;
    v2_i32    trg;
    v2_i32    attr_q12;
    v2_i32    p_center; // result of calculation

    ALIGNAS(32)
    cam_clamp_coord_s clamp_coord[4];

    u16 shake_ticks;
    u16 shake_ticks_max;
    u16 shake_str_x;
    u16 shake_str_y;
    u16 shake_str_x2;
    u16 shake_str_y2;
    u16 trg_fade_q12; // [0, 4096]
    b8  locked_x;     // should be replaced by clamp coords
    b8  locked_y;     // should be replaced by clamp coords
    b8  has_trg;
    u8  trg_fade_spd; // speed to fade (per tick)
} cam_s;

void    cam_update(g_s *g, cam_s *c);
void    cam_clamp_clr_hard(g_s *g); // what even in this name
void    cam_clamp_clr(g_s *g);
void    cam_clamp_set_hard(g_s *g, i32 x1, i32 y1, i32 x2, i32 y2); // what even in this name
void    cam_clamp_set(g_s *g, i32 x1, i32 y1, i32 x2, i32 y2);      // pass <= 0 to set clamp coord back to room
void    cam_clamp_setr(g_s *g, rec_i32 r);
void    cam_clamp_setr_hard(g_s *g, rec_i32 r);
void    cam_clamp_x1(g_s *g, i32 x); // pass <= 0 to set clamp coord back to room
void    cam_clamp_y1(g_s *g, i32 y); // pass <= 0 to set clamp coord back to room
void    cam_clamp_x2(g_s *g, i32 x); // pass <= 0 to set clamp coord back to room
void    cam_clamp_y2(g_s *g, i32 y); // pass <= 0 to set clamp coord back to room
void    cam_screenshake(cam_s *c, i32 ticks, i32 str);
void    cam_screenshake_xy(cam_s *c, i32 ticks, i32 str_x, i32 str_y);
v2_i32  cam_pos_px_top_left(cam_s *c);
v2_i32  cam_pos_px_center(cam_s *c);
rec_i32 cam_rec_px(cam_s *c);
void    cam_hard_set_positon(g_s *g, cam_s *c);

#endif