// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DRAW_H
#define DRAW_H

#include "rope.h"
#include "title.h"

enum {
        ANIMATION_MODE_ONCE,
        ANIMATION_MODE_LOOP,
        ANIMATION_MODE_LOOP_REVERSE,
        ANIMATION_MODE_LOOP_PINGPONG,
        ANIMATION_MODE_ONCE_REVERSE,
};

/* animation frames ALWAYS go from left to right
 */

typedef struct {
        int texID;
        int mode;
        int ticks;
        int n_frames;
        int state;
        int x;
        int y;
        int w;
        int h;
} animation_s;

rec_i32 animation_rec(animation_s *a);

enum {
        ITEM_FRAME_SIZE = 64,
        ITEM_BARREL_R   = 16,
        ITEM_BARREL_D   = ITEM_BARREL_R * 2,
        ITEM_SIZE       = 32,
        ITEM_X_OFFS     = 16,
        ITEM_Y_OFFS     = 16,
};

void merge_layer(tex_s screen, tex_s layer);
void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, v2_i32 camp);
void draw_rope(rope_s *r, v2_i32 camp);
void draw_particles(game_s *g, v2_i32 camp);
void draw_UI(game_s *g, v2_i32 camp);

#endif