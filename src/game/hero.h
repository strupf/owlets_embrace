// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "rope.h"

enum hero_inp {
        HERO_INP_LEFT     = 0x01,
        HERO_INP_RIGHT    = 0x02,
        HERO_INP_UP       = 0x04,
        HERO_INP_DOWN     = 0x08,
        HERO_INP_JUMP     = 0x10,
        HERO_INP_USE_ITEM = 0x20,
};

typedef struct {
        int x; // placeholder
} heroitem_s;

struct hero_s {
        objhandle_s obj;
        i32         jumpticks;
        i32         edgeticks;
        int         inp;  // input mask
        int         inpp; // input mask previous frame
        bool32      wasgrounded;
        i32         vel_q8_prev;

        heroitem_s items[16];

        objhandle_s hook;
        rope_s      rope;
        int         pickups;
};

obj_s *hero_create(game_s *g, hero_s *h);
void   hero_update(game_s *g, obj_s *o, hero_s *h);
void   hero_check_level_transition(game_s *g, obj_s *hero);
void   hero_pickup_logic(game_s *g, hero_s *h, obj_s *o);
void   hero_interact_logic(game_s *g, hero_s *h, obj_s *o);

#endif