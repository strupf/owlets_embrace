// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "grapplinghook.h"
#include "obj.h"
#include "rope.h"

// power and movement upgrades
enum {
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_SPRINT,
    HERO_UPGRADE_FLY,
    HERO_UPGRADE_STOMP,
    HERO_UPGRADE_CLIMB,
    HERO_UPGRADE_STOMP_WAVE,
    //
    NUM_HERO_UPGRADES = 32
};

// equippable optional charms
enum {
    HERO_CHARM_ATTRACT_COINS,
    //
};

enum {
    HERO_ST_NONE,
    HERO_ST_GROUND,
    HERO_ST_AIR,
    HERO_ST_CLIMB,
    HERO_ST_LADDER,
    HERO_ST_WATER,
    HERO_ST_STOMP,
};

enum {
    HERO_LADDER_NONE,
    HERO_LADDER_VERTICAL,
    HERO_LADDER_WALL,
};

#define HERO_HURT_LP_TICKS             100
#define HERO_WATER_THRESHOLD           18
#define HERO_WIDTH                     14
#define HERO_X_OFFS_LADDER             ((16 - HERO_WIDTH) >> 1)
#define HERO_CROUCHED_MAX_TICKS        5
#define HERO_HEIGHT_CROUCHED           16
#define HERO_HEIGHT                    26
#define HERO_VX_CRAWL                  384
#define HERO_VX_WALK                   640
#define HERO_VX_SPRINT                 896
#define HERO_VX_MAX_GROUND             1024
#define HERO_REEL_RATE                 30
#define HERO_SWIM_TICKS                50 // duration of swimming without upgrade
#define HERO_ATTACK_TICKS              18
#define HEROHOOK_N_HIST                4
#define HERO_BREATH_TICKS              200
#define HERO_GRAVITY                   80
#define HERO_GRAVITY_LOW               40
#define HERO_LOW_GRAV_TICKS            80
#define HERO_TICKS_PER_STAMINA_UPGRADE 1024
#define HERO_DISTSQ_INTERACT           POW2(80)
#define HERO_DISTSQ_PICKUP             POW2(100)
#define HERO_N_MAX_JUMPED_ON           4
#define HERO_WEAPON_DROP_TICKS         20
#define HERO_TICKS_STOMP_INIT          6
#define STAMINA_UI_TICKS_HIDE          12
#define WALLJUMP_ANIM_TICKS            10
#define WALLJUMP_MOM_TICKS             55
#define HERO_NUM_JUMPED_ON             8
#define HERO_HEALTH_RESTORE_TICKS      250
#define HERO_STOMP_LANDING_TICKS       8
#define HERO_HURT_TICKS                50
#define HERO_INVINCIBILITY_TICKS       50
#define HERO_W_STOMP_ADD_SYMM          4

typedef struct {
    i16 vy;
    i16 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    i16 v0;    // "jetpack" velocity, goes to v1 over ticks or less
    i16 v1;
} hero_jumpvar_s;

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND,
    HERO_JUMP_GROUND_BOOSTED,
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

typedef struct hero_s {
    obj_handle_s interactable;
    obj_handle_s hook;

    u8  gliding;
    b8  grabbing;
    i8  push_pull;
    b8  is_idle;
    b8  swimming;
    b8  climbing;
    b8  diving;
    u8  dead_bounce;
    u8  ladder; // while climbing: 1 if on ladder, 0 if on wall
    b8  jump_ui_may_hide;
    b8  sprint;
    u8  impact_ticks;
    u8  walljump_tech_tick;
    i8  walljump_tick; // signed ticks -> direction
    i8  touched_wall;
    u8  sprint_dtap;
    u8  crouch_standup;
    u8  crouched;
    i8  crawl; // facing sign
    u8  stomp;
    u8  stomp_landing_ticks;
    u8  holds_weapon;
    u8  swimsfx_delay;
    u8  statep;
    u8  invincibility_ticks;
    u8  skidding;
    u8  b_hold_tick;
    u8  attack_ID;
    u8  attack_hold_tick;
    u8  attack_tick;
    u8  attack_flipflop;
    u8  attack_hold_frame; // used for animating
    u8  edgeticks;
    u8  jump_index; // index into jump parameter table
    u8  jump_btn_buffer;
    u8  jumpticks;
    u8  low_grav_ticks;
    u8  low_grav_ticks_0;
    u8  hurt_ticks;
    i16 air_block_ticks;
    i16 air_block_ticks_og;
    u16 health_restore_tick;
    u16 stamina_ui_collected_tick;
    u16 stamina_ui_fade_out;
    u16 stamina_added;
    u16 stamina;
    u16 swimticks;
    i16 ladderx;
    u16 breath_ticks;
    u32 idle_ticks;
    u32 idle_anim;
    b32 aim_mode;
    i32 hook_aim_dir;
    i32 hook_aim_mode_tick;
    i32 hook_aim;
    i32 hook_aim_crank_buildup;
    u8  n_jumped_on;
    u8  n_stomped_on;
    u8  n_jumped_or_stomped_on;
    u16 coins;
    u32 charms;
    u8  stamina_upgrades;
    u8  n_map_pins;
    u8  name[LEN_HERO_NAME];
    u32 upgrades;

    obj_handle_s jumped_on[HERO_NUM_JUMPED_ON];
    obj_handle_s stomped_on[HERO_NUM_JUMPED_ON];
    obj_handle_s jumped_or_stomped_on[HERO_NUM_JUMPED_ON];
} hero_s;

obj_s *hero_create(g_s *g);
void   hero_on_update(g_s *g, obj_s *o, inp_s inp);
void   hero_post_update(g_s *g, obj_s *o, inp_s inp);
void   hero_on_animate(g_s *g, obj_s *o);
i32    hero_get_actual_state(g_s *g, obj_s *o);
void   hero_on_squish(g_s *g, obj_s *o);
void   hero_check_rope_intact(g_s *g, obj_s *o);
void   hero_hurt(g_s *g, obj_s *o, i32 damage);
bool32 hero_try_stand_up(g_s *g, obj_s *o);
void   hero_start_jump(g_s *g, obj_s *o, i32 ID);
void   hero_stamina_update_ui(g_s *g, obj_s *o, i32 amount); // swaps "temporary" stamina to regular stamina -> UI
i32    hero_stamina_modify(g_s *g, obj_s *o, i32 dt);
void   hero_stamina_add_ui(g_s *g, obj_s *o, i32 dt); // add with UI animation
i32    hero_stamina_left(g_s *g, obj_s *o);
i32    hero_stamina_ui_full(g_s *g, obj_s *o);
i32    hero_stamina_ui_added(g_s *g, obj_s *o);
i32    hero_stamina_max(g_s *g, obj_s *o);
bool32 hero_present_and_alive(g_s *g, obj_s **o);
void   hero_action_throw_grapple(g_s *g, obj_s *o, i32 ang_q16, i32 vel);
bool32 hero_action_ungrapple(g_s *g, obj_s *o);
obj_s *hero_interactable_available(g_s *g, obj_s *o);
i32    hero_breath_tick(obj_s *o);
i32    hero_breath_tick_max(g_s *g);
void   hero_restore_grounded_stuff(g_s *g, obj_s *o);
i32    hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy);
i32    hero_swim_frameID(i32 animation);
i32    hero_swim_frameID_idle(i32 animation);
v2_i32 hero_hook_aim_dir(hero_s *h);
void   hero_stomped_ground(g_s *g, obj_s *o);
void   hero_walljump(g_s *g, obj_s *o, i32 dir);
bool32 hero_stomping(obj_s *o);
i32    hero_register_jumped_on(obj_s *ohero, obj_s *o);
i32    hero_register_stomped_on(obj_s *ohero, obj_s *o);
i32    hero_can_grab(g_s *g, obj_s *o, i32 dirx);
i32    hero_item_button(g_s *g, inp_s inp, obj_s *o);
bool32 hero_has_upgrade(g_s *g, i32 ID);
void   hero_add_upgrade(g_s *g, i32 ID);
void   hero_rem_upgrade(g_s *g, i32 ID);
void   hero_set_name(g_s *g, const char *name);
char  *hero_get_name(g_s *g);
void   hero_inv_add(g_s *g, i32 ID, i32 n);
void   hero_inv_rem(g_s *g, i32 ID, i32 n);
i32    hero_inv_count_of(g_s *g, i32 ID);
void   hero_coins_change(g_s *g, i32 n);
i32    hero_coins(g_s *g);

enum {
    HERO_INTERACTION_NONE,
    HERO_INTERACTION_INTERACT,
    HERO_INTERACTION_GRAB,
    HERO_INTERACTION_UNHOOK,
};

typedef struct {
    i32          action;
    obj_handle_s interact;
} hero_interaction_s;

hero_interaction_s hero_get_interaction(g_s *g, obj_s *o);
void               hero_interaction_do(g_s *g, obj_s *o, hero_interaction_s i);

#endif