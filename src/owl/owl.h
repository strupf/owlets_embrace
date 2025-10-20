// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HEADER_OWL_H
#define HEADER_OWL_H

#include "gamedef.h"
#include "grapplinghook.h"
#include "hitbox.h"
#include "obj.h"
#include "wire.h"

#define OWL_USE_ALT_AIR_JUMPS           0
#define OWL_ONEWAY_PLAT_DOWN_JUST_DOWN  1 // 1: drop through platforms by simply pressing down, else down + A
#define OWL_STOMP_ONLY_WITH_COMP_ON_B   0
//
#define OWL_W                           14
#define OWL_H                           26
#define OWL_H_CROUCH                    16
#define OWL_WATER_THRESHOLD             17
#define OWL_W_STOMP_ADD_SYMM            4             // amount to add to width for easier stomping of objects
#define OWL_GRAVITY                     Q_VOBJ(0.32)  // normal gravity
#define OWL_GRAVITY_LOW                 Q_VOBJ(0.16)  // low gravity around tipping point up to down
#define OWL_GRAVITY_V_BEG               -Q_VOBJ(0.80) // v when low gravity starts
#define OWL_GRAVITY_V_END               +Q_VOBJ(0.30) // v when low gravity ends
#define OWL_GRAVITY_V_PEAK              +Q_VOBJ(0.00) // v when gravity is lowest
#define OWL_CROUCH_MAX_TICKS            5             // ticks to duck down
#define OWL_CROUCH_STANDUP_TICKS        5             // ticks to stand up
#define OWL_VX_CRAWL                    Q_VOBJ(1.5)
#define OWL_VX_WALK                     Q_VOBJ(3.0)
#define OWL_VX_CARRY                    Q_VOBJ(2.0)
#define OWL_VX_SPRINT                   Q_VOBJ(4.0)
#define OWL_VX_MAX_GROUND               Q_VOBJ(5.0)
#define OWL_VX_SWIM                     Q_VOBJ(2.0)
#define OWL_CLIMB_LADDER_SNAP_X_SYM     12 // pixel range centerx +- x to snap to a ladder
#define OWL_CLIMB_LADDER_SNAP_Y         8  // centery pixel for ladder positioning
#define OWL_AIR_STOMP_MAX               14
#define OWL_STOMP_LANDING_TICKS         10
#define OWL_JUMP_EDGE_TICKS             16
#define OWL_INTERACTABLE_DST            48
#define OWL_WALLJUMP_TICKS_BLOCK        20 // ticks to block player from moving right back to the wall
#define OWL_WALLJUMP_INIT_TICKS         4
#define OWL_WALLJUMP_DIST_PX            12
#define OWL_WALLJUMP_VX                 Q_VOBJ(3.5)
#define OWL_PUSH_TICKS_MIN              12
#define OWL_HURT_TICKS                  50
#define OWL_HURT_TICKS_SPRITE           24 // threshold of hurt ticks between hurt sprite and blinking
#define OWL_AIR_JUMP_FLAP_TICKS         8
#define OWL_AIR_JUMP_ENDED_TICKS        8
#define OWL_CARRY_PICKUP_TICKS          20
//
#define OWL_STAMINA_TICKS_UI_FADE       10
#define OWL_STAMINA_PER_CONTAINER       512
#define OWL_STAMINA_DRAIN_AIR_JUMP_INIT 128
#define OWL_STAMINA_DRAIN_AIR_JUMP_HOLD 16
#define OWL_STAMINA_DRAIN_CLIMB_STILL   2
#define OWL_STAMINA_DRAIN_CLIMB_UP      10
#define OWL_STAMINA_DRAIN_DIVE          2
#define OWL_STAMINA_RESTORE_SWIM        16
#define OWL_STAMINA_BLINK_TICKS         10

enum {
    OWL_SPECIAL_ST_EAT_FROG = 1,
    OWL_SPECIAL_ST_FLY_KNOCKBACK,
};

enum {
    OWL_UPGRADE_HOOK                = 1 << 0,
    OWL_UPGRADE_SWIM                = 1 << 1,
    OWL_UPGRADE_DIVE                = 1 << 2,
    OWL_UPGRADE_FLY                 = 1 << 3,
    OWL_UPGRADE_CLIMB               = 1 << 4,
    OWL_UPGRADE_STOMP               = 1 << 5,
    OWL_UPGRADE_POWERSTOMP          = 1 << 6,
    OWL_UPGRADE_COMPANION           = 1 << 7, // companion available
    OWL_UPGRADE_COMP_COLLECT        = 1 << 8,
    OWL_UPGRADE_COMP_FIND_HIDDEN    = 1 << 9,
    OWL_UPGRADE_COMP_ATTACK_ENEMIES = 1 << 10,
    OWL_UPGRADE_HOMING_ITEMS        = 1 << 11,
};

enum {
    OWL_CLIMB_LADDER   = 1,
    OWL_CLIMB_WALL     = 2,
    OWL_CLIMB_WALLJUMP = 3,
};

enum {
    OWL_GROUND_PUSH = 1,
    OWL_GROUND_PULL = 2
};

enum {
    OWL_JUMP_WATER,
    OWL_JUMP_GROUND,
    OWL_JUMP_GROUND_BOOSTED,
    OWL_JUMP_AIR_0,
    OWL_JUMP_AIR_1,
    OWL_JUMP_AIR_2,
    OWL_JUMP_WALL,
    OWL_JUMP_CARRY,
    //
    NUM_OWL_JUMP
};

enum {
    OWL_STANCE_GRAPPLE,
    OWL_STANCE_ATTACK
};

enum {
    OWL_SWIM_SURFACE = 1,
    OWL_SWIM_DIVE    = 2
};

enum {
    OWL_ATTACK_SIDE = 1,
    OWL_ATTACK_UP   = 2,
    OWL_ATTACK_DOWN = 3,
};

enum {
    OWL_ST_NULL,
    OWL_ST_DEAD,
    OWL_ST_GROUND,
    OWL_ST_AIR,
    OWL_ST_WATER,
    OWL_ST_CLIMB,
    OWL_ST_LAUNCHED,
};

typedef struct owl_jumpvar_s {
    ALIGNAS(8)
    i16 vy;
    i16 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    i16 v0;    // "jetpack" velocity, goes to v1 over ticks or less
    i16 v1;
} owl_jumpvar_s;

typedef struct {
    obj_handle_s h;
    bool32       stomped;
} obj_jumpstomped_s;

extern const owl_jumpvar_s g_owl_jumpvar[NUM_OWL_JUMP];

typedef struct owl_s {
    SAVED u8          name[OWL_LEN_NAME];
    SAVED u32         upgrades;
    u16               health;
    SAVED u16         health_max;
    u16               stamina;
    SAVED u16         stamina_max;   // calculated from stamina_containers
    u16               stamina_added; // how of stamina was just added (only visual)
    u8                stamina_upgrades;
    u8                stamina_added_delay_ticks;
    u8                stamina_ui_fade_ticks;
    b8                stamina_ui_show;
    u16               stamina_empty_tick;
    u8                stamina_blink_tick;
    u8                stamina_blink_tick_max;
    u8                n_air_jumps;
    u8                n_air_jumps_max;
    u8                special_state;
    u16               special_state_timer;
    b8                invincible;
    //
    i16               climb_from_x;  // used for smooting, visual only
    i16               climb_ladderx; // position on ladder
    i16               climb_anim;
    u8                climb_tick; // smooth camera movement on snapping
    u8                climb_wall_move_tick;
    u8                climb; // OWLET_CLIMB_LADDER, _WALL
    u8                climb_slide_down;
    u8                climb_move_acc; // acceleration counter
    //
    v2_i32            knockback_v;
    u16               knockback;
    u16               knockback_total;
    //
    i32               jump_snd_iID;
    i16               jump_from_y;
    i16               jump_vy0;
    i16               jump_vy1;
    u8                jump_ground_ticks;
    u8                jump_index; // index into jump parameter table
    u8                jump_edgeticks;
    u8                jump_ticks;
    u8                jump_ticks_max;
    u8                air_gliding;
    u8                air_stomp; // stomp ticks
    u8                air_walljump_ticks;
    u8                air_walljump_anim_ticks;
    //
    i16               wallj_from_x;
    i16               wallj_from_y;
    u8                wallj_ticks;
    u8                jump_anim_ticks;
    //
    i32               ground_anim; // animation timer for pushing, crawling, walking, idle
    i32               ground_pull_wire_anim;
    i8                ground_push_tick; // [-96, +96], tick for pushing left (-) or right (+)
    u8                ground_push_pull; // OWL_GROUND_PUSH_NONE, OWL_GROUND_PUSH,  OWL_GROUND_PULL
    u8                ground;
    u8                ground_skid_ticks;             // ticks cancel sprinting
    u8                ground_impact_ticks;           // ticks just landed
    i8                ground_sprint_doubletap_ticks; // signed for direction of double tap
    u8                ground_stomp_landing_ticks;
    u8                ground_crouch_standup_ticks;
    b8                ground_crouch_crawl;   // crawling (facing locked)
    u8                ground_crouch;         // if crouched: greater than 0. Ticks up to OWL_CROUCH_MAX_TICKS
    i8                ground_pull_wire;      // sign
    i8                ground_pull_wire_prev; // sign
    //
    u32               attackUID;
    u32               hitboxUID;
    u8                attack_tick;
    u8                attack_type;
    u8                attack_flipflop;
    u8                attack_last_frame;
    //
    u8                stomp_hook_mom;
    //
    i32               swim_anim;
    u8                swim_sideways;
    u8                swim; // OWLET_SWIM_SURFACE, _DIVE
    //
    obj_handle_s      carried;
    i32               carry_anim;
    i32               carry; // tick
    //
    i32               aim_crank_acc;       // accumulator of crank towards aim mode
    u8                aim_crank_t_falloff; // ticks until accumulator falls off again
    u8                aim_ticks;           // how long aiming already
    //
    v2_i32            safe_pos;
    v2_i32            safe_v;
    i32               safe_facing;
    //
    u16               dead_anim_ticks;
    u8                dead_bounce_counter;
    u8                dead_ground_ticks;
    //
    b8                sprint;
    u8                stance;           // OWLET_STANCE_GRAPPLE, _ATTACK
    u8                stance_swap_tick; // tick to swap stance (hold B)
    u8                hitID;
    u8                ticks_health; // animator tick low health ui
    u8                squish;
    b8                jump_ui_may_hide;
    u8                invincibility_ticks;
    u8                hurt_ticks;
    obj_handle_s      interactable;
    u8                n_jumpstomped;
    obj_jumpstomped_s jumpstomped[8];
} owl_s;

obj_s *owl_create(g_s *g);
void   owl_on_update(g_s *g, obj_s *o, inp_s inp);
void   owl_on_update_post(g_s *g, obj_s *o, inp_s inp);
obj_s *owl_if_present_and_alive(g_s *g);
void   owl_on_animate(g_s *g, obj_s *o);
i32    owl_state_check(g_s *g, obj_s *o);
void   owl_water(g_s *g, obj_s *o, inp_s inp);
void   owl_climb(g_s *g, obj_s *o, inp_s inp);
void   owl_ground(g_s *g, obj_s *o, inp_s inp);
void   owl_air(g_s *g, obj_s *o, inp_s inp);
//
bool32 owl_upgrade_add(obj_s *o, u32 ID);
bool32 owl_upgrade_rem(obj_s *o, u32 ID);
bool32 owl_upgrade_has(obj_s *o, u32 ID);
bool32 owl_in_water(g_s *g, obj_s *o);
bool32 owl_submerged(g_s *g, obj_s *o);
bool32 owl_climb_still_on_ladder(g_s *g, obj_s *o, i32 offx, i32 offy);
bool32 owl_climb_still_on_wall(g_s *g, obj_s *o, i32 facing, i32 offx, i32 offy);
void   owl_set_to_climb(g_s *g, obj_s *o);
void   owl_cancel_climb(g_s *g, obj_s *o);
void   owl_cancel_swim(g_s *g, obj_s *o);
void   owl_cancel_air(g_s *g, obj_s *o);
void   owl_cancel_push_pull(g_s *g, obj_s *o);
void   owl_cancel_ground(g_s *g, obj_s *o);
bool32 owl_attack_cancellable(obj_s *o);
void   owl_cancel_attack(g_s *g, obj_s *o);
void   owl_cancel_hook_aim(g_s *g, obj_s *o);
void   owl_cancel_knockback(g_s *g, obj_s *o);
void   owl_cancel_carry(g_s *g, obj_s *o);
void   owl_cancel_swap(g_s *g, obj_s *o);
void   owl_on_controlled_by_other(g_s *g, obj_s *o);
void   owl_ungrapple(g_s *g, obj_s *o);
bool32 owl_try_force_normal_height(g_s *g, obj_s *o);
void   owl_jump_ground(g_s *g, obj_s *o);
void   owl_jump_air(g_s *g, obj_s *o);
bool32 owl_jump_wall(g_s *g, obj_s *o, i32 dirx_jump);
void   owl_ground_crawl(g_s *g, obj_s *o, inp_s inp);
i32    owl_swap_ticks();
bool32 owl_climb_try_snap_to_ladder(g_s *g, obj_s *o);
void   owl_climb_ladder(g_s *g, obj_s *o, inp_s inp);
void   owl_climb_wall(g_s *g, obj_s *o, inp_s inp);
i32    owl_swim_frameID(i32 animation);
i32    owl_swim_frameID_idle(i32 animation);
void   owl_jump_out_of_water(g_s *g, obj_s *o);
i32    owl_stamina_modify(obj_s *o, i32 dt);
void   owl_on_touch_ground(g_s *g, obj_s *o);
i32    owl_jumpstomped_register(obj_s *ohero, obj_s *o, bool32 stomped);
i32    owl_hook_aim_angle_crank(i32 crank_q16);
v2_i32 owl_hook_aim_vec_from_angle(i32 a_q16, i32 l);
v2_i32 owl_hook_aim_vec_from_crank_angle(i32 crank_q16, i32 l);
void   owl_walljump_execute(g_s *g, obj_s *o);
v2_i32 owl_rope_v_to_connected_node(g_s *g, obj_s *o);
void   owl_kill(g_s *g, obj_s *o);
void   owl_set_stance(g_s *g, obj_s *o, i32 stance);
void   owl_special_state(g_s *g, obj_s *o, i32 special_state);
void   owl_special_state_unset(obj_s *o);
void   owl_stomp_land(g_s *g, obj_s *o);
void   owl_cb_hitbox(g_s *g, hitbox_s *hb, void *arg);

#endif