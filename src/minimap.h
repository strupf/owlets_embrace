// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MINIMAP_H
#define MINIMAP_H

#include "gamedef.h"

#define MAP_NUM_PINS      64
#define MINIMAP_SCREENS_X 164
#define MINIMAP_SCREENS_Y 274
#define MINIMAP_N_SCREENS (MINIMAP_SCREENS_X * MINIMAP_SCREENS_Y)

typedef struct {
    i16 type;
    i16 x; // world coordinates in tiles
    i16 y;
} map_pin_s;

typedef struct {
    bool32 opened;
    i32    centerx;
    i32    centery;
    u8     tick;
    u8     n_pins;
    i32    fade;
    i32    fade_out;

    map_pin_s *pin_hovered;
    map_pin_s  pins[MAP_NUM_PINS];
    u32        visited_tmp[MINIMAP_N_SCREENS / 32];
    u32        visited[MINIMAP_N_SCREENS / 32];
} minimap_s;

void minimap_init(g_s *g);
void minimap_open(g_s *g);
void minimap_update(g_s *g);
void minimap_draw(g_s *g);
void minimap_draw_pause(g_s *g);
void minimap_confirm_visited(g_s *g);
void minimap_try_visit_screen(g_s *g);
b32  minimap_screen_visited_tmp(g_s *g, i32 sx, i32 sy);
b32  minimap_screen_visited(g_s *g, i32 sx, i32 sy);

#endif