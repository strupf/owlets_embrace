// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "obj.h"
#include "rope.h"

#define HERO_BREATH_TICKS 200

enum {
    HERO_HOOK_START_AIMING   = 1,
    HERO_HOOK_AIMING         = 2,
    HERO_HOOK_JUST_THROWN    = 3,
    HERO_HOOK_JUST_DESTROYED = 4,
};

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

typedef struct {
    v2_i32 p;
    v2_i32 pp;
} hook_pt_s;

#define ROPE_VERLET_IT   20
#define ROPE_VERLET_N    32
#define ROPE_VERLET_GRAV 40

enum {
    INVENTORY_ID_GOLD,
    INVENTORY_ID_KEY_CIRCLE,
    INVENTORY_ID_KEY_SQUARE,
    INVENTORY_ID_KEY_TRIANGLE,
};

typedef struct {
    char name[64];
    char desc[256];
} inventory_item_desc_s;
// extern const inventory_item_desc_s g_item_desc[INVENTORY_NUM_ITEMS];

typedef struct {
    rope_s       rope;
    bool32       rope_active;
    i32          n_airjumps;
    i32          n_hitbox; // only for debugging
    hitbox_s     hitbox_def[4];
    obj_handle_s interactable;
    i32          n_obj_following;
    obj_handle_s obj_following[16];
    hook_pt_s    hookpt[ROPE_VERLET_N];

    i32    sprint_ticks;
    bool32 sprinting;
    i32    walljumpticks;
    i32    runup_tick;
    i32    hook_aiming_ticks;
    bool32 diving;
    i32    breath_ticks;
    i32    ropewalljump_dir;
    bool32 carrying;
    i32    swimticks;
    bool32 gliding;
    bool32 sliding;
    i32    walking_ticks;
    i32    ground_impact_ticks;
    i32    attackbuffer;
    i32    jump_btn_buffer;
    i32    airjumps_left;
    i32    jump_index; // index into jump parameter table
    i32    jumpticks;
    i32    edgeticks;
    bool32 onladder;
    i32    ladderx;
    v2_i32 jumped_at;
} hero_s;

obj_s *hero_create(game_s *g);
void   hero_on_update(game_s *g, obj_s *o, inp_s inp);
void   hero_on_squish(game_s *g, obj_s *o);
void   hero_on_animate(game_s *g, obj_s *o);
void   hero_check_rope_intact(game_s *g, obj_s *o);
void   hero_hurt(game_s *g, obj_s *o, int damage);
void   hero_kill(game_s *g, obj_s *o);
int    hero_determine_state(game_s *g, obj_s *o, hero_s *h);
bool32 hero_is_submerged(game_s *g, obj_s *o, int *water_depth);
int    hero_breath_tick(game_s *g);
//
void   hook_on_animate(game_s *g, obj_s *o);
void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);

#endif