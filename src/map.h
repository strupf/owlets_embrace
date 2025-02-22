// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_H
#define MAP_H

#include "gamedef.h"

#define MAP_NUM_PINS 64

typedef struct {
    u32    type;
    v2_i32 p; // world coordinates (pixels)
} map_pin_s;

typedef struct {
    i32       crank_acc_q16;
    u16       w_pixel;
    u16       h_pixel;
    u16       scl;
    u16       n_pins;
    v2_i32    p; // center pos in world coordinates (pixels)
    map_pin_s pins[MAP_NUM_PINS];
} map_s;

void   map_init(g_s *g);
void   map_update(g_s *g);
void   map_draw(g_s *g);
v2_i32 map_coords_world_q8_from_screen(map_s *m, v2_i32 pxpos);
v2_i32 map_coords_screen_from_world_q8(map_s *m, v2_i32 wp_q8);

#endif