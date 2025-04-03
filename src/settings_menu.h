// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include "settings.h"

enum {
    SETTINGS_MENU_OPT_EXIT_CONFIRM,
    SETTINGS_MENU_OPT_MODE,
    SETTINGS_MENU_OPT_VOL_MUS,
    SETTINGS_MENU_OPT_VOL_SFX,
    SETTINGS_MENU_OPT_SAVE_EXIT,
    SETTINGS_MENU_OPT_RESET,
    //
    NUM_SETTINGS_MENU_OPT
};

typedef struct {
    settings_s settings;
    b8         active;
    u8         fade_enter;
    u8         fade_leave;
    u8         opt;
    u8         exit_confirm; // leave with or without save
} settings_menu_s;

void settings_menu_enter(settings_menu_s *sm);
void settings_menu_update(settings_menu_s *sm);
void settings_menu_draw(settings_menu_s *sm);

#endif