// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void steering_obj_init(steering_obj_s *s, i32 vmax_q8, i32 m_inv_q8)
{
    mclr(s, sizeof(steering_obj_s));
    s->vmax_q8 = vmax_q8;
}

void steering_obj_set_pos(steering_obj_s *s, i32 x, i32 y)
{
    steering_obj_set_pos_q12(s, x << 8, y << 8);
}

void steering_obj_set_pos_q12(steering_obj_s *s, i32 x_q8, i32 y_q8)
{
    s->p_q8.x = x_q8;
    s->p_q8.y = y_q8;
}

void steering_obj_seek(steering_obj_s *s, v2_i32 p_q8)
{
    v2_i32            v = steer_seek_vel(s->p_q8, s->v_q8, p_q8, s->vmax_q8);
    steering_force_s *f = &s->f[s->n_f++];
    f->f                = v;
    f->weight           = 1;
    f->type             = STEERING_FORCE_SEEK;
}

void steering_obj_arrival(steering_obj_s *s, v2_i32 p_q8, i32 r_q8)
{

    v2_i32            v = steer_arrival_vel(s->p_q8, s->v_q8, p_q8, s->vmax_q8, r_q8);
    steering_force_s *f = &s->f[s->n_f++];
    s->p_arrival        = p_q8;
    f->f                = v;
    f->weight           = 1;
    f->type             = STEERING_FORCE_ARRIVAL;
}

void steering_obj_flush(steering_obj_s *s)
{
    v2_i32            f          = {0};
    steering_force_s *sf_arrival = 0;

    for (i32 n = 0; n < s->n_f; n++) {
        steering_force_s *sf = &s->f[n];

        f = v2_i32_add(f, sf->f);
        if (sf->type == STEERING_FORCE_ARRIVAL) {
            sf_arrival = sf;
        }
    }

    v2_i32 a = f;
    s->v_q8  = v2_i32_add(s->v_q8, a);
    s->p_q8  = v2_i32_add(s->p_q8, s->v_q8);

    if (sf_arrival && s->n_f == 1 && v2_i32_distance_appr(s->p_arrival, s->p_q8) < 8) {
        s->p_q8   = s->p_arrival;
        s->v_q8.x = 0;
        s->v_q8.y = 0;
    }
    s->n_f = 0;
}

v2_i32 steer_seek_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax)
{
    v2_i32 v_trg = v2_i32_setlen_fast(v2_i32_sub(p_trg, p), vmax);
    v2_i32 v_dtt = v2_i32_sub(v_trg, v);

    i32    fmax = 100;
    v2_i32 f    = v2_i32_mul_ratio(v_dtt, fmax, vmax);
    return f;
}

v2_i32 steer_flee_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax)
{
    v2_i32 r = steer_seek_vel(p, v, p_trg, vmax);
    return v2_i32_inv(r);
}

v2_i32 steer_arrival_vel(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax, i32 radius)
{
    v2_i32 p_dt = v2_i32_sub(p_trg, p);
    i32    l    = v2_i32_len_appr(p_dt);
    i32    vset = vmax;
    if (l < radius) {
        vset = div_away_from_zero_i32((vmax * l), radius);
    }
    v2_i32 v_trg = v2_i32_setlenl(p_dt, l, vset);
    v2_i32 v_dtt = v2_i32_sub(v_trg, v);

    i32    fmax = 100;
    v2_i32 f    = v2_i32_mul_ratio(v_dtt, fmax, vmax);
    return f;
}