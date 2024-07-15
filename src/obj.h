// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#define ENEMY_HURT_TICKS 20

#include "gamedef.h"
#include "objdef.h"
#include "rope.h"

#define NUM_OBJ_POW_2   9 // = 2^N
#define NUM_OBJ         (1 << NUM_OBJ_POW_2)
#define OBJ_ID_INDEX_SH (32 - NUM_OBJ_POW_2)
#define OBJ_ID_GEN_MASK (((u32)1 << OBJ_ID_INDEX_SH) - 1)

static inline u32 obj_GID_incr_gen(u32 gid)
{
    return ((gid & ~OBJ_ID_GEN_MASK) | ((gid + 1) & OBJ_ID_GEN_MASK));
}

static inline u32 obj_GID_set(i32 index, i32 gen)
{
    assert(0 <= index && index < NUM_OBJ);
    return (((u32)index << OBJ_ID_INDEX_SH) | ((u32)gen));
}

enum {
    PROJECTILE_ID_DEFAULT,
    PROJECTILE_ID_BUDPLANT,
    PROJECTILE_ID_STALACTITE_BREAK,
};

#define OBJ_FLAG_MOVER              ((u64)1 << 0)
#define OBJ_FLAG_INTERACTABLE       ((u64)1 << 2)
#define OBJ_FLAG_PLATFORM           ((u64)1 << 5)
#define OBJ_FLAG_PLATFORM_HERO_ONLY ((u64)1 << 6) // only acts as a platform for the hero
#define OBJ_FLAG_KILL_OFFSCREEN     ((u64)1 << 7)
#define OBJ_FLAG_HOOKABLE           ((u64)1 << 8)
#define OBJ_FLAG_SPRITE             ((u64)1 << 9)
#define OBJ_FLAG_ENEMY              ((u64)1 << 10)
#define OBJ_FLAG_COLLECTIBLE        ((u64)1 << 11)
#define OBJ_FLAG_HURT_ON_TOUCH      ((u64)1 << 12)
#define OBJ_FLAG_CARRYABLE          ((u64)1 << 13)
#define OBJ_FLAG_IS_CARRIED         ((u64)1 << 14)
#define OBJ_FLAG_CLAMP_ROOM_X       ((u64)1 << 15)
#define OBJ_FLAG_CLAMP_ROOM_Y       ((u64)1 << 16)
#define OBJ_FLAG_BOSS               ((u64)1 << 17)
#define OBJ_FLAG_HOVER_TEXT         ((u64)1 << 18)
#define OBJ_FLAG_CAN_BE_JUMPED_ON   ((u64)1 << 19)
#define OBJ_FLAG_RENDER_AABB        ((u64)1 << 63)

#define OBJ_FLAG_CLAMP_TO_ROOM  (OBJ_FLAG_CLAMP_ROOM_X | OBJ_FLAG_CLAMP_ROOM_Y)
#define OBJ_FLAG_ACTOR_PLATFORM (OBJ_FLAG_ACTOR | OBJ_FLAG_PLATFORM)

enum {
    OBJ_BUMPED_X_NEG     = 1 << 0,
    OBJ_BUMPED_X_POS     = 1 << 1,
    OBJ_BUMPED_Y_NEG     = 1 << 2,
    OBJ_BUMPED_Y_POS     = 1 << 3,
    OBJ_BUMPED_SQUISH    = 1 << 4,
    OBJ_BUMPED_ON_HEAD   = 1 << 5,
    OBJ_BUMPED_X_BOUNDS  = 1 << 6,
    OBJ_BUMPED_Y_BOUNDS  = 1 << 7,
    OBJ_BUMPED_JUMPED_ON = 1 << 8,
    OBJ_BUMPED_X         = OBJ_BUMPED_X_NEG | OBJ_BUMPED_X_POS,
    OBJ_BUMPED_Y         = OBJ_BUMPED_Y_NEG | OBJ_BUMPED_Y_POS,
};

enum {
    OBJ_MOVER_GLUE_GROUND  = 1 << 0,
    OBJ_MOVER_ONE_WAY_PLAT = 1 << 1,
    OBJ_MOVER_SLIDE_Y_POS  = 1 << 2,
    OBJ_MOVER_SLIDE_Y_NEG  = 1 << 3,
    OBJ_MOVER_SLIDE_X_POS  = 1 << 4,
    OBJ_MOVER_SLIDE_X_NEG  = 1 << 5,
    OBJ_MOVER_MAP          = 1 << 6,
};

#define OBJ_HOVER_TEXT_TICKS 20
typedef void (*obj_action_s)(game_s *g, obj_s *o);

typedef struct {
    texrec_s trec;
    v2_i16   offs;
    i16      flip;
} obj_sprite_s;

typedef struct {
    u16   sndID_hurt;
    u16   sndID_die;
    u8    die_tick;
    u8    hurt_tick;
    v2_i8 hurt_shake_offs;
    bool8 invincible;
} enemy_s;

typedef struct {
    v2_i32 offs;
    i16    tick;
    i16    time;
} obj_carry_s;

static inline u32 save_ID_gen(i32 roomID, i32 objID)
{
    u32 save_ID = ((u32)roomID << 16) | ((u32)objID);
    return save_ID;
}

typedef void (*obj_on_update_f)(game_s *g, obj_s *o);
typedef void (*obj_on_animate_f)(game_s *g, obj_s *o);
typedef void (*obj_on_draw_f)(game_s *g, obj_s *o, v2_i32 cam);
typedef void (*obj_on_trigger_f)(game_s *g, obj_s *o, i32 trigger);
typedef void (*obj_on_interact_f)(game_s *g, obj_s *o);

#define OBJ_MAGIC U32_C(0xABABABAB)
struct obj_s {
    obj_s            *next; // linked list
    //
    u32               GID;     // generational index
    u16               ID;      // type of object
    u16               subID;   // subtype of object
    u32               save_ID; // used to register save events
    flags64           flags;
    flags32           tags;
    //
    obj_on_update_f   on_update;
    obj_on_animate_f  on_animate;
    obj_on_draw_f     on_draw_pre;
    obj_on_draw_f     on_draw;
    obj_on_trigger_f  on_trigger;
    obj_on_interact_f on_interact;
    //
    i32               render_priority;
    flags16           bumpflags; // has to be cleared manually
    flags16           moverflags;
    i16               mass; // mass, for solid movement
    i16               w;
    i16               h;
    v2_i32            pos; // position in pixels
    v2_i32            posprev;
    v2_i16            subpos_q8;
    v2_i16            vel_q8;
    v2_i16            vel_prev_q8;
    v2_i16            vel_cap_q8;
    v2_i16            drag_q8;
    v2_i16            gravity_q8;
    v2_i16            tomove;
    v2_i16            knockback_q8;
    u16               knockback_tick;
    // some generic behaviour fields
    i32               facing; // -1 left, +1 right
    i32               trigger;
    i32               action;
    i32               subaction;
    i32               animation;
    i32               timer;
    i32               subtimer;
    i16               state;
    i16               substate;
    i16               health;
    i16               health_max;
    u16               cam_attract_r;
    i16               invincible_tick;
    enemy_s           enemy;
    //
    ropenode_s       *ropenode;
    rope_s           *rope;
    obj_handle_s      linked_solid;
    obj_carry_s       carry;
    //
    i32               n_sprites;
    obj_sprite_s      sprites[4];
    char              filename[64];
    //
    ALIGN(4) char     mem[512];
    u32               magic;
};

obj_handle_s obj_handle_from_obj(obj_s *o);
obj_s       *obj_from_obj_handle(obj_handle_s h);
bool32       obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out);
bool32       obj_handle_valid(obj_handle_s h);
//
obj_s       *obj_create(game_s *g);
void         obj_delete(game_s *g, obj_s *o); // only flags for deletion -> deleted at end of frame
bool32       obj_tag(game_s *g, obj_s *o, i32 tag);
bool32       obj_untag(game_s *g, obj_s *o, i32 tag);
obj_s       *obj_get_tagged(game_s *g, i32 tag);
void         objs_cull_to_delete(game_s *g); // removes all flagged objects
bool32       overlap_obj(obj_s *a, obj_s *b);
rec_i32      obj_aabb(obj_s *o);
rec_i32      obj_rec_left(obj_s *o);
rec_i32      obj_rec_right(obj_s *o);
rec_i32      obj_rec_bottom(obj_s *o);
rec_i32      obj_rec_top(obj_s *o);
v2_i32       obj_pos_bottom_center(obj_s *o);
v2_i32       obj_pos_center(obj_s *o);
bool32       map_blocked(game_s *g, obj_s *o, rec_i32 r, i32 m);
bool32       map_blocked_pt(game_s *g, obj_s *o, i32 x, i32 y, i32 m);
bool32       map_overlaps_mass_eq_or_higher(game_s *g, rec_i32 r, i32 m);
bool32       obj_step_x(game_s *g, obj_s *o, i32 dx, bool32 slide, i32 mpush);
bool32       obj_step_y(game_s *g, obj_s *o, i32 dy, bool32 slide, i32 mpush);
void         obj_move(game_s *g, obj_s *o, v2_i32 dt);
bool32       obj_try_wiggle(game_s *g, obj_s *o);
void         obj_on_squish(game_s *g, obj_s *o);
bool32       obj_grounded(game_s *g, obj_s *o);
bool32       obj_grounded_at_offs(game_s *g, obj_s *o, v2_i32 offs);
bool32       obj_would_fall_down_next(game_s *g, obj_s *o, i32 xdir); // not on ground returns false
void         squish_delete(game_s *g, obj_s *o);
v2_i32       obj_constrain_to_rope(game_s *g, obj_s *o);
obj_s       *carryable_present(game_s *g);
v2_i32       carryable_pos_on_hero(obj_s *ohero, obj_s *ocarry, rec_i32 *rlift);
void         carryable_on_lift(game_s *g, obj_s *o);
void         carryable_on_drop(game_s *g, obj_s *o);
v2_i32       carryable_animate_spr_offset(obj_s *o);
void         obj_on_hooked(game_s *g, obj_s *o);
obj_s       *obj_closest_interactable(game_s *g, v2_i32 pos);

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void    obj_apply_movement(obj_s *o);
enemy_s enemy_default();

#endif