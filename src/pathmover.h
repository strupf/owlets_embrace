// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// utility to move a point along a linear path of nodes, either in a ping pong
// or looping way

#ifndef PATHMOVER_H
#define PATHMOVER_H

#include "gamedef.h"

#define PATHMOVER_MAX_PATH_NODES 24

enum {
    PATHMOVER_PATH_NONE,
    PATHMOVER_PATH_CIRCULAR,
    PATHMOVER_PATH_PINGPONG
};

typedef struct pathmover_s {
    i32 path_pos;  // current position
    u8  path_from; // current position
    u8  path_to;   // current position
    u8  n_path;
    u8  path_type;

    ALIGNAS(8)
    v2_i32 path_nodes[PATHMOVER_MAX_PATH_NODES];
    i32    seg_len[PATHMOVER_MAX_PATH_NODES]; // index 0 is length between node 0 and 1
    v2_i32 p_path_curr;
} pathmover_s;

void   pathmover_setup(pathmover_s *m, v2_i32 *p_arr, i32 n, i32 type);
i32    pathmover_move_on_path_to_next(pathmover_s *m, i32 dt, i32 *reached_node);
i32    pathmover_distance_to_in_dir(pathmover_s *m, i32 s_move, i32 p_end);
i32    pathmover_distance_to(pathmover_s *m, i32 p_end, i32 *s_move);
v2_i32 pathmover_curr_pos(pathmover_s *m);
v2_i32 pathmover_curr_connection_v2(pathmover_s *m);
i32    pathmover_num_nodes(pathmover_s *m);

#endif