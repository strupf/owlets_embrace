// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ROPE_H
#define ROPE_H

#include "gamedef.h"

typedef struct ropenode_s ropenode_s;
typedef struct rope_s     rope_s;

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
        u32         len_max;
        i32         len_max_q16;
        i32         spring_q8;
        i32         damping_q8;
        bool32      dirty;
        ropenode_s *pool;
        ropenode_s  nodesraw[NUM_ROPE_NODES];
};

void   rope_init(rope_s *r);
void   ropenode_move(game_s *g, rope_s *r, ropenode_s *rn, v2_i32 dt);
void   rope_update(game_s *g, rope_s *r);
bool32 rope_intact(game_s *g, rope_s *r);
bool32 rope_stretched(game_s *g, rope_s *r);
void   rope_moved_by_solid(game_s *g, rope_s *r, obj_s *solid, v2_i32 dt);
v2_i32 rope_adjust_connected_vel(game_s *g, rope_s *r, ropenode_s *rn,
                                 v2_i32 subpos, v2_i32 vel);
#endif