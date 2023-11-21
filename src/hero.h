// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "rope.h"

#define LEN_HERO_NAME 16

enum {
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_HIGH_JUMP,
    HERO_UPGRADE_BOW,
};

enum { // aquired automatically by setting upgrades
    HERO_ITEM_HOOK,
    HERO_ITEM_BOMB,
    HERO_ITEM_BOW,
    //
    NUM_HERO_ITEMS
};

typedef struct {
    char    name[LEN_HERO_NAME];
    flags32 aquired_upgrades;
    flags32 aquired_items;
    i32     ropelen;
    rope_s  rope;

    int    selected_item_prev;
    int    selected_item;
    int    selected_item_next;
    bool32 itemselection_dirty;
} hero_s;

bool32 hero_has_upgrade(hero_s *h, int upgrade);
void   hero_aquire_upgrade(hero_s *h, int upgrade);
void   hero_update(game_s *g, obj_s *o);
void   hook_update(game_s *g, obj_s *hook);
void   hero_room_transition(game_s *g, obj_s *o);
void   hero_crank_item_selection(hero_s *h);

#endif