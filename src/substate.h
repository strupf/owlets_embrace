// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _SUBSTATE_H
#define _SUBSTATE_H

#include "gamedef.h"
#include "textbox.h"

enum {
    SUBSTATE_NONE,
    SUBSTATE_RESPAWN,
    SUBSTATE_UPGRADE,
    SUBSTATE_TRANSITION,
    SUBSTATE_TEXTBOX,
    SUBSTATE_SHOP,
    SUBSTATE_MAINMENU_FADE_IN,
    SUBSTATE_FREEZE,
};

typedef struct {
    int tick;
    int phase;
} respawn_s;

typedef struct {
    int phase;
    int t;
    int upgrade;
} upgrade_s;

typedef struct {
    int    fade_phase;
    int    fade_tick;
    int    type;
    int    hero_face;
    v2_i32 hero_v;
    v2_i32 hero_feet;
    int    teleport_ID;
    char   to_load[64];
    int    dir;
} transition_s;

typedef struct {
    int          state;
    //
    transition_s transition;
    upgrade_s    upgrade;
    respawn_s    respawn;
    textbox_s    textbox;
    int          mainmenu_tick;
    int          freeze_tick;
} substate_s;

bool32 substate_blocks_gameplay(substate_s *st);
bool32 substate_finished(substate_s *st);
void   substate_update(game_s *g, substate_s *st);
void   substate_draw(game_s *g, substate_s *st, v2_i32 cam);
//
void   substate_transition_teleport(game_s *g, substate_s *st, const char *map, v2_i32 hero_feet);
bool32 substate_transition_try_hero_slide(game_s *g, substate_s *st);
void   substate_upgrade_collected(game_s *g, substate_s *st, int upgrade);
void   substate_respawn(game_s *g, substate_s *st);
void   substate_load_textbox(game_s *g, substate_s *st, const char *filename);
void   substate_freeze(substate_s *st, int ticks);

#endif