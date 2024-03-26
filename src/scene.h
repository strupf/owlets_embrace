// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _SCENE_H
#define _SCENE_H

#include "gamedef.h"

typedef void (*scene_enter_f)(game_s *g);
typedef void (*scene_leave_f)(game_s *g);
typedef int (*scene_update_f)(game_s *g, inp_s inp);
typedef int (*scene_draw_f)(game_s *g);

typedef struct {
    scene_enter_f  on_enter;
    scene_leave_f  on_leave;
    scene_update_f on_update;
    scene_draw_f   on_draw;
    i32            tick;
    i32            tick_og;
} scene_s;

void scenestack_push(game_s *g, scene_s s);
void scenestack_pop(game_s *g);
void scenestack_replace(game_s *g, scene_s s);
void scenestack_update(game_s *g, inp_s inp);
void scenestack_draw(game_s *g);

#endif