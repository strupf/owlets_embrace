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
    HERO_UPGRADE_WHIP,
    HERO_UPGRADE_HIGH_JUMP,
    HERO_UPGRADE_BOW,

};

enum { // aquired automatically by setting upgrades
    HERO_ITEM_HOOK,
    HERO_ITEM_WHIP,
    HERO_ITEM_BOMB,
    HERO_ITEM_BOW,

    //
    NUM_HERO_ITEMS
};

enum {
    HERO_ATTACK_NONE,
    HERO_ATTACK_SIDE,
    HERO_ATTACK_UP,
    HERO_ATTACK_DIA_UP,
    HERO_ATTACK_DIA_DOWN,
    HERO_ATTACK_DOWN,
};

typedef struct {
    char    name[LEN_HERO_NAME];
    flags32 aquired_upgrades;
    flags32 aquired_items;
    i32     ropelen;
    rope_s  rope;
    int     attack;
    int     attack_tick;
    bool32  facing_locked;
    int     selected_item;
    bool32  itemselection_decoupled;
    int     item_angle;
} hero_s;

bool32 hero_has_upgrade(hero_s *h, int upgrade);
void   hero_aquire_upgrade(hero_s *h, int upgrade);
void   hero_update(game_s *g, obj_s *o);
void   hook_update(game_s *g, obj_s *hook);
void   hero_room_transition(game_s *g, obj_s *o);
void   hero_crank_item_selection(hero_s *h);

#endif