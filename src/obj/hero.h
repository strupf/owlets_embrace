// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "inventory.h"
#include "rope.h"

enum {
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_WHIP,
    HERO_UPGRADE_HIGH_JUMP,
    HERO_UPGRADE_LONG_HOOK,
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_AIR_JUMP_1,
    HERO_UPGRADE_AIR_JUMP_2,
    HERO_UPGRADE_AIR_JUMP_3,
};

enum { // aquired automatically by setting upgrades
    HERO_ITEM_HOOK,
    HERO_ITEM_WHIP,
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
    rope_s  rope;

    int         n_airjumps;
    int         selected_item;
    bool32      itemselection_decoupled;
    int         item_angle;
    inventory_s inventory;

    int      health;
    int      n_hitbox; // only for debugging
    hitbox_s hitbox_def[4];
} herodata_s;

typedef struct {
    int attackbuffer;
    int jump_btn_buffer;
    int airjumps_left;
    int jump_index; // index into jump parameter table
    i32 jumpticks;
    i32 edgeticks;
    i32 swimticks;
} hero_s;

obj_s *hero_create(game_s *g);
void   hero_on_update(game_s *g, obj_s *o);
void   hero_on_squish(game_s *g, obj_s *o);
void   hero_on_animate(game_s *g, obj_s *o);
void   hook_on_animate(game_s *g, obj_s *o);
bool32 hero_has_upgrade(herodata_s *h, int upgrade);
void   hero_aquire_upgrade(herodata_s *h, int upgrade);
void   hook_update(game_s *g, obj_s *hook);
void   hero_crank_item_selection(herodata_s *h);
void   hero_check_rope_intact(game_s *g, obj_s *o);
void   hero_hurt(game_s *g, obj_s *o, herodata_s *h, int damage);

#endif