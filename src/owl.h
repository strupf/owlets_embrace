// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HEADER_OWL_H
#define HEADER_OWL_H

#include "gamedef.h"
#include "grapplinghook.h"
#include "obj.h"
#include "rope.h"

#define OWL_LEN_NAME                20
#define OWL_W                       14
#define OWL_H                       26
#define OWL_H_CROUCH                16
#define OWL_WATER_THRESHOLD         17
#define OWL_W_STOMP_ADD_SYMM        4 // amount to add to width for easier stomping of objects
#define OWL_GRAVITY                 Q_VOBJ(0.32)
#define OWL_CROUCH_MAX_TICKS        5 // ticks to duck down
#define OWL_CROUCH_STANDUP_TICKS    5 // ticks to stand up
#define OWL_VX_CRAWL                Q_VOBJ(1.5)
#define OWL_VX_WALK                 Q_VOBJ(3.0)
#define OWL_VX_SPRINT               Q_VOBJ(4.0)
#define OWL_VX_MAX_GROUND           Q_VOBJ(5.0)
#define OWL_VX_SWIM                 Q_VOBJ(3.0)
#define OWL_CLIMB_LADDER_SNAP_X_SYM 12 // pixel range centerx +- x to snap to a ladder
#define OWL_CLIMB_LADDER_SNAP_Y     8  // centery pixel for ladder positioning
#define OWL_AIR_STOMP_MAX           14
#define OWL_STOMP_LANDING_TICKS     10
#define OWL_JUMP_EDGE_TICKS         16
#define OWL_INTERACTABLE_DST        64
//
#define OWL_STAMINA_TICKS_UI_FADE   10
#define OWL_STAMINA_PER_CONTAINER   256
#define OWL_STAMINA_AIR_JUMP_INIT   256
#define OWL_STAMINA_AIR_JUMP_HOLD   16

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
    OWL_CLIMB_NONE,
    OWL_CLIMB_LADDER,
    OWL_CLIMB_WALL
};

enum {
    OWL_JUMP_WATER,
    OWL_JUMP_GROUND_SPEAR,
    OWL_JUMP_GROUND,
    OWL_JUMP_GROUND_BOOSTED,
    OWL_JUMP_FLY,
    OWL_JUMP_WALL,
    //
    NUM_OWL_JUMP
};

enum {
    OWL_STANCE_GRAPPLE,
    OWL_STANCE_ATTACK
};

enum {
    OWL_SWIM_NONE,
    OWL_SWIM_SURFACE,
    OWL_SWIM_DIVE
};

enum {
    OWL_ST_NULL,
    OWL_ST_GROUND,
    OWL_ST_AIR,
    OWL_ST_WATER,
    OWL_ST_CLIMB
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
    u8           name[OWL_LEN_NAME];
    u32          upgrades;
    u16          health;
    u16          health_max;
    u16          stamina;
    u16          stamina_max;   // calculated from stamina_containers
    u16          stamina_added; // how of stamina was just added (only visual)
    u8           stamina_upgrades;
    u8           stamina_added_delay_ticks;
    u8           stamina_ui_fade_ticks;
    b8           stamina_ui_show;
    //
    i16          climb_from_x;  // used for smooting, visual only
    i16          climb_ladderx; // position on ladder
    i16          climb_anim;
    u8           climb_camx_smooth; // smooth camera movement on snapping
    u8           climb;             // OWLET_CLIMB_NONE, _LADDER, _WALL
    //
    i16          jump_from_y;
    u8           jump_ground_ticks;
    i32          jump_snd_iID;
    u8           jump_index; // index into jump parameter table
    u8           jump_ticks;
    u8           jump_edgeticks;
    u8           air_gliding;
    u8           air_stomp; // stomp ticks
    //
    u8           ground_push_tick;
    u16          ground_pushing_anim;
    i8           ground_pushing_prev; // pushing sign
    i8           ground_pushing;      // pushing sign
    u8           ground;
    u8           ground_skid_ticks;   // ticks cancel sprinting
    u8           ground_impact_ticks; // ticks just landed
    u16          ground_idle_ticks;   // ticks for animating idle
    i32          ground_walking;      // animating walking/sprinting
    u8           ground_sprint_doubletap_ticks;
    u8           ground_stomp_landing_ticks;
    //
    i32          crouch_crawl_anim;
    u8           crouch_standup_ticks;
    b8           crouch_crawl; // crawling (facing locked)
    u8           crouch;       // if crouched: greater than 0. Ticks up to OWL_CROUCH_MAX_TICKS
    //
    u8           attack_tick;
    u8           attack_flipflop;
    u8           attack_last_frame;
    //
    u8           stomp_hook_mom;
    //
    i32          swim_anim;
    u8           swim_sideways;
    u8           swim; // OWLET_SWIM_NONE, _SURFACE, _DIVE
    //
    v2_i32       safe_pos;
    v2_i32       safe_v;
    i32          safe_facing;
    v2_i8        render_align_offs;
    //
    b8           sprint;
    u8           stance;           // OWLET_STANCE_GRAPPLE, _ATTACK
    u8           stance_swap_tick; // tick to swap stance (hold B)
    u8           hitID;
    u8           ticks_health; // animator tick low health ui
    u8           squish;
    u8           dead_bounce;
    b8           jump_ui_may_hide;
    u8           invincibility_ticks;
    u8           hurt_ticks;
    obj_handle_s interactable;

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
bool32 owl_in_water(g_s *g, obj_s *o);
bool32 owl_submerged(g_s *g, obj_s *o);
bool32 owl_climb_still_on_ladder(g_s *g, obj_s *o, i32 offx, i32 offy);
bool32 owl_climb_still_on_wall(g_s *g, obj_s *o, i32 offx, i32 offy);
void   owl_cancel_climb(g_s *g, obj_s *o);
void   owl_cancel_swim(g_s *g, obj_s *o);
void   owl_cancel_air(g_s *g, obj_s *o);
void   owl_cancel_push_pull(g_s *g, obj_s *o);
void   owl_cancel_ground(g_s *g, obj_s *o);
void   owl_cancel_attack(g_s *g, obj_s *o);
void   owl_ungrapple(g_s *g, obj_s *o);
bool32 owl_try_force_normal_height(g_s *g, obj_s *o);
void   owl_jump_ground(g_s *g, obj_s *o);
void   owl_jump_air(g_s *g, obj_s *o);
void   owl_ground_crawl(g_s *g, obj_s *o, inp_s inp);
i32    owl_swap_ticks();
bool32 owl_climb_try_snap_to_ladder(g_s *g, obj_s *o);
void   owl_climb_ladder(g_s *g, obj_s *o, inp_s inp);
i32    owl_swim_frameID(i32 animation);
i32    owl_swim_frameID_idle(i32 animation);
void   owl_jump_out_of_water(g_s *g, obj_s *o);
i32    owl_stamina_modify(obj_s *o, i32 dt);
void   owl_on_touch_ground(g_s *g, obj_s *o);
i32    owl_jumpstomped_register(obj_s *ohero, obj_s *o, bool32 stomped);

#endif