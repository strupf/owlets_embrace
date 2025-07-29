// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_reset(g_s *g)
{
    cs_s *cs = &g->cs;
    mclr(cs, sizeof(cs_s));
}

b32 cs_wait_and_pause_for_owl_idle(g_s *g)
{
    obj_s *owl = obj_get_owl(g);
    grapplinghook_destroy(g, &g->ghook);
    if (!owl) return 1;
    if (!obj_grounded(g, owl)) return 0;
    return (owl->v_q12.x == 0);
}