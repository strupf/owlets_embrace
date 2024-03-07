// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _SCENE_H
#define _SCENE_H

#include "gamedef.h"

typedef struct {
    int x;
} scene_s;

void scene_push(scene_s *s);
void scene_pop();
void scene_update();
void scene_draw();

#endif