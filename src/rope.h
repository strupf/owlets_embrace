// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ROPE_H
#define ROPE_H

#include "gamedef.h"

typedef struct rope_s     rope_s;
typedef struct ropenode_s ropenode_s;

struct ropenode_s {
    ropenode_s *next;
    ropenode_s *prev;
    v2_i32      p;
};

#define NUM_ROPE_NODES 64
struct rope_s {
    ropenode_s *head;
    ropenode_s *tail;
    v2_i32      pmin;
    v2_i32      pmax;
    u32         len_max_q4;
    bool32      dirty;
    ropenode_s *pool;
    ropenode_s  nodesraw[NUM_ROPE_NODES];
};

typedef struct {
    i32    mass;
    v2_i32 vel_q8;
    v2_i32 subpos_q8; // subpixel
} rope_attachment_s;

void        rope_init(rope_s *r);
void        rope_set_len_max_q4(rope_s *r, i32 len_max_q4);
u32         rope_length_q4(game_s *g, rope_s *r);
void        ropenode_move(game_s *g, rope_s *r, ropenode_s *rn, v2_i32 dt);
void        rope_update(game_s *g, rope_s *r);
bool32      rope_intact(game_s *g, rope_s *r);
bool32      rope_stretched(game_s *g, rope_s *r);
void        rope_moved_by_solid(game_s *g, rope_s *r, obj_s *solid, v2_i32 dt);
ropenode_s *ropenode_neighbour(rope_s *r, ropenode_s *rn);
// i32         ropenode_pulling_force(rope_s *r, ropenode_s *rn_at, );
#endif