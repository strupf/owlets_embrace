// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "inventory.h"
#include "obj.h"
#include "rope.h"

enum {
    HERO_STATE_GROUND,
    HERO_STATE_AIR,
    HERO_STATE_LADDER,
    HERO_STATE_SWIMMING,
};

enum {
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_WHIP,
    HERO_UPGRADE_HIGH_JUMP,
    HERO_UPGRADE_LONG_HOOK,
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_GLIDE,
    HERO_UPGRADE_AIR_JUMP_1,
    HERO_UPGRADE_AIR_JUMP_2,
    HERO_UPGRADE_AIR_JUMP_3,
};

enum {
    HERO_ATTACK_NONE,
    HERO_ATTACK_SIDE,
    HERO_ATTACK_UP,
    HERO_ATTACK_DOWN,
};

typedef struct {
    char    name[LEN_HERO_NAME];
    flags32 aquired_upgrades;
    rope_s  rope;
    bool32  rope_active;

    int    n_airjumps;
    int    selected_item;
    bool32 itemselection_decoupled;
    int    item_angle;

    int      health;
    int      n_hitbox; // only for debugging
    hitbox_s hitbox_def[4];

    obj_handle_s interactable;
} herodata_s;

typedef struct {
    int    sprint_ticks;
    bool32 sprinting;
    int    sprint_dir;
    //
    int    whip_ticks;
    int    whip_count; // alternates between 0 and 1
    //
    i32    walljumpticks;
    i32    runup_wall_ticks;
    i32 runup_wall;
    i32    swimticks;
    bool32 gliding;
    bool32 sliding;
    int    walking_ticks;
    int    landed_ticks;
    int    inair_ticks;
    int    ground_ticks;
    int    jumped_ticks;
    int    attackbuffer;
    int    jump_btn_buffer;
    int    airjumps_left;
    int    jump_index; // index into jump parameter table
    i32    jumpticks;
    i32    edgeticks;
    bool32 onladder;
    int    ladderx;
    v2_i32 jumped_at;

    int state_prev;
    u32 last_time_air;
    u32 last_time_ground;
    u32 last_time_ladder;
    u32 last_time_swim;
} hero_s;

static_assert(sizeof(hero_s) <= 256, "");

obj_s *hero_create(game_s *g);
void   hero_on_update(game_s *g, obj_s *o);
void   hero_on_squish(game_s *g, obj_s *o);
void   hero_on_animate(game_s *g, obj_s *o);
bool32 hero_has_upgrade(herodata_s *h, int upgrade);
void   hero_crank_item_selection(herodata_s *h);
void   hero_check_rope_intact(game_s *g, obj_s *o);
void   hero_hurt(game_s *g, obj_s *o, herodata_s *h, int damage);
int    hero_determine_state(game_s *g, obj_s *o, hero_s *h);
//
void   hook_on_animate(game_s *g, obj_s *o);
void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);

#endif