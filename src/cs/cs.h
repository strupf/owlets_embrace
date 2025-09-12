// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CS_H
#define CS_H

#include "gamedef.h"
#include "obj/puppet.h"

typedef struct {
    v2_i32 pt[24];
    b32    circ;
    i32    n_pt;
    bool32 controllable;
} cs_camera_pan_config_s;

void   cs_intro_enter(g_s *g);
void   cs_powerup_enter(g_s *g);
void   cs_intro_comp_1_enter(g_s *g);
void   cs_demo_1_enter(g_s *g);
void   cs_demo_2_enter(g_s *g);
void   cs_resetsave_enter(g_s *g);
void   cs_gameover_enter(g_s *g);
bool32 cs_maptransition_try_slide_enter(g_s *g);
void   cs_maptransition_teleport(g_s *g, u8 *map_name, v2_i32 pos);
void   cs_explain_hook_enter(g_s *g);
void   cs_bossplant_intro_enter(g_s *g);
void   cs_bossplant_outro_enter(g_s *g);
void   cs_finding_comp_enter(g_s *g);
void   cs_finding_hook_enter(g_s *g);
void   cs_aquire_heartpiece_enter(g_s *g, bool32 is_stamina);
void   cs_mole_1_enter(g_s *g);
void   cs_on_save_enter(g_s *g);
void   cs_on_load_enter(g_s *g);
void   cs_on_load_title_wakeup(g_s *g);
void   cs_camera_pan_enter(g_s *g, cs_camera_pan_config_s *pan_config);

#define CS_MEM_BYTES 256

typedef struct cs_s cs_s;
struct cs_s {
    i32    tick;
    i32    phase;
    i32    counter0;
    i32    counter1;
    i32    counter2;
    i32    counter3;
    void  *heap;
    obj_s *p_comp;
    obj_s *p_owl;
    obj_s *p_o[4];

    void (*on_skip)(g_s *g, cs_s *cs);
    void (*on_trigger)(g_s *g, cs_s *cs, i32 trigger);
    void (*on_update)(g_s *g, cs_s *cs, inp_s inp);
    void (*on_draw)(g_s *g, cs_s *cs, v2_i32 cam);
    void (*on_draw_bh_terrain)(g_s *g, cs_s *cs, v2_i32 cam);
    void (*on_draw_background)(g_s *g, cs_s *cs, v2_i32 cam);
    ALIGNAS(32)
    byte mem[CS_MEM_BYTES];
};

void cs_reset(g_s *g);
b32  cs_wait_and_pause_for_owl_idle(g_s *g);

#endif