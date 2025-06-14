// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ROPE_H
#define ROPE_H

#include "gamedef.h"

#define ROPE_VERLET_N  32
#define NUM_ROPE_NODES 32

typedef struct rope_s     rope_s;
typedef struct ropenode_s ropenode_s;

typedef struct rope_pt_s {
    ALIGNAS(16)
    v2_i32 p;
    v2_i32 pp;
} rope_pt_s;

struct ropenode_s {
    ALIGNAS(16)
    ropenode_s *next;
    ropenode_s *prev;
    v2_i32      p;
};

struct rope_s {
    bool16       active;
    bool16       dirty;
    u32          len_max_q4;
    obj_handle_s o_head;
    obj_handle_s o_tail;
    rope_pt_s    ropept[ROPE_VERLET_N];
    //
    ropenode_s  *head;
    ropenode_s  *tail;
    v2_i32       pmin;
    v2_i32       pmax;
    ropenode_s  *pool;
    ropenode_s   nodesraw[NUM_ROPE_NODES];
};

void        rope_init(rope_s *r);
u32         rope_len_q4(g_s *g, rope_s *r);
void        ropenode_move(g_s *g, rope_s *r, ropenode_s *rn, i32 dx, i32 dy);
void        rope_update(g_s *g, rope_s *r);
bool32      rope_is_intact(g_s *g, rope_s *r);
void        rope_moved_by_aabb(g_s *g, rope_s *r, rec_i32 aabb, i32 dx, i32 dy);
ropenode_s *ropenode_neighbour(rope_s *r, ropenode_s *rn);
void        rope_verletsim(g_s *g, rope_s *r);
i32         rope_stretch_q8(g_s *g, rope_s *r);
obj_s      *rope_obj_connected_to(obj_s *o);

#endif