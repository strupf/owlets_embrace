// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MINIMAP_H
#define MINIMAP_H

#include "gamedef.h"

#define MAP_NUM_PINS      64
#define MINIMAP_SCREENS_X 512
#define MINIMAP_SCREENS_Y 256
#define MINIMAP_N_SCREENS (MINIMAP_SCREENS_X * MINIMAP_SCREENS_Y)

enum {
    MINIMAP_SCREEN_INDEX_VISITED   = 0,
    MINIMAP_SCREEN_INDEX_CONFIRMED = 1
};

enum {
    MINIMAP_ST_INACTIVE,
    MINIMAP_ST_FADE_IN,
    MINIMAP_ST_FADE_IN_MENU,
    MINIMAP_ST_FADE_OUT,
    MINIMAP_ST_NAV,
    MINIMAP_ST_DELETE_PIN,
    MINIMAP_ST_SELECT_PIN,
};

typedef struct {
    i16 x; // world coordinates in tiles
    i16 y;
    u16 type;
} minimap_pin_s;

typedef struct {
    i32 centerx;
    i32 centery;
    i32 cursorx;
    i32 cursory;
    i16 herox;
    i16 heroy;
    u8  state;
    u8  tick;
    u8  pin_selected;
    u8  n_pins;
    u8  pin_ui_fade;
    u8  pin_deny_tick;

    minimap_pin_s *pin_hovered;
    minimap_pin_s  pins[MAP_NUM_PINS];
    u32            visited[MINIMAP_N_SCREENS >> 4]; // 2 bits per screen
} minimap_s;

void minimap_init(g_s *g);
void minimap_open(g_s *g);
void minimap_open_via_menu(g_s *g);
void minimap_update(g_s *g);
void minimap_draw(g_s *g);
void minimap_draw_pause(g_s *g);
void minimap_confirm_visited(g_s *g);
void minimap_try_visit_screen(g_s *g);
i32  minimap_screen_visited(g_s *g, i32 tx, i32 ty); // tile coordinates

#endif