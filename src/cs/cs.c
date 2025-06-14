// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_reset(g_s *g)
{
    cs_s *cs = &g->cuts;
    mclr(cs, sizeof(cs_s));
}

b32 cs_wait_and_pause_for_hero_idle(g_s *g)
{
    obj_s *ohero = obj_get_hero(g);
    grapplinghook_destroy(g, &g->ghook);
    if (!ohero) return 1;
    if (!obj_grounded(g, ohero)) return 0;
    return (ohero->v_q12.x == 0);
}