// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

typedef struct hero_s hero_s;

#include "game/obj.h"
#include "game/rope.h"

enum hero_item {
        HERO_ITEM_BOW,
        HERO_ITEM_HOOK,
        HERO_ITEM_SWORD,
        HERO_ITEM_BOMB,
        //
        NUM_HERO_ITEMS,
};

enum {
        HERO_WIDTH          = 16,
        HERO_HEIGHT         = 24,
        HERO_ROPE_MIN       = 50 << 16,
        HERO_ROPE_MAX       = 300 << 16,
        HERO_ROPE_REEL_RATE = 64,
        HERO_C_JUMP_INIT    = 700,
        HERO_C_ACCX_MAX     = 135,
        HERO_C_JUMP_MAX     = 80,
        HERO_C_JUMP_MIN     = 0,
        HERO_C_JUMPTICKS    = 12,
        HERO_C_EDGETICKS    = 6,
        HERO_C_GRAVITY      = 55,
        HERO_C_WHIP_TICKS   = 40,
};

enum hero_state_machine {
        HERO_STATE_LADDER,
        HERO_STATE_GROUND,
        HERO_STATE_AIR,
};

struct hero_s {
        objhandle_s obj;
        int         state;
        //
        bool32      hashook;
        i32         jumpticks;
        i32         edgeticks;
        bool32      wasgrounded;
        bool32      caninteract;
        bool32      onladder;
        int         ladderx;
        v2_i32      ppos;
        objhandle_s hook;
        rope_s      rope;
        int         whip_ticks;
        i32         rope_len_q16;
        char        hero_name[LEN_STR_HERO_NAME];
};

obj_s  *hero_create(game_s *g, hero_s *h);
void    hero_update(game_s *g, obj_s *o);
bool32  hero_using_hook(hero_s *h);
bool32  hero_using_whip(hero_s *h);
rec_i32 hero_whip_hitbox(hero_s *h);

#endif