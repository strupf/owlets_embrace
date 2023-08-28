// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DRAW_H
#define DRAW_H

#include "gamedef.h"

enum {
        SPRITE_ANIM_FRAMES = 16,
};

enum sprite_anim_mode {
        SPRITE_ANIM_MODE_ONCE,
        SPRITE_ANIM_MODE_PINGPONG,
        SPRITE_ANIM_MODE_LOOP,
        SPRITE_ANIM_MODE_RNG,
};

typedef struct {
        tex_s tex;
        int   sx; // first frame x
        int   sy; // first frame y
        int   sw; // sprite w
        int   sh; // sprite h
        int   dx; // direction in x
        int   dy; // direction in y
        int   nframes;
        int   time;
        int   frame;
        int   mode;
        int   dir;
        int   times[SPRITE_ANIM_FRAMES];
} sprite_anim_s;

void        sprite_anim_update(sprite_anim_s *a);
void        sprite_anim_set(sprite_anim_s *a, int frame, int time);
texregion_s sprite_anim_get(sprite_anim_s *a);

#endif