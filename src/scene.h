// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SCENE_H
#define SCENE_H

typedef struct {
    void (*tick)(void *arg);
    void (*draw)(void *arg);
    void *arg;
} scene_s;

typedef struct {
    int     n;
    int     fade_in;
    int     fade_out;
    int     fade;
    scene_s scenes[16];
} scene_stack_s;

scene_s scene_create(void (*tick)(void *arg), void (*draw)(void *arg), void *arg);
void    scene_push(scene_stack_s *st, scene_s s, int fade_in_ticks);
void    scene_pop(scene_stack_s *st, int fade_out_ticks);
void    scene_transition(scene_stack_s *st, scene_s s, void (*cb)(void *arg), void *arg);
void    scene_update(scene_stack_s *st);
void    scene_draw(scene_stack_s *st);
#endif