// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ROPE_H
#define ROPE_H

#include "gamedef.h"

typedef struct ropenode_s     ropenode_s;
typedef struct rope_s         rope_s;
typedef struct ropecollider_s ropecollider_s;

struct ropecollider_s {
        ropecollider_s *next;
        ropecollider_s *prev;
        tri_i32         tris[256];
        int             n;
};

struct ropenode_s {
        ropenode_s *next;
        ropenode_s *prev;
        v2_i32      p;
};

#define NUM_ROPE_NODES 256
struct rope_s {
        ropenode_s *head;
        ropenode_s *tail;

        u32         len_max;
        i32         len_max_q16;
        i32         spring_q8;
        i32         damping_q8;
        ropenode_s *pool;
        ropenode_s  nodesraw[NUM_ROPE_NODES];
};

void   rope_init(rope_s *r);
void   ropenode_move(rope_s *r, ropecollider_s *c, ropenode_s *rn, v2_i32 dt);
void   rope_update(rope_s *r, ropecollider_s *c);
bool32 rope_blocked(rope_s *r, ropecollider_s *c);
v2_i32 rope_adjust_connected_vel(rope_s *r, ropenode_s *rn,
                                 v2_i32 subpos, v2_i32 vel);
#endif