// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _INVENTORY_H
#define _INVENTORY_H

#include "gamedef.h"

enum {
    MENU_SCREEN_TAB_MAP,
    MENU_SCREEN_TAB_INVENTORY,
    //
    NUM_MENU_SCREEN_TABS
};

enum {
    MAP_MODE_SCROLL,
    MAP_MODE_PIN_SELECT,
};

enum {
    MAP_PIN_TYPE_CROSS,
    MAP_PIN_TYPE_SKULL,
    //
    NUM_MAP_PIN_TYPES
};

typedef struct {
    bool32 active;
    i32    tab;
    bool32 tab_selected;
    i32    tab_ticks;

    struct {
        i32    mode;
        i32    pin_type;
        v2_i32 pos;
        i32    scl_q12;
        i32    cursoranimtick;
    } map;

    struct {
        i32 x;
        i32 y;
    } inventory;
} menu_screen_s;

bool32 menu_screen_active(menu_screen_s *m);
void   menu_screen_update(game_s *g, menu_screen_s *m);
void   menu_screen_draw(game_s *g, menu_screen_s *m);
void   menu_screen_open(game_s *g, menu_screen_s *m);

#endif