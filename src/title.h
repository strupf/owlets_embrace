// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "gamedef.h"
#include "sys/sys_types.h"

#define TITLE_SKIP_TO_GAME 1

enum {
    TITLE_FADE_NONE,
    TITLE_FADE_INTERNAL, // only fade text
    TITLE_FADE_GAME,     // fade to black
};

enum {
    TITLE_FADE_TICKS,
    TITLE_FADE_GAME_TICKS,
};

enum {
    TITLE_FADE_OUT,
    TITLE_FADE_IN,
};

// mainmenu state machine
enum {
    TITLE_ST_PRESS_START, // title screen
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
} title_s;

void title_init(title_s *t);
void title_update(game_s *g, title_s *t);
void title_render(title_s *t);

#endif