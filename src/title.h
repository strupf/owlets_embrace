// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "gamedef.h"
#include "save.h"

typedef struct app_s app_s;

#define TITLE_SKIP_TO_GAME 1

enum {
    TITLE_MAIN_FADE_OUT,
    TITLE_MAIN_FADE_IN,
};

typedef struct {
    b32 exists;
    i32 tick;
    u8  name[20];
    u8  map_name[32];
} save_preview_s;

enum {
    TITLE_BTN_SLOT_1,
    TITLE_BTN_SLOT_2,
    TITLE_BTN_SLOT_3,
    TITLE_BTN_SPEEDRUN,
    TITLE_BTN_SLOT_COPY,
    TITLE_BTN_SLOT_DELETE,
    //
    NUM_TITLE_BTNS
};

typedef struct {
    i16 x_src;
    i16 y_src;
    i16 x_dst;
    i16 y_dst;
    u8  lerp_tick;
    u8  lerp_tick_max;
} title_btn_s;

typedef struct title_s {
    i32            tick;
    u8             title_subst;
    u8             comp_bump;
    u8             start_tick;
    i8             title_fade;
    u8             state;
    u8             state_prev;
    u8             option;
    u8             selected;
    u8             copy_to;
    u8             confirm_tick;
    u8             preload_fade_q7;
    u8             preload_fade_dir; // 0 = out, 1 = in
    i8             preload_slot;
    i8             title_fade_dir;
    u8             fade_to_game;
    title_btn_s    buttons[NUM_TITLE_BTNS];
    //
    save_preview_s saves[3];
    i8             comp_btnID;
    v2_i32         pos_comp_target;
    v2_i32         pos_comp_q8;
    v2_i32         v_comp_q8;
} title_s;

void title_init(title_s *t);
void title_update(app_s *app, title_s *t);
void title_render(title_s *t);
void title_start_game(app_s *app, i32 slot);
void title_paused(title_s *t);

#endif