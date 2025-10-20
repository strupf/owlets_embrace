// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STEERING_H
#define STEERING_H

#include "gamedef.h"

enum {
    STEERING_FORCE_SEEK,
    STEERING_FORCE_FLEE,
    STEERING_FORCE_ARRIVAL,
};

typedef struct {
    ALIGNAS(16)
    v2_i32 f;
    u8     type;
    u8     weight;
} steering_force_s;

typedef struct {
    ALIGNAS(32)
    v2_i32           p_q8;
    v2_i32           v_q8;
    i32              vmax_q8;
    v2_i32           p_arrival;
    i32              n_f;
    steering_force_s f[4];
} steering_obj_s;

void steering_obj_init(steering_obj_s *s, i32 vmax_q8, i32 m_inv_q8);
void steering_obj_set_pos(steering_obj_s *s, i32 x, i32 y);
void steering_obj_set_pos_q12(steering_obj_s *s, i32 x_q8, i32 y_q8);
void steering_obj_seek(steering_obj_s *s, v2_i32 p_q8);
void steering_obj_arrival(steering_obj_s *s, v2_i32 p_q8, i32 r_q8);
void steering_obj_flush(steering_obj_s *s);

// integer
v2_i32 steer_seek_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax);
v2_i32 steer_flee_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax);
v2_i32 steer_arrival_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax, i32 radius);

#endif