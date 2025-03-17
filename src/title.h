// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TITLE_H
#define TITLE_H

#include "gamedef.h"
#include "save.h"
#include "textinput.h"

typedef struct app_s app_s;

#define TITLE_SKIP_TO_GAME 1

typedef struct {
    b32 exists;
    i32 tick;
    u8  name[LEN_HERO_NAME];
    u8  map_name[32];
} save_preview_s;

typedef struct title_s {
    i32            select_anim;
    i32            tick;
    u8             state;
    u8             state_prev;
    u8             option;
    u8             selected;
    u8             copy_to;
    u8             confirm_tick;
    //
    save_preview_s saves[3];
    textinput_s    tinput;
    v2_i32         pos_comp_target;
    v2_i32         pos_comp_q8;
    v2_i32         v_comp_q8;
} title_s;

void title_init(title_s *t);
void title_update(app_s *app, title_s *t);
void title_render(title_s *t);
void title_start_game(app_s *app, i32 slot);

#endif