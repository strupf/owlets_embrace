// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "gamedef.h"
#include "savefile.h"
#include "sys/sys_types.h"

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

// mainmenu state machine
enum {
    MAINMENU_ST_PRESS_START,      // title screen
    MAINMENU_ST_FILESELECT,       // select file to play
    MAINMENU_ST_DELETE_SELECT,    // select file to delete
    MAINMENU_ST_DELETE_NO_OR_YES, // confirm file deletion prompt
    MAINMENU_ST_COPY_SELECT_FROM, // select file to copy
    MAINMENU_ST_COPY_SELECT_TO,   // select file copy destination
    MAINMENU_ST_COPY_NO_OR_YES,   // confirm file copy prompt
};

// human-readable constants for option numbers
enum {
    MAINMENU_OPTION_COPY   = 3,
    MAINMENU_OPTION_DELETE = 4,
    MAINMENU_OPTION_NO     = 0,
    MAINMENU_OPTION_YES    = 1,
};

typedef struct {
    int        exists;
    savefile_s sf;
} mainmenu_savefile_s;

typedef struct {
    int state;
    int option;

    int file_to_copy_from;
    int file_to_copy_to;
    int file_to_delete;

    mainmenu_savefile_s savefiles[3]; // savefiles for preview
} mainmenu_s;

void mainmenu_init(mainmenu_s *t);
void mainmenu_update(game_s *g, mainmenu_s *t);
void mainmenu_render(mainmenu_s *t);

#endif