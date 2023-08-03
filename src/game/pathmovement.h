// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PATHMOVEMENT_H
#define PATHMOVEMENT_H

#include "gamedef.h"

enum {
        PATH_LINE,
        PATH_CIRC,
};

enum {
        PATH_ONCE,
        PATH_PINGPONG,
};

typedef struct {
        v2_i32 p;
} pathnode_s;

typedef struct {
        pathnode_s *from;
        pathnode_s *to;
        i32         len;
} pathedge_s;

typedef struct {
        i32 from;
        i32 to;
        i32 x;

        pathnode_s nodes[16];
        pathedge_s edges[16];
        int        nnodes;
        int        nedges;

        int pathtype;
} pathmover_s;

void path_update(pathmover_s *p);

#endif