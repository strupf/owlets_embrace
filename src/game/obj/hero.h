// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "game/gamedef.h"
#include "game/rope.h"

enum hero_inp {
        HERO_INP_LEFT     = 0x01,
        HERO_INP_RIGHT    = 0x02,
        HERO_INP_UP       = 0x04,
        HERO_INP_DOWN     = 0x08,
        HERO_INP_JUMP     = 0x10,
        HERO_INP_USE_ITEM = 0x20,
};

enum hero_item {
        HERO_ITEM_HOOK,
        HERO_ITEM_BOW,
        HERO_ITEM_BOMB,
        HERO_ITEM_SWORD,
        //
        NUM_HERO_ITEMS,
};

struct hero_s {
        objhandle_s obj;
        i32         jumpticks;
        i32         edgeticks;
        flags32     inp;  // input mask
        flags32     inpp; // input mask previous frame
        bool32      wasgrounded;

        i32 swordticks;
        int sworddir;

        bool32 aquired_item[NUM_HERO_ITEMS];
        int    c_item;
        int    n_items;

        objhandle_s hook;
        rope_s      rope;
        int         pickups;
};

obj_s *hero_create(game_s *g, hero_s *h);
void   hero_update(game_s *g, obj_s *o, void *arg);

#endif