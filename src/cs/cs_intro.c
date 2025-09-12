// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_intro_update(g_s *g, cs_s *cs, inp_s inp);

void cs_intro_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_intro_update;
}

void cs_intro_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs->tick++;
    cs_reset(g);
}
