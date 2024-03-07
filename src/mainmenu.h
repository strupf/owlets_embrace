// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAINMENU_H
#define MAINMENU_H

#include "gamedef.h"
#include "sys/sys_types.h"

#define MAINMENU_SKIP_TO_GAME 0

enum {
    MAINMENU_FADE_NONE,
    MAINMENU_FADE_INTERNAL, // only fade text
    MAINMENU_FADE_GAME,     // fade to black
};

enum {
    MAINMENU_FADE_TICKS,
    MAINMENU_FADE_GAME_TICKS,
};

enum {
    MAINMENU_FADE_OUT,
    MAINMENU_FADE_IN,
};

// mainmenu state machine
enum {
    MAINMENU_ST_PRESS_START, // title screen
};

#define TITLE_FADE 30

typedef struct {
    i32 fade_to_game;

    i32 state;
    i32 option;

    i32 title_blink;
    i32 title_fade;
    f32 feather_time;
    f32 feather_y;
} mainmenu_s;

void mainmenu_init(mainmenu_s *t);
void mainmenu_update(game_s *g, mainmenu_s *t);
void mainmenu_render(mainmenu_s *t);

#endif