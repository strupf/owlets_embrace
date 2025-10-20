// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// a wire which wraps around geometry and is affected by moving rectangular solid objects

#ifndef WIRE_H
#define WIRE_H

#include "gamedef.h"

#define NUM_WIRE_NODES 32

typedef struct wire_s     wire_s;
typedef struct wirenode_s wirenode_s;

struct wirenode_s {
    ALIGNAS(16)
    wirenode_s *next;
    wirenode_s *prev;
    v2_i32      p;
};

struct wire_s {
    ALIGNAS(32)
    bool32      dirty;
    wirenode_s *head;
    wirenode_s *tail;
    v2_i32      pmin;
    v2_i32      pmax;
    wirenode_s *pool;
    wirenode_s  nodesraw[NUM_WIRE_NODES];
};

void        wire_init(wire_s *r);
void        wire_init_as_copy(wire_s *r, wire_s *to_copy);
u32         wire_len_qx(g_s *g, wire_s *r, i32 q);
void        wirenode_move(g_s *g, wire_s *r, wirenode_s *rn, i32 dx, i32 dy);
void        wire_optimize(g_s *g, wire_s *r);
bool32      wire_is_intact(g_s *g, wire_s *r);
void        wire_moved_by_aabb(g_s *g, wire_s *r, rec_i32 aabb, i32 dx, i32 dy);
wirenode_s *wirenode_neighbour_of_end_node(wire_s *r, wirenode_s *rn);
v2_i32      wirenode_vec(wire_s *r, wirenode_s *rn);

#endif