// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_A_H
#define BOSS_A_H

#include "gamedef.h"
#include "map_loader.h"

enum {
    BOSS_A_ASLEEP,
    BOSS_A_DEFEATED,
    //
    BOSS_A_INTRO_0,
    //
    BOSS_A_P1_IDLE,
    //
    BOSS_A_P1_TO_P2,
    //
    BOSS_A_P2_IDLE,
    //
    BOSS_A_OUTRO_0,
};

#define BOSS_A_CORE_NUM_SEGS_PER_LEG 16
#define BOSS_A_CORE_NUM_LEGS         3

enum {
    BOSS_A_CORE_ST_IDLE,
    BOSS_A_CORE_ST_HIDDEN,

};

#define BOSS_A_CORE_LEG_LEN_Q8 (256 << 8)

typedef struct {
    v2_i32 p_q8;
    v2_i32 pp_q8;
} boss_a_segment_s;

typedef struct {
    i32              l_q8;
    boss_a_segment_s segs[BOSS_A_CORE_NUM_SEGS_PER_LEG];
} boss_a_core_leg_s;

typedef struct {
    v2_i32            p_idle;
    v2_i32            p_head;
    v2_i32            p_dst;
    v2_i32            p_anchor;
    i32               bop_tick;
    boss_a_core_leg_s legs[BOSS_A_CORE_NUM_LEGS];
} boss_a_core_s;

obj_s *boss_a_core_create(g_s *g, v2_i32 p_anchor);
void   boss_a_core_on_update(g_s *g, obj_s *o);
void   boss_a_core_on_animate(g_s *g, obj_s *o);
void   boss_a_core_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void   boss_a_core_constrain_legs(g_s *g, obj_s *o);
void   boss_a_core_move_to_center(g_s *g, obj_s *o);
void   boss_a_core_show(obj_s *o);
void   boss_a_core_hide(obj_s *o);

enum {
    BOSS_A_PLANT_ST_CLOSED_ASLEEP,
    BOSS_A_PLANT_ST_CLOSED,
    BOSS_A_PLANT_ST_OPEN,
    BOSS_A_PLANT_ST_DEAD,
};

typedef struct {
    i32 x_anchor;
    i32 y_anchor;
    i32 phase_tick;
    i32 phase;

    i32          plant_state;
    i32          plant_tick;
    obj_s       *o_core;
    obj_handle_s o_tendrils[2];
} boss_a_s;

void boss_a_load(g_s *g, boss_a_s *b);
void boss_a_update(g_s *g, boss_a_s *b);
void boss_a_animate(g_s *g, boss_a_s *b);
void boss_a_draw(g_s *g, boss_a_s *b, v2_i32 cam);
void boss_a_draw_post(g_s *g, boss_a_s *b, v2_i32 cam);
void boss_a_awake(g_s *g);
void boss_a_p2_init(g_s *g, boss_a_s *b);

void boss_a_plant_set(g_s *g, boss_a_s *b, i32 state);
void boss_a_plant_animate(g_s *g, boss_a_s *b);
void boss_a_plant_draw(g_s *g, boss_a_s *b, v2_i32 cam);

enum {
    BOSS_A_TENDRIL_ST_IDLE,
    BOSS_A_TENDRIL_ST_SLASH_INIT,
    BOSS_A_TENDRIL_ST_SLASH_ANTICIPATE,
    BOSS_A_TENDRIL_ST_SLASH_EXE,
    BOSS_A_TENDRIL_ST_SLASH_WAIT,
    BOSS_A_TENDRIL_ST_HOOKED,
};

#define BOSS_A_TENDRIL_NUM_SEGS               12
#define BOSS_A_TENDRIL_TICKS_SLASH_INIT       12
#define BOSS_A_TENDRIL_TICKS_SLASH_ANTICIPATE 12
#define BOSS_A_TENDRIL_TICKS_SLASH_EXE        8
#define BOSS_A_TENDRIL_TICKS_SLASH_WAIT       12

typedef struct {
    i32    active;
    v2_i32 p_anchor;
    v2_i32 p_head;
    v2_i32 p_idle;
    i32    head_bop_tick;

    v2_i32 p_slash_from;
    v2_i32 p_slash_to;
    v2_i32 p_src;

    boss_a_segment_s segs[BOSS_A_TENDRIL_NUM_SEGS];
} boss_a_tendril_s;

obj_s *boss_a_tendril_create(g_s *g);
#endif