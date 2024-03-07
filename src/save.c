// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

bool32 hero_has_upgrade(game_s *g, int ID)
{
    save_s *hs = &g->save;
    return hs->upgrades[ID];
}

void hero_add_upgrade(game_s *g, int ID)
{
    save_s *hs       = &g->save;
    hs->upgrades[ID] = 1;
}

void hero_rem_upgrade(game_s *g, int ID)
{
    save_s *hs       = &g->save;
    hs->upgrades[ID] = 0;
}

void hero_set_name(game_s *g, const char *name)
{
    save_s *hs = &g->save;
    str_cpysb(hs->name, name);
}

char *hero_get_name(game_s *g)
{
    save_s *hs = &g->save;
    return &hs->name[0];
}

void hero_inv_add(game_s *g, int ID, int n)
{
    save_s *hs = &g->save;
    hs->items[ID].n += n;
}

void hero_inv_rem(game_s *g, int ID, int n)
{
    save_s *hs = &g->save;
    assert(n <= hero_inv_count_of(g, ID));
    hs->items[ID].n -= n;
}

int hero_inv_count_of(game_s *g, int ID)
{
    save_s *hs = &g->save;
    return hs->items[ID].n;
}

void saveID_put(game_s *g, u32 ID)
{
    save_s *hs = &g->save;
    assert(hs->n_saveIDs < ARRLEN(hs->saveIDs));
    hs->saveIDs[hs->n_saveIDs++] = ID;
}

bool32 saveID_has(game_s *g, u32 ID)
{
    save_s *hs = &g->save;
    for (int n = 0; n < hs->n_saveIDs; n++) {
        if (hs->saveIDs[n] == ID) return 1;
    }
    return 0;
}