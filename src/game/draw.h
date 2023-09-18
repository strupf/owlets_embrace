// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DRAW_H
#define DRAW_H

#include "backforeground.h"
#include "gamedef.h"
#include "rope.h"

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

enum {
        ITEM_FRAME_SIZE = 64,
        ITEM_BARREL_R   = 16,
        ITEM_BARREL_D   = ITEM_BARREL_R * 2,
        ITEM_SIZE       = 32,
        ITEM_X_OFFS     = 16,
        ITEM_Y_OFFS     = 16,
};

void merge_layer(tex_s screen, tex_s layer);
void draw_background(backforeground_s *bg, v2_i32 camp);
void draw_foreground(backforeground_s *bg, v2_i32 camp);
void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, int l, v2_i32 camp);
void draw_transition(game_s *g);
void draw_rope(rope_s *r, v2_i32 camp);
void draw_particles(game_s *g, v2_i32 camp);
void draw_game_UI(game_s *g, v2_i32 camp);

#endif