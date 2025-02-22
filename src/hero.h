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
    HERO_UPGRADE_SPEAR,
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

enum {
    HERO_HOOK_B_HOLD,
    HERO_HOOK_B_TIMED,
    HERO_HOOK_AIR,
    HERO_HOOK_AIR_HOLD,
    HERO_HOOK_SWITCH,
};

enum {
    HERO_ATTACK_NONE,
    HERO_ATTACK_GROUND,
    HERO_ATTACK_AIR,
};

enum {
    INVENTORY_ID_GOLD,
    INVENTORY_ID_KEY_CIRCLE,
    INVENTORY_ID_KEY_SQUARE,
    INVENTORY_ID_KEY_TRIANGLE,
};

enum {
    SWIMMING_SURFACE = 1,
    SWIMMING_DIVING  = 2,
};

#define HERO_LEN_INPUT_BUF             32
#define HERO_SQUISH_TICKS              35
#define HERO_CLIMB_Y1_OFFS             2 // from top downwards
#define HERO_CLIMB_Y2_OFFS             4 // from bottom upwards
#define HERO_B_HOLD_TICKS_HOOK         10
#define HERO_TICKS_SPIN_ATTACK         22
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
#define HERO_ATTACK_TICKS_AIR          18
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
#define HERO_STOMP_LANDING_TICKS       12
#define HERO_HURT_TICKS                50
#define HERO_INVINCIBILITY_TICKS       50
#define HERO_W_STOMP_ADD_SYMM          4

typedef struct {
    b8 started;
    u8 progress;
    u8 ticks_in;
    u8 ticks_out;
} ui_itemswap_s;

#define UI_ITEMSWAP_TICKS         50
#define UI_ITEMSWAP_TICKS_POP_UP  25
#define UI_ITEMSWAP_TICKS_FADE    30
#define UI_ITEMSWAP_TICKS_FADE_IN 30
#define UI_ITEMSWAP_TICKS_DENY    20

void ui_itemswap_start(g_s *g);
void ui_itemswap_set_progress(g_s *g, i32 p);
void ui_itemswap_exit(g_s *g);
void ui_itemswap_update(g_s *g);

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND,
    HERO_JUMP_GROUND_BOOSTED,
    HERO_JUMP_FLY,
    HERO_JUMP_WALL,
    //
    NUM_HERO_JUMP
};

typedef struct {
    i16 vy;
    i16 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    u8  v0;    // "jetpack" velocity, goes to v1 over ticks or less
    u8  v1;
} hero_jumpvar_s;

extern const hero_jumpvar_s g_herovar[NUM_HERO_JUMP];

typedef struct {
    obj_handle_s h;
    bool32       stomped;
} jumpstomped_s;

typedef struct hero_s {
    u32 upgrades;
    u32 charms;
    u32 idle_ticks;
    u32 idle_anim;
    i32 crank_acc_q16;
    i32 hook_aim_dir;
    i32 hook_aim_mode_tick;
    i32 hook_aim;
    i32 hook_aim_crank_buildup;
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
    u8  state_prev;
    u8  state_curr;
    i8  spinattack; // 0< active, <0 blocked/cooldown
    b8  ropepulled;
    u8  gliding;
    i8  grabbing; // -1/+1 = grabbing static, -2/+2 = push/pull
    b8  is_idle;
    u8  swimming; // SWIMMING_SURFACE, SWIMMING_DIVING
    u8  squish;
    b8  climbing;
    u8  dead_bounce;
    u8  ladder; // while climbing: 1 if on ladder, 0 if on wall
    b8  jump_ui_may_hide;
    b8  sprint;
    u8  impact_ticks;
    i8  walljump_tick; // signed ticks -> direction
    u8  crouch_standup;
    i8  crouched; // 0< sitting, <0 crawling
    i8  stomp;    // 0< active, <0 landing animation
    b8  holds_spear;
    u8  swimsfx_delay;
    u8  invincibility_ticks;
    u8  skidding;
    u8  b_hold_tick;
    b8  ungrappled;
    u8  attack_ID;
    u8  attack_tick;
    u8  attack_flipflop;
    u8  edgeticks;
    u8  jump_index; // index into jump parameter table
    u8  jumpticks;
    u8  low_grav_ticks;
    u8  low_grav_ticks_0;
    u8  hurt_ticks;
    u8  n_jumpstomped;
    u8  stamina_upgrades;
    u8  n_ibuf;
    u8  ibuf[HERO_LEN_INPUT_BUF];
    u8  name[LEN_HERO_NAME];

    obj_handle_s  obj_grabbed;
    jumpstomped_s jumpstomped[HERO_NUM_JUMPED_ON];
} hero_s;

obj_s *hero_create(g_s *g);
i32    hero_special_input(g_s *g, obj_s *o, inp_s inp);
void   hero_on_update(g_s *g, obj_s *o, inp_s inp);
void   hero_post_update(g_s *g, obj_s *o, inp_s inp);
void   hero_on_animate(g_s *g, obj_s *o);
i32    hero_get_actual_state(g_s *g, obj_s *o);
void   hero_on_squish(g_s *g, obj_s *o);
void   hero_check_rope_intact(g_s *g, obj_s *o);
void   hero_hurt(g_s *g, obj_s *o, i32 damage);
void   hero_stamina_update_ui(obj_s *o, i32 amount); // swaps "temporary" stamina to regular stamina -> UI
i32    hero_stamina_modify(obj_s *o, i32 dt);
void   hero_stamina_add_ui(obj_s *o, i32 dt); // add with UI animation
i32    hero_stamina_left(obj_s *o);
i32    hero_stamina_ui_full(obj_s *o);
i32    hero_stamina_ui_added(obj_s *o);
i32    hero_stamina_max(obj_s *o);
bool32 hero_present_and_alive(g_s *g, obj_s **o);
void   hero_action_throw_grapple(g_s *g, obj_s *o, i32 ang_q16, i32 vel);
bool32 hero_action_ungrapple(g_s *g, obj_s *o);
obj_s *hero_interactable_available(g_s *g, obj_s *o);
i32    hero_breath_tick(obj_s *o);
i32    hero_breath_tick_max(g_s *g);
i32    hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy);
i32    hero_swim_frameID(i32 animation);
i32    hero_swim_frameID_idle(i32 animation);
v2_i32 hero_hook_aim_dir(hero_s *h);
void   hero_stomped_ground(g_s *g, obj_s *o);
void   hero_walljump(g_s *g, obj_s *o, i32 dir);
bool32 hero_stomping(obj_s *o);
i32    hero_register_jumpstomped(obj_s *ohero, obj_s *o, bool32 stomped);
i32    hero_can_grab(g_s *g, obj_s *o, i32 dirx);
bool32 hero_has_upgrade(g_s *g, i32 ID);
void   hero_add_upgrade(g_s *g, i32 ID);
void   hero_rem_upgrade(g_s *g, i32 ID);
void   hero_set_name(g_s *g, const char *name);
char  *hero_get_name(g_s *g);
void   hero_inv_add(g_s *g, i32 ID, i32 n);
void   hero_inv_rem(g_s *g, i32 ID, i32 n);
i32    hero_inv_count_of(g_s *g, i32 ID);
void   hero_ungrab(g_s *g, obj_s *o);

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

// if button was pressed and released max frames_ago
bool32 hero_ibuf_tap(hero_s *h, i32 b, i32 frames_ago);

// if button was pressed max frames_ago
bool32 hero_ibuf_pressed(hero_s *h, i32 b, i32 frames_ago);
#endif