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
    HERO_STATE_AIR_HOOK,
    HERO_STATE_DEAD,
};

enum {

    HERO_UPGRADE_WHIP,
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_LONG_HOOK,
    HERO_UPGRADE_SPRINT,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_GLIDE,
    HERO_UPGRADE_WALLJUMP,
    HERO_UPGRADE_AIR_JUMP_1,
    HERO_UPGRADE_AIR_JUMP_2,
    HERO_UPGRADE_AIR_JUMP_3,
    //
    NUM_HERO_UPGRADES = 32
};

enum {
    HERO_ATTACK_NONE,
    HERO_ATTACK_SIDE,
    HERO_ATTACK_UP,
    HERO_ATTACK_DOWN,
};

typedef struct {
    v2_i32 p;
    v2_i32 pp;
} hook_pt_s;

#define ROPE_VERLET_IT   20
#define ROPE_VERLET_N    32
#define ROPE_VERLET_GRAV 40

typedef struct {
    char   name[LEN_HERO_NAME];
    bool8  upgrades[NUM_HERO_UPGRADES];
    rope_s rope;
    bool32 rope_active;

    int n_airjumps;

    int      health;
    int      n_hitbox; // only for debugging
    hitbox_s hitbox_def[4];

    obj_handle_s interactable;
    int          gold;
    int          gold_added;
    int          gold_added_ticks;
    int          hero_spawn_x;
    int          hero_spawn_y;

    int          n_obj_following;
    obj_handle_s obj_following[16];

    hook_pt_s hookpt[ROPE_VERLET_N];
} herodata_s;

typedef struct {
    int    sprint_ticks;
    bool32 sprinting;
    //
    int    whip_ticks;
    int    whip_count; // alternates between 0 and 1
    //
    i32    walljumpticks;

    i32    ropewalljump_dir;
    bool32 carrying;
    i32    swimticks;
    bool32 gliding;
    bool32 sliding;
    int    walking_ticks;
    int    ground_impact_ticks;
    int    attackbuffer;
    int    jump_btn_buffer;
    int    airjumps_left;
    int    jump_index; // index into jump parameter table
    i32    jumpticks;
    i32    edgeticks;
    bool32 onladder;
    int    ladderx;
    v2_i32 jumped_at;
} hero_s;

static_assert(sizeof(hero_s) <= 256, "");

obj_s *hero_create(game_s *g);
void   hero_on_update(game_s *g, obj_s *o);
void   hero_on_squish(game_s *g, obj_s *o);
void   hero_on_animate(game_s *g, obj_s *o);
void   hero_check_rope_intact(game_s *g, obj_s *o);
void   hero_hurt(game_s *g, obj_s *o, int damage);
void   hero_kill(game_s *g, obj_s *o);
int    hero_determine_state(game_s *g, obj_s *o, hero_s *h);
//
void   hook_on_animate(game_s *g, obj_s *o);
void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);

#endif