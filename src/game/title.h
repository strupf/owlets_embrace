// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "fading.h"
#include "game_def.h"

enum {
        MAINMENU_STATE_TITLE,
        MAINMENU_STATE_SELECT_FILE,
};

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

typedef struct {
        bool32   input_blocked;
        int      curr_option;
        int      state;
        int      num_options;
        int      fadephase;
        int      fadeticks;
        fading_s fade;

        float feather_time;
        float feather_y;
        int   press_start_ticks;
} mainmenu_s;

void update_title(game_s *g, mainmenu_s *m);
void draw_title(game_s *g, mainmenu_s *m);

#endif