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
    HERO_UPGRADE_HOOK       = 1 << 0,
    HERO_UPGRADE_SWIM       = 1 << 1,
    HERO_UPGRADE_DIVE       = 1 << 2,
    HERO_UPGRADE_FLY        = 1 << 3,
    HERO_UPGRADE_CLIMB      = 1 << 4,
    HERO_UPGRADE_STOMP      = 1 << 5,
    HERO_UPGRADE_POWERSTOMP = 1 << 6,
    HERO_UPGRADE_SPEAR      = 1 << 7,
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
    HERO_ATTACK_NONE,
    HERO_ATTACK_GROUND
};

enum {
    HERO_MODE_NORMAL,
    HERO_MODE_COMBAT
};

enum {
    SWIMMING_SURFACE = 1,
    SWIMMING_DIVING  = 2,
};

#define HERO_B_HOLD_SWAP           30
#define HERO_B_HOLD_SWAP_THRESHOLD 15

#define HERO_CRANK_BUILDUP_Q16         30000
#define HERO_DROWN_TICKS               150
#define HERO_AIM_STR_TICKS_PERIOD      100
#define HERO_AIM_THROW_STR_OFF_Q4      4 // offset for default in period
#define HERO_AIM_THROW_STR_MIN         1000
#define HERO_AIM_THROW_STR_MAX         5000
#define HERO_AIM_MODE_CRANK_THRESHOLD  32768 // angle to crank until aim mode
#define HERO_THROW_INTENSITY_MAX       256
#define HERO_LOW_HP_ANIM_TICKS_HIT     20
#define HERO_LOW_HP_ANIM_TICKS_RECOVER 20
#define HERO_LEN_INPUT_BUF             32
#define HERO_SQUISH_TICKS              35
#define HERO_CLIMB_Y1_OFFS             2 // from top downwards
#define HERO_CLIMB_Y2_OFFS             4 // from bottom upwards
#define HERO_TICKS_SPIN_ATTACK         22
#define HERO_HURT_LP_TICKS             85 // duration of lowpass after hurt
#define HERO_HURT_LP_TICKS_FADE        30 // ticks for fading from hurt lowpass back to normal
#define HERO_WATER_THRESHOLD           18
#define HERO_WIDTH                     14
#define HERO_X_OFFS_LADDER             ((16 - HERO_WIDTH) >> 1)
#define HERO_CROUCHED_MAX_TICKS        5
#define HERO_HEIGHT_CROUCHED           16
#define HERO_HEIGHT                    26
#define HERO_VX_CRAWL                  Q_VOBJ(1.5)
#define HERO_VX_WALK                   Q_VOBJ(3.0)
#define HERO_VX_SPRINT                 Q_VOBJ(4.0)
#define HERO_VX_MAX_GROUND             Q_VOBJ(4.0)
#define HERO_REEL_RATE                 30
#define HERO_SWIM_TICKS                50 // duration of swimming without upgrade
#define HEROHOOK_N_HIST                4
#define HERO_GRAVITY                   Q_VOBJ(0.3125)
#define HERO_TICKS_PER_STAMINA_UPGRADE 1024
#define HERO_DISTSQ_INTERACT           POW2(50)
#define HERO_DISTSQ_PICKUP             POW2(100)
#define HERO_TICKS_STOMP_INIT          6
#define STAMINA_UI_TICKS_HIDE          12
#define WALLJUMP_ANIM_TICKS            10
#define WALLJUMP_MOM_TICKS             55
#define WALLJUMP_TICKS                 20
#define HERO_WALLJUMP_THRESHOLD_PX     12
#define HERO_NUM_JUMPED_ON             8
#define HERO_HEALTH_RESTORE_TICKS      250
#define HERO_STOMP_LANDING_TICKS       12
#define HERO_HURT_TICKS                50
#define HERO_INVINCIBILITY_TICKS       100
#define HERO_W_STOMP_ADD_SYMM          4
//
#define HERO_ITEMSWAP_TICKS            25
#define HERO_ITEMSWAP_TICKS_INIT       15
#define HERO_ITEMSWAP_TICKS_FADE       25

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND_SPEAR,
    HERO_JUMP_GROUND,
    HERO_JUMP_GROUND_BOOSTED,
    HERO_JUMP_FLY,
    HERO_JUMP_WALL,
    //
    NUM_HERO_JUMP
};

typedef struct hero_jumpvar_s {
    ALIGNAS(8)
    i16 vy;
    i16 ticks; // ticks of variable jump (decreases faster if jump button is not held)
    i16 v0;    // "jetpack" velocity, goes to v1 over ticks or less
    i16 v1;
} hero_jumpvar_s;

extern const hero_jumpvar_s g_herovar[NUM_HERO_JUMP];

typedef struct {
    obj_handle_s h;
    bool32       stomped;
} jumpstomped_s;

typedef struct {
    v2_i32 p;
    v2_i32 pp;
} hero_hook_trail_el_s;

typedef struct hero_s {
    u32    upgrades;
    i32    crank_acc_q16;
    i32    hook_aim_mode_tick;
    i32    hook_aim;
    i32    hook_aim_mode;
    i32    crank_swap_buildup;
    i32    jump_snd_iID;
    u16    idle_state;
    u16    idle_animID;
    u32    idle_anim_tick;
    u32    idle_tick;
    v2_i32 safe_pos;
    v2_i16 safe_v;
    v2_i8  render_align_offs;
    i16    safe_facing;
    i16    air_block_ticks;
    i16    air_block_ticks_og;
    u8     drown_tick;
    u16    stamina_ui_collected_tick;
    u16    stamina_ui_fade_out;
    u16    stamina_added;
    u16    stamina;
    i16    ladderx;
    u8     mode;
    u8     hitID;
    u8     ticks_health; // animator tick low health ui
    u8     state_prev;
    u8     state_curr;
    b8     ropepulled;
    u8     gliding;
    b8     was_grounded_upd;
    b8     was_grounded_upd_post;
    i8     grabbing; // -1/+1 = grabbing static, -2/+2 = push/pull
    u8     swimming; // SWIMMING_SURFACE, SWIMMING_DIVING
    u8     squish;
    b8     climbing;
    u8     dead_bounce;
    u8     ladder; // while climbing: 1 if on ladder, 0 if on wall
    b8     jump_ui_may_hide;
    b8     sprint;
    u8     impact_ticks;
    u8     walljump_glue;
    i8     walljump_tick; // signed ticks -> direction
    u8     crouch_standup;
    i8     crouched; // 0< sitting, <0 crawling
    i8     stomp;    // 0< active, <0 landing animation
    u8     swimsfx_delay;
    u8     invincibility_ticks;
    u8     stamina_pieces;
    u8     skidding;
    u8     b_hold_tick;
    u8     attack_ID;
    u8     attack_tick;
    u8     attack_flipflop;
    u8     attack_last_frame;
    u8     edgeticks;
    u8     jump_index; // index into jump parameter table
    u8     jumpticks;
    u8     hurt_ticks;
    u8     n_jumpstomped;
    u8     stamina_upgrades;
    u8     n_ibuf;                   // position in circular input buffer
    u8     ibuf[HERO_LEN_INPUT_BUF]; // circular buffer of button states
    u8     name[LEN_HERO_NAME];

    hero_hook_trail_el_s hooktrail[4];
    obj_handle_s         obj_grabbed;
    jumpstomped_s        jumpstomped[HERO_NUM_JUMPED_ON];
} hero_s;

obj_s   *hero_create(g_s *g);
i32      hero_special_input(g_s *g, obj_s *o, inp_s inp);
void     hero_on_update(g_s *g, obj_s *o, inp_s inp);
void     hero_post_update(g_s *g, obj_s *o, inp_s inp);
void     hero_on_animate(g_s *g, obj_s *o);
void     hero_animate_ui(g_s *g);
i32      hero_get_actual_state(g_s *g, obj_s *o);
void     hero_on_squish(g_s *g, obj_s *o);
void     hero_check_rope_intact(g_s *g, obj_s *o);
void     hero_hurt(g_s *g, obj_s *o, i32 damage);
void     hero_stamina_update_ui(obj_s *o, i32 amount); // swaps "temporary" stamina to regular stamina -> UI
i32      hero_stamina_modify(obj_s *o, i32 dt);
void     hero_stamina_add_ui(obj_s *o, i32 dt); // add with UI animation
i32      hero_stamina_left(obj_s *o);
i32      hero_stamina_ui_full(obj_s *o);
i32      hero_stamina_ui_added(obj_s *o);
i32      hero_stamina_max(obj_s *o);
bool32   hero_present_and_alive(g_s *g, obj_s **o);
void     hero_action_throw_grapple(g_s *g, obj_s *o, i32 ang_q16, i32 vel);
bool32   hero_action_ungrapple(g_s *g, obj_s *o);
obj_s   *hero_interactable_available(g_s *g, obj_s *o);
i32      hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy);
i32      hero_ladder_or_climbwall_snapdata(g_s *g, obj_s *o, i32 offx, i32 offy, i32 *dt_snap_x);
i32      hero_swim_frameID(i32 animation);
i32      hero_swim_frameID_idle(i32 animation);
v2_i32   hero_hook_aim_dir(hero_s *h);
void     hero_stomped_ground(g_s *g, obj_s *o);
void     hero_walljump(g_s *g, obj_s *o, i32 dir);
bool32   hero_stomping(obj_s *o);
i32      hero_register_jumpstomped(obj_s *ohero, obj_s *o, bool32 stomped);
i32      hero_can_grab(g_s *g, obj_s *o, i32 dirx);
bool32   hero_has_upgrade(g_s *g, u32 ID);
void     hero_add_upgrade(g_s *g, u32 ID);
void     hero_rem_upgrade(g_s *g, u32 ID);
void     hero_ungrab(g_s *g, obj_s *o);
hitbox_s hero_hitbox_wingattack(obj_s *o);
hitbox_s hero_hitbox_stomp(obj_s *o);
hitbox_s hero_hitbox_powerstomp(obj_s *o);
void     hero_aim_abort(obj_s *o);
i32      hero_aim_angle_conv(i32 crank_q16);
i32      hero_aim_throw_strength(obj_s *o);

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