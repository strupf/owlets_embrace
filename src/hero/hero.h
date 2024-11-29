// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_H
#define HERO_H

#include "gamedef.h"
#include "obj.h"
#include "rope.h"

enum {
    HERO_STATE_NULL = -1,
    //
    HERO_STATE_GROUND,
    HERO_STATE_CLIMB,
    HERO_STATE_AIR,
    HERO_STATE_LADDER,
    HERO_STATE_SWIMMING,
    HERO_STATE_DEAD,
};

enum {
    HERO_UPGRADE_HOOK,
    HERO_UPGRADE_SWIM,
    HERO_UPGRADE_DIVE,
    HERO_UPGRADE_SPRINT,
    HERO_UPGRADE_FLY,
    HERO_UPGRADE_STOMP,
    HERO_UPGRADE_CLIMB,
    //
    NUM_HERO_UPGRADES = 32
};

enum {
    HERO_LADDER_NONE,
    HERO_LADDER_VERTICAL,
    HERO_LADDER_WALL,
};

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
#define HERO_DISTSQ_INTERACT           POW2(40)
#define HERO_DISTSQ_PICKUP             POW2(100)
#define HERO_N_MAX_JUMPED_ON           4
#define HERO_WEAPON_DROP_TICKS         20
#define HERO_TICKS_STOMP_INIT          6
#define STAMINA_UI_TICKS_HIDE          12
#define WALLJUMP_ANIM_TICKS            10
#define WALLJUMP_MOM_TICKS             37
#define HERO_NUM_JUMPED_ON             8
#define HERO_HEALTH_RESTORE_TICKS      250

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

typedef_struct (hero_s) {
    obj_handle_s interactable;
    obj_handle_s hook;

    u8     gliding;
    b8     is_idle;
    b8     diving;
    u8     ladder_type; // while climbing: 1 if on ladder, 0 if on wall
    b8     jump_ui_may_hide;
    b8     action_jumpp;
    b8     action_jump;
    b8     dropped_weapon;
    b8     sprint;
    b8     climbing;
    u8     impact_ticks;
    u8     walljump_tech_tick;
    i8     walljump_tick; // signed ticks -> direction
    i8     touched_wall;
    i8     pushing;
    u8     sprint_dtap;
    u8     crouch_standup;
    u8     crouched;
    i8     crawl; // facing sign
    u8     stomp;
    u8     holds_weapon;
    u8     swimsfx_delay;
    u8     statep;
    u8     invincibility_ticks;
    u8     skidding;
    u8     b_hold_tick;
    u8     attack_ID;
    u8     attack_hold_tick;
    u8     attack_tick;
    u8     attack_flipflop;
    u8     attack_hold_frame; // used for animating
    u8     edgeticks;
    u8     jump_index; // index into jump parameter table
    u8     jump_btn_buffer;
    u8     jumpticks;
    u8     low_grav_ticks;
    u8     low_grav_ticks_0;
    u16    health_restore_tick;
    u16    stamina_ui_collected_tick;
    u16    stamina_ui_fade_out;
    u16    stamina_added;
    u16    stamina;
    u16    swimticks;
    i16    ladderx;
    u16    breath_ticks;
    u32    idle_ticks;
    u32    idle_anim;
    bool32 aim_mode;
    i32    hook_aim_dir;
    i32    hook_aim_mode_tick;
    i32    hook_aim;

    u16          n_jumped_on;
    u16          n_stomped_on;
    u16          n_jumped_or_stomped_on;
    obj_handle_s jumped_on[HERO_NUM_JUMPED_ON];
    obj_handle_s stomped_on[HERO_NUM_JUMPED_ON];
    obj_handle_s jumped_or_stomped_on[HERO_NUM_JUMPED_ON];
};

obj_s *hero_create(g_s *g);
void   hero_on_update(g_s *g, obj_s *o);
void   hero_on_animate(g_s *g, obj_s *o);
void   hero_handle_input(g_s *g, obj_s *o);
void   hero_on_squish(g_s *g, obj_s *o);
void   hero_check_rope_intact(g_s *g, obj_s *o);
void   hero_hurt(g_s *g, obj_s *o, i32 damage);
void   hero_kill(g_s *g, obj_s *o);
i32    hero_determine_state(g_s *g, obj_s *o, hero_s *h);
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
void   hero_action_throw_grapple(g_s *g, obj_s *o);
bool32 hero_action_ungrapple(g_s *g, obj_s *o);
void   hero_action_attack(g_s *g, obj_s *o);
void   hero_post_update(g_s *g, obj_s *o);
obj_s *hero_pickup_available(g_s *g, obj_s *o);
obj_s *hero_interactable_available(g_s *g, obj_s *o);
void   hero_start_item_pickup(g_s *g, obj_s *o);
void   hero_drop_item(g_s *g, obj_s *o);
bool32 hero_is_submerged(g_s *g, obj_s *o, i32 *water_depth);
i32    hero_breath_tick(obj_s *o);
i32    hero_breath_tick_max(g_s *g);
void   hero_restore_grounded_stuff(g_s *g, obj_s *o);
bool32 hero_is_climbing(g_s *g, obj_s *o, i32 facing);
bool32 hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy);
bool32 hero_try_snap_to_ladder(g_s *g, obj_s *o, i32 diry);
bool32 hero_rec_ladder(g_s *g, obj_s *o, rec_i32 *rout);
bool32 hero_rec_on_ladder(g_s *g, rec_i32 aabb, rec_i32 *rout);
i32    hero_swim_frameID(i32 animation);
i32    hero_swim_frameID_idle(i32 animation);
void   hero_on_stomped(g_s *g, obj_s *o);
v2_i32 hero_hook_aim_dir(hero_s *h);
void   hero_stomped_ground(g_s *g, obj_s *o);
void   hero_walljump(g_s *g, obj_s *o, i32 dir);
bool32 hero_stomping(obj_s *o);
i32    hero_register_jumped_on(obj_s *ohero, obj_s *o);
i32    hero_register_stomped_on(obj_s *ohero, obj_s *o);

#endif