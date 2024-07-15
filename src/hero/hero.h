// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "hero_air.h"
#include "hero_animate.h"
#include "hero_ground.h"
#include "hero_ladder.h"
#include "hero_misc.h"
#include "hero_swim.h"
#include "obj.h"
#include "rope.h"

enum {
    HERO_STATE_GROUND,
    HERO_STATE_AIR,
    HERO_STATE_LADDER,
    HERO_STATE_SWIMMING,
    HERO_STATE_DEAD,
};

enum {
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_HOOK_LONG,
    HERO_UPGRADE_SPRINT,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_WALLJUMP,
    HERO_UPGRADE_WALLCLIMB,
    HERO_UPGRADE_FLY,
    HERO_UPGRADE_LAMP,
    HERO_UPGRADE_SHIELD,
    HERO_UPGRADE_WHIRLWIND,
    //
    NUM_HERO_UPGRADES = 32
};

enum {
    HERO_ITEM_HOOK,
    HERO_ITEM_WEAPON,
};

#define HERO_HEIGHT                 26
#define HERO_HEIGHT_CRAWL           16
#define HERO_DRAG_Y                 256
#define HERO_GLIDE_VY               200
#define HERO_SPRINT_TICKS           60 // ticks walking until sprinting
#define HERO_VX_WALK                640
#define HERO_VX_SPRINT              900
#define HERO_VY_BOOST_SPRINT_ABS    0   // absolute vy added to a jump sprinted
#define HERO_VX_BOOST_SPRINT_Q8     300 // vx multiplier when jumping sprinted
#define HERO_VX_BOOST_SLIDE_Q8      370 // vx multiplier when jumping slided
#define HERO_DRAG_SLIDING           250
#define HERO_REEL_RATE              30
#define HERO_ROPEWALLJUMP_TICKS     30
#define HERO_SWIM_TICKS             50 // duration of swimming without upgrade
#define HERO_RUNUP_TICKS            50
#define HERO_ATTACK_TICKS           18
#define HERO_SPRINT_DTAP_TICKS      20
#define HEROHOOK_N_HIST             4
#define HERO_BREATH_TICKS           200
#define HERO_GRAVITY                80
#define HERO_GRAVITY_LOW            50
#define HERO_LOW_GRAV_TICKS         80
#define HERO_TICKS_PER_JUMP_UPGRADE 30

typedef struct {
    i32 vy;
    i32 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    i32 v0;    // "jetpack" velocity, goes to v1 over ticks or less
    i32 v1;
} hero_jumpvar_s;

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND,
    HERO_JUMP_FLY,
    HERO_JUMP_WALL,
    //
    NUM_HERO_JUMP
};

extern const hero_jumpvar_s g_herovar[NUM_HERO_JUMP];

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

#define JUMP_UI_TICKS_HIDE 12

typedef struct hero_s {
    obj_handle_s interactable;
    obj_handle_s hook;
    //
    bool8        carrying;
    bool8        was_diving;
    bool8        trys_lifting;
    bool8        sliding;
    bool8        is_idle;
    bool8        diving;
    bool8        onladder;
    bool8        reel_in;
    bool8        crawling;
    bool8        crawlingp;
    bool8        hook_pr;
    bool8        item_only_hook;
    bool8        hook_cancel_tick;
    bool8        jump_ui_may_hide;
    u8           jump_ui_collected_tick;
    u8           jump_ui_fade_out;
    u8           attack_hold_tick;
    u8           attack_tick;
    u8           attack_flipflop;
    u8           attack_hold_frame; // used for animating
    i8           grabbingp;
    i8           grabbing;
    u8           ground_impact_ticks;
    u8           edgeticks;
    u8           jump_index; // index into jump parameter table
    u8           jump_btn_buffer;
    i16          flytime_added;
    i16          flytime;
    i16          low_grav_ticks;
    i16          low_grav_ticks_0;
    u16          swimticks;
    i32          lifting_tick;
    i32          crawling_to_stand;
    u32          b_hold_tick;
    i32          sprint_ticks;
    i32          sprint_dtap;
    i32          idle_ticks;
    i32          idle_anim;
    i32          breath_ticks;
    i32          ropewalljump_dir;
    i32          jumpticks;
    i32          ladderx;
    i32          reel_in_dtap;
    i32          jump_boost_tick;
    u32          jump_fly_snd_iID;
} hero_s;

obj_s *hero_create(game_s *g);
void   hero_on_squish(game_s *g, obj_s *o);
void   hero_check_rope_intact(game_s *g, obj_s *o);
void   hero_hurt(game_s *g, obj_s *o, i32 damage);
void   hero_kill(game_s *g, obj_s *o);
i32    hero_determine_state(game_s *g, obj_s *o, hero_s *h);
bool32 hero_unhook(game_s *g, obj_s *o);
i32    hero_max_rope_len_q4(game_s *g);
bool32 hero_stand_up(game_s *g, obj_s *o);
void   hero_start_jump(game_s *g, obj_s *o, i32 ID);
//
void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);
void   hero_flytime_update_ui(game_s *g, obj_s *ohero, i32 amount); // swaps "temporary" flytime to regular flytime -> UI
void   hero_flytime_modify(game_s *g, obj_s *ohero, i32 dt);
void   hero_flytime_add_ui(game_s *g, obj_s *ohero, i32 dt); // add with UI animation
i32    hero_flytime_left(game_s *g, obj_s *ohero);
i32    hero_flytime_ui_full(game_s *g, obj_s *ohero);
i32    hero_flytime_ui_added(game_s *g, obj_s *ohero);
i32    hero_flytime_max(game_s *g, obj_s *ohero);
bool32 hero_present_and_alive(game_s *g, obj_s **ohero);
void   hero_item_activate(game_s *g, obj_s *o);

#endif