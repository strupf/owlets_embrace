// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_comp_find_update(g_s *g, cs_s *cs);
void cs_comp_find_draw(g_s *g, cs_s *cs, v2_i32 cam);
//
void cs_powerup_update(g_s *g, cs_s *cs);
void cs_powerup_draw(g_s *g, cs_s *cs, v2_i32 cam);
//
void cs_maptransition_update(g_s *g, cs_s *cs);
void cs_maptransition_draw(g_s *g, cs_s *cs, v2_i32 cam);
//
void cs_gameover_update(g_s *g, cs_s *cs);
void cs_gameover_draw(g_s *g, cs_s *cs, v2_i32 cam);
//
void cs_resetsave_update(g_s *g, cs_s *cs);
void cs_resetsave_draw(g_s *g, cs_s *cs, v2_i32 cam);
//
void cs_demo_1_update(g_s *g, cs_s *cs);
void cs_demo_1_draw(g_s *g, cs_s *cs, v2_i32 cam);

void cs_reset(g_s *g)
{
    cs_s *cs = &g->cuts;
    mclr(cs, sizeof(cs_s));
}

b32 cs_wait_and_pause_for_hero_idle(g_s *g)
{
    obj_s *ohero = obj_get_hero(g);
    if (!ohero) return 1;
    if (!obj_grounded(g, ohero)) return 0;
    return (ohero->v_q8.x == 0);
}