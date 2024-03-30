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

void hero_coins_change(game_s *g, i32 n)
{
    if (n == 0) return;

    i32 ct = g->save.coins + g->coins_added + n;
    if (ct < 0) return;

    if (g->coins_added == 0 || g->coins_added_ticks) {
        g->coins_added_ticks = 100;
    }
    g->coins_added += n;
}

i32 hero_coins(game_s *g)
{
    i32 c = g->save.coins + g->coins_added;
    assert(0 <= c);
    return c;
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

bool32 hero_visited_tile(game_s *g, map_worldroom_s *room, int x, int y)
{
    u32 w = max_i((room->w / SYS_DISPLAY_W) >> 3, 1);
    u8 *b = &g->save.visited_tiles[room->ID - 1][(x >> 3) + y * w];
    return (*b & (1 << (x & 7)));
}

void hero_set_visited_tile(game_s *g, map_worldroom_s *room, int x, int y)
{
    u32 w = max_i((room->w / SYS_DISPLAY_W) >> 3, 1);
    u8 *b = &g->save.visited_tiles[room->ID - 1][(x >> 3) + y * w];
    *b |= 1 << (x & 7);
}