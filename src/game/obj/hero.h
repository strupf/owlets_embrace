// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

typedef struct hero_s hero_s;

#include "game/obj.h"
#include "game/rope.h"

enum {
        HERO_ANIM_IDLE,
        HERO_ANIM_WALKING,
        HERO_ANIM_LAND,
};

enum {
        HERO_ITEM_BOW,
        HERO_ITEM_HOOK,
        HERO_ITEM_SWORD,
        HERO_ITEM_BOMB,
        //
        NUM_HERO_ITEMS,
};

enum {
        HERO_WIDTH          = 8,
        HERO_HEIGHT         = 24,
        HERO_ROPE_MIN       = 50 << 16,
        HERO_ROPE_MAX       = 250 << 16,
        HERO_ROPE_REEL_RATE = 150000,
        HERO_C_JUMP_INIT    = 700,
        HERO_C_ACCX_MAX     = 135,
        HERO_C_JUMP_MAX     = 80,
        HERO_C_JUMP_MIN     = 0,
        HERO_C_JUMPTICKS    = 12,
        HERO_C_EDGETICKS    = 6,
        HERO_C_GRAVITY      = 55,
        HERO_C_WHIP_TICKS   = 10,
};

enum {
        HERO_STATE_GROUND   = 0x001,
        HERO_STATE_LADDER   = 0x002,
        HERO_STATE_ITEM     = 0x004,
        HERO_STATE_CARRYING = 0x008,
};

struct hero_s {
        obj_s       o;
        int         state;
        int         anim;
        int         animstate;
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
        int         whip_ticks;
        i32         rope_len_q16;
        bool32      gliding;
        bool32      locked_facing;

        flags32 aquired_items;
        bool32  itemselection_dirty;
        int     selected_item;
        int     selected_item_prev;
        int     selected_item_next;
};

obj_s  *hero_create(game_s *g);
void    hero_update(game_s *g, obj_s *o);
bool32  hero_using_hook(obj_s *o);
bool32  hero_using_whip(obj_s *o);
rec_i32 hero_whip_hitbox(obj_s *o);
bool32  hero_has_item(hero_s *h, int itemID);
void    hero_set_cur_item(hero_s *h, int itemID);
void    hero_aquire_item(hero_s *h, int itemID);

#endif