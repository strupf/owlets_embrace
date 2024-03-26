// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "scene.h"

typedef struct {
    i32     n;
    scene_s scenes[8];
} scenestack_s;

static scenestack_s SCENE;

void scenestack_push(game_s *g, scene_s s)
{
    SCENE.scenes[SCENE.n++] = s;
    if (s.on_enter) {
        s.on_enter(g);
    }
}

void scenestack_pop(game_s *g)
{
    assert(0 < SCENE.n);
    scene_s s = SCENE.scenes[--SCENE.n];
    if (s.on_leave) {
        s.on_leave(g);
    }
}

void scenestack_replace(game_s *g, scene_s s)
{
    scenestack_pop(g);
    scenestack_push(g, s);
}

void scenestack_update(game_s *g, inp_s inp)
{
    for (int n = SCENE.n - 1; 0 <= n; n--) {
        scene_s s       = SCENE.scenes[n];
        int     blocked = s.on_update(g, inp);
        if (blocked) break;
    }
}

void scenestack_draw(game_s *g)
{
    for (int n = 0; n < SCENE.n; n++) {
        scene_s s = SCENE.scenes[n];
        s.on_draw(g);
    }
}
