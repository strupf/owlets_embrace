// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pathmover.h"

i32 pathmover_move_on_path_to_next(pathmover_s *m, i32 v_q4, i32 *reached_node); // moved by v_q4, but at maximum to the next node; returns d_q4 remaining
i32 pathmover_next_pt(pathmover_s *m, i32 p_from, i32 p_to);
i32 pathmover_distance_to_in_dir(pathmover_s *m, i32 s_move, i32 p_end); // distance to reach p_end when moving in direction s_move
i32 pathmover_len_between(pathmover_s *m, i32 a, i32 b);

void pathmover_setup(pathmover_s *m, v2_i32 *p_arr, i32 n, i32 type)
{
    for (i32 i = 0; i < n; i++) {
        m->path_nodes[i] = p_arr[i];
    }

    for (i32 i = 1; i < n; i++) {
        i32 l             = v2_i32_distancel(m->path_nodes[i - 1], m->path_nodes[i]);
        m->seg_len[i - 1] = l;
    }
    m->n_path = n;
    if (type == PATHMOVER_PATH_CIRCULAR) {
        m->seg_len[n - 1] = v2_i32_distancel(m->path_nodes[0], m->path_nodes[n - 1]);
    }
    m->path_type = type;
    m->path_from = 0;
    m->path_to   = 1;
}

i32 pathmover_move_on_path_to_next(pathmover_s *m, i32 dt, i32 *reached_node)
{
    if (dt == 0) return 0;

    i32 l     = pathmover_len_between(m, m->path_from, m->path_to);
    i32 d     = dt;
    i32 pos_p = m->path_pos;

    if (dt < 0 && 0 < m->path_pos) {
        d = -min_i32(-dt, m->path_pos);
    }
    if (dt > 0 && m->path_pos < l) {
        d = +min_i32(+dt, l - m->path_pos);
    }
    m->path_pos += d;

    if (reached_node) {
        *reached_node = -1;
    }

    if (m->path_pos == 0) {
        if (reached_node) {
            *reached_node = m->path_from;
        }
    } else if (m->path_pos < 0) {
        i32 p_from   = m->path_from;
        i32 p_to     = m->path_to;
        m->path_to   = m->path_from;
        m->path_from = pathmover_next_pt(m, p_to, p_from);
        m->path_pos += pathmover_len_between(m, m->path_from, m->path_to);
    } else if (l <= m->path_pos) {
        if (reached_node) {
            *reached_node = m->path_to;
        }
        m->path_pos -= l;
        i32 p_from   = m->path_from;
        i32 p_to     = m->path_to;
        m->path_from = m->path_to;
        m->path_to   = pathmover_next_pt(m, p_from, p_to);
    }
    assert(0 <= m->path_from && m->path_from < m->n_path);
    assert(0 <= m->path_to && m->path_to < m->n_path);

    return (dt - d);
}

i32 pathmover_next_pt(pathmover_s *m, i32 p_from, i32 p_to)
{
    switch (m->path_type) {
    case PATHMOVER_PATH_CIRCULAR: {
        if (p_to == m->n_path - 1) {
            return (p_from == m->n_path - 2 ? 0 : m->n_path - 2);
        }
        if (p_to == 0) {
            return (p_from == 1 ? m->n_path - 1 : 1);
        }
        if (p_from < p_to) {
            return p_to + 1;
        }
        return p_to - 1;
    }
    case PATHMOVER_PATH_PINGPONG: {
        if (p_to == m->n_path - 1) {
            return m->n_path - 2;
        }
        if (p_to == 0) {
            return 1;
        }
        if (p_from < p_to) {
            return p_to + 1;
        }
        return p_to - 1;
    }
    }
    return 0;
}

i32 pathmover_distance_to_in_dir(pathmover_s *m, i32 s_move, i32 p_end)
{
    if (s_move == 0) return 0;

    i32 p1 = (0 < s_move ? m->path_from : m->path_to);
    i32 p2 = (0 < s_move ? m->path_to : m->path_from);
    i32 l  = (0 < s_move ? pathmover_len_between(m, p1, p2) - m->path_pos
                         : m->path_pos);

    while (p2 != p_end) {
        i32 p = pathmover_next_pt(m, p1, p2);
        p1    = p2;
        p2    = p;
        l += pathmover_len_between(m, p1, p2);
    }
    return l;
}

i32 pathmover_distance_to(pathmover_s *m, i32 p_end, i32 *s_move)
{
    i32 d1 = pathmover_distance_to_in_dir(m, +1, p_end);
    i32 d2 = pathmover_distance_to_in_dir(m, -1, p_end);
    if (d1 < d2) {
        if (s_move) {
            *s_move = +1;
        }
        return d1;
    }
    if (d1 > d2) {
        if (s_move) {
            *s_move = -1;
        }
        return d2;
    }
    if (s_move) {
        *s_move = 0;
    }
    return d1;
}

i32 pathmover_len_between(pathmover_s *m, i32 a, i32 b)
{
    if (m->path_type == PATHMOVER_PATH_CIRCULAR &&
        ((a == 0 && b == m->n_path - 1) ||
         (b == 0 && a == m->n_path - 1))) {
        return m->seg_len[m->n_path - 1];
    }
    return m->seg_len[max_i32(a, b) - 1];
}

v2_i32 pathmover_curr_pos(pathmover_s *m)
{
    v2_i32 p1 = m->path_nodes[m->path_from];
    v2_i32 p2 = m->path_nodes[m->path_to];
    i32    l  = pathmover_len_between(m, m->path_from, m->path_to);
    return v2_i32_lerpl(p1, p2, m->path_pos, l - 1);
}

v2_i32 pathmover_curr_connection_v2(pathmover_s *m)
{
    v2_i32 p1 = m->path_nodes[m->path_from];
    v2_i32 p2 = m->path_nodes[m->path_to];
    return v2_i32_sub(p2, p1);
}

i32 pathmover_num_nodes(pathmover_s *m)
{
    return m->n_path;
}