// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "scripts.h"

typedef struct {
    i32 x;
} script_golem_s;

void script_golem(g_s *g, void *arg);

void script_golem_init(g_s *g)
{
    script_golem_s   *s = game_alloct(g, script_golem_s);
    game_event_obs_s *l = game_event_obs_add(g);
    l->arg              = s;
    l->func             = script_golem;
}

void script_golem_ev(g_s *g, void *arg, i32 e)
{
}

void script_golem(g_s *g, void *arg)
{
}
