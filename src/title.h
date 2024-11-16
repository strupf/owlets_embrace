// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "gamedef.h"
#include "save.h"
#include "textinput.h"

#define TITLE_SKIP_TO_GAME 1

// mainmenu state machine
enum {
    TITLE_ST_PRESS_START, // title screen
    //
    TITLE_ST_FILE_SELECT,
    TITLE_ST_FILE_SELECTED,
    TITLE_ST_FILE_CPY,
    TITLE_ST_FILE_CPY_CONFIRM,
    TITLE_ST_FILE_DEL_CONFIRM,
    TITLE_ST_FILE_NEW,
    TITLE_ST_FILE_START,
    //
    TITLE_ST_OPTIONS,
};

enum {
    TITLE_F_START,
    TITLE_F_CPY,
    TITLE_F_DEL
};

typedef struct {
    u32  health;
    u32  tick;
    char name[32];
    char areaname[64];
} save_preview_s;

typedef struct title_s {
    u32            state_tick;
    u32            timer;
    u16            state;
    u16            state_prev;
    i16            option;
    u16            fade;
    u16            fade_0;
    u16            selected;
    u16            copy_to;
    u16            msg_tick;
    //
    save_preview_s saves[3];
    textinput_s    tinput;
    char           msg[64];
} title_s;

void title_init(title_s *t);
void title_update(g_s *g, title_s *t);
void title_render(title_s *t);

#endif