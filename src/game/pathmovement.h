// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PATHMOVEMENT_H
#define PATHMOVEMENT_H

#include "os/os.h"

typedef enum {
        PATH_TYPE_LINE,
        PATH_TYPE_CIRC,
} path_type_e;

typedef enum {
        PATH_LINE_STOP_END,
        PATH_LINE_PINGPONG,
} path_line_e;

typedef struct pathnode_s pathnode_s;
struct pathnode_s {
        v2_i32      p;
        pathnode_s *next;
        pathnode_s *prev;
};

typedef struct {
        i32 x;
        i32 v;

        path_type_e pathtype;

        union {
                struct {
                        pathnode_s *from;
                        pathnode_s *to;
                        path_line_e movetype;
                        pathnode_s  nodes[16];
                        i32         nodedistances[16];
                };
                struct {
                        v2_i32 circcenter;
                        i32    circr;
                        i32    circumference;
                };
        };
} pathmover_s;

void   path_update(pathmover_s *p);
v2_i32 path_pos(pathmover_s *p);

#endif