// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "fading.h"
#include "game_def.h"
#include "savefile.h"

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

enum {
        MAINMENU_FILESELECT_0,
        MAINMENU_FILESELECT_1,
        MAINMENU_FILESELECT_2,
        MAINMENU_FILESELECT_DELETE,
        MAINMENU_FILESELECT_COPY,
        //
        NUM_MAINMENU_FILESELECT_OPTIONS
};

enum {
        MAINMENU_DELETE_NONE,
        MAINMENU_DELETE_SELECT,
        MAINMENU_DELETE_NO,
        MAINMENU_DELETE_YES,
};

enum {
        MAINMENU_COPY_NONE,
        MAINMENU_COPY_SELECT_FROM,
        MAINMENU_COPY_SELECT_TO,
        MAINMENU_COPY_NO,
        MAINMENU_COPY_YES,
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

        int deletestate;
        int copystate;
        int file_select_1;
        int file_select_2;
        int animticks_copy;
        int animticks_delete;

        savefile_s file0;
        savefile_s file1;
        savefile_s file2;
} mainmenu_s;

void update_title(game_s *g, mainmenu_s *m);
void draw_title(game_s *g, mainmenu_s *m);

#endif