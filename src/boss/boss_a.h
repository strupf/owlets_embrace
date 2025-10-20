// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_A_H
#define BOSS_A_H

#include "gamedef.h"
#include "map_loader.h"

typedef struct boss_a_s boss_a_s;

enum {
    BOSS_A_ASLEEP,
    BOSS_A_DEFEATED,
    //
    BOSS_A_INTRO_0,
    BOSS_A_INTRO_10,
    BOSS_A_INTRO_20,
    BOSS_A_INTRO_30,
    BOSS_A_INTRO_40,
    BOSS_A_INTRO_50,
    BOSS_A_INTRO_END,
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

#define BOSS_A_CORE_TICKS_STOMP_INIT       10
#define BOSS_A_CORE_TICKS_STOMP_TARGETING  10
#define BOSS_A_CORE_TICKS_STOMP_ANTICIPATE 10
#define BOSS_A_CORE_TICKS_STOMP_EXE        10
#define BOSS_A_CORE_TICKS_STOMP_CRASHED    10
#define BOSS_A_CORE_TICKS_STOMP_RECOVER    10

enum {
    BOSS_A_CORE_ST_HIDDEN,
    BOSS_A_CORE_ST_IDLE,
    BOSS_A_CORE_ST_STOMP_INIT,
    BOSS_A_CORE_ST_STOMP_TARGETING,
    BOSS_A_CORE_ST_STOMP_ANTICIPATE,
    BOSS_A_CORE_ST_STOMP_EXE,
    BOSS_A_CORE_ST_STOMP_CRASHED,
    BOSS_A_CORE_ST_STOMP_RECOVER,
    BOSS_A_CORE_ST_DYING

};

#define BOSS_A_CORE_LEG_LEN_Q8 (256 << 8)

typedef struct {
    ALIGNAS(8)
    v2_i32 p_q8;
    v2_i32 pp_q8;
} boss_a_segment_s;

typedef struct {
    i32              l_q8;
    boss_a_segment_s segs[BOSS_A_CORE_NUM_SEGS_PER_LEG];
} boss_a_core_leg_s;

typedef struct {
    boss_a_s         *b;
    v2_i32            p_idle;
    v2_i32            p_head;
    v2_i32            p_dst;
    v2_i32            p_anchor;
    i32               bop_tick;
    i32               hurt_tick;
    i32               show_q8;
    boss_a_core_leg_s legs[BOSS_A_CORE_NUM_LEGS];
} boss_a_core_s;

obj_s *boss_a_core_create(g_s *g, boss_a_s *b, v2_i32 p_anchor);
void   boss_a_core_constrain_legs(g_s *g, obj_s *o);
void   boss_a_core_move_to_center(g_s *g, obj_s *o);
void   boss_a_core_set_show(obj_s *o, i32 show);
void   boss_a_core_on_update(g_s *g, obj_s *o);
void   boss_a_core_on_animate(g_s *g, obj_s *o);
void   boss_a_core_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void   boss_a_core_move_to_pos(g_s *g, obj_s *o, v2_i32 pos);

enum {
    BOSS_A_TENDRIL_ST_HIDDEN,
    BOSS_A_TENDRIL_ST_IDLE,
    BOSS_A_TENDRIL_ST_SLASH_INIT,
    BOSS_A_TENDRIL_ST_SLASH_ANTICIPATE,
    BOSS_A_TENDRIL_ST_SLASH_EXE,
    BOSS_A_TENDRIL_ST_SLASH_WAIT,
    BOSS_A_TENDRIL_ST_SLASH_RETURN,
    BOSS_A_TENDRIL_ST_HOOKED,
};

#define BOSS_A_TENDRIL_NUM_SEGS               12
#define BOSS_A_TENDRIL_TICKS_SLASH_INIT       10
#define BOSS_A_TENDRIL_TICKS_SLASH_ANTICIPATE 20
#define BOSS_A_TENDRIL_TICKS_SLASH_EXE        10
#define BOSS_A_TENDRIL_TICKS_SLASH_WAIT       12
#define BOSS_A_TENDRIL_TICKS_SLASH_RETURN     10

typedef struct {
    boss_a_s *b;
    i32       x_sign;
    i32       active;
    v2_i32    p_anchor;
    i32       head_bop_tick;
    i32       show_q8;

    v2_i32 p_idle;

    v2_i32 p_slash0; // position of obj before slash
    v2_i32 p_slash1; // slash from
    v2_i32 p_slash2; // slash to

    boss_a_segment_s segs[BOSS_A_TENDRIL_NUM_SEGS];
} boss_a_tendril_s;

obj_s *boss_a_tendril_create(g_s *g, boss_a_s *b, v2_i32 p_anchor, i32 x_sign);
void   boss_a_tendril_move_to_pos(g_s *g, obj_s *o, v2_i32 pos);
void   boss_a_tendril_update(g_s *g, obj_s *o);
void   boss_a_tendril_animate(g_s *g, obj_s *o);
void   boss_a_tendril_draw(g_s *g, obj_s *o, v2_i32 cam);
void   boss_a_tendril_set_show(obj_s *o, i32 show);
void   boss_a_tendril_slash(obj_s *o, v2_i32 p_from, v2_i32 p_to);

enum {
    BOSS_A_PLANT_ST_CLOSED_ASLEEP,
    BOSS_A_PLANT_ST_CLOSED,
    BOSS_A_PLANT_ST_OPEN,
    BOSS_A_PLANT_ST_DEAD,
};

enum {
    BOSS_A_INDEX_TENDRIL_L,
    BOSS_A_INDEX_TENDRIL_R,
    //
    BOSS_A_NUM_TENDRILS
};

struct boss_a_s {
    obj_s *o_comp;
    obj_s *puppet_comp;
    obj_s *puppet_owl;
    i32    puppet_comp_flag;
    i32    puppet_owl_flag;
    i32    counter;
    i32    x_anchor;
    i32    y_anchor;
    i32    phase_tick;
    i32    phase;

    i32          plant_state;
    i32          plant_tick;
    obj_s       *o_core;
    obj_handle_s o_tendrils[BOSS_A_NUM_TENDRILS];
};

void boss_a_load(g_s *g, boss_a_s *b);
void boss_a_update(g_s *g, boss_a_s *b);
void boss_a_animate(g_s *g, boss_a_s *b);
void boss_a_draw(g_s *g, boss_a_s *b, v2_i32 cam);
void boss_a_draw_post(g_s *g, boss_a_s *b, v2_i32 cam);
void boss_a_awake(g_s *g);
void boss_a_p2_init(g_s *g, boss_a_s *b);
void boss_a_on_killed_core(g_s *g, boss_a_s *b);
void boss_a_on_trigger(g_s *g, i32 trigger, boss_a_s *b);
void boss_a_segments_constrain(boss_a_segment_s *segs, i32 num, i32 loops, i32 l);

void boss_a_plant_set(g_s *g, boss_a_s *b, i32 state);
void boss_a_plant_animate(g_s *g, boss_a_s *b);
void boss_a_plant_draw(g_s *g, boss_a_s *b, v2_i32 cam);

#endif