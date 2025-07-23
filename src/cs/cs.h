// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CS_H
#define CS_H

#include "gamedef.h"

void   cs_powerup_enter(g_s *g);
void   cs_intro_comp_1_enter(g_s *g);
void   cs_demo_1_enter(g_s *g);
void   cs_demo_2_enter(g_s *g);
void   cs_demo_3_enter(g_s *g);
void   cs_resetsave_enter(g_s *g);
void   cs_gameover_enter(g_s *g);
bool32 cs_maptransition_try_slide_enter(g_s *g);
void   cs_maptransition_teleport(g_s *g, u32 map_hash, v2_i32 pos);
void   cs_explain_hook_enter(g_s *g);
void   cs_bossplant_intro_enter(g_s *g);
void   cs_bossplant_outro_enter(g_s *g);
void   cs_finding_comp_enter(g_s *g);
void   cs_finding_hook_enter(g_s *g);
void   cs_aquire_heartpiece_enter(g_s *g, bool32 is_stamina);
void   cs_mole_1_enter(g_s *g);

enum {
    CS_ID_NONE,
    CS_ID_COMP_FIND,
    CS_ID_BOSS,
    CS_ID_POWERUP,
    CS_ID_GAMEOVER,
    CS_ID_MAPTRANSITION,
    CS_ID_RESETSAVE,
    CS_ID_DIALOG_SIMPLE,
    CS_ID_DEMO_1,
};

#define CS_MEM_BYTES 256

typedef struct cs_s cs_s;
struct cs_s {
    i32    tick;
    u16    ID;
    u16    phase;
    u8     counter0;
    u8     counter1;
    u8     counter2;
    u8     counter3;
    obj_s *p_comp;
    obj_s *p_hero;
    obj_s *p_o[4];

    void (*on_trigger)(g_s *g, cs_s *cs, i32 trigger);
    void (*on_update)(g_s *g, cs_s *cs);
    void (*on_draw)(g_s *g, cs_s *cs, v2_i32 cam);
    void (*on_draw_bh_terrain)(g_s *g, cs_s *cs, v2_i32 cam);
    void (*on_draw_background)(g_s *g, cs_s *cs, v2_i32 cam);
    ALIGNAS(8)
    byte mem[CS_MEM_BYTES];
};

void cs_reset(g_s *g);
b32  cs_wait_and_pause_for_hero_idle(g_s *g);

#endif