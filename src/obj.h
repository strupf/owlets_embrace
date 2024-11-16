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
#define OBJ_FLAG_CLAMP_ROOM_X       ((u64)1 << 15)
#define OBJ_FLAG_CLAMP_ROOM_Y       ((u64)1 << 16)
#define OBJ_FLAG_BOSS               ((u64)1 << 17)
#define OBJ_FLAG_HOVER_TEXT         ((u64)1 << 18)
#define OBJ_FLAG_CAN_BE_JUMPED_ON   ((u64)1 << 19)
#define OBJ_FLAG_LIGHT              ((u64)1 << 20)
#define OBJ_FLAG_RENDER_AABB        ((u64)1 << 63)

#define OBJ_FLAG_CLAMP_TO_ROOM (OBJ_FLAG_CLAMP_ROOM_X | OBJ_FLAG_CLAMP_ROOM_Y)

enum obj_bump_flags_e {
    OBJ_BUMP_X_NEG     = 1 << 0,
    OBJ_BUMP_X_POS     = 1 << 1,
    OBJ_BUMP_Y_NEG     = 1 << 2,
    OBJ_BUMP_Y_POS     = 1 << 3,
    OBJ_BUMP_SQUISH    = 1 << 4,
    OBJ_BUMP_ON_HEAD   = 1 << 5,
    OBJ_BUMP_X_BOUNDS  = 1 << 6,
    OBJ_BUMP_Y_BOUNDS  = 1 << 7,
    OBJ_BUMP_JUMPED_ON = 1 << 8,
    OBJ_BUMP_X         = OBJ_BUMP_X_NEG | OBJ_BUMP_X_POS,
    OBJ_BUMP_Y         = OBJ_BUMP_Y_NEG | OBJ_BUMP_Y_POS,
};

static inline i32 obj_bump_x_flag(i32 x)
{
    if (0 < x) return OBJ_BUMP_X_POS;
    if (x < 0) return OBJ_BUMP_X_NEG;
    return 0;
}

static inline i32 obj_bump_y_flag(i32 y)
{
    if (0 < y) return OBJ_BUMP_Y_POS;
    if (y < 0) return OBJ_BUMP_Y_NEG;
    return 0;
}

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
typedef void (*obj_action_s)(g_s *g, obj_s *o);

typedef struct {
    texrec_s trec;
    v2_i16   offs;
    i16      flip;
} obj_sprite_s;

typedef struct enemy_s {
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

typedef void (*obj_on_update_f)(g_s *g, obj_s *o);
typedef void (*obj_on_animate_f)(g_s *g, obj_s *o);
typedef void (*obj_on_draw_f)(g_s *g, obj_s *o, v2_i32 cam);
typedef void (*obj_on_trigger_f)(g_s *g, obj_s *o, i32 trigger);
typedef void (*obj_on_interact_f)(g_s *g, obj_s *o);

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
    u8                mass_og;
    u8                mass; // mass, for solid movement
    i16               w;
    i16               h;
    v2_i32            pos; // position in pixels
    v2_i16            subpos_q8;
    v2_i16            v_q8;
    v2_i16            v_prev_q8;
    v2_i16            drag_q8;
    v2_i16            grav_q8;
    v2_i16            tomove;
    v2_i16            knockback_q8;
    u16               knockback_tick;
    i16               v_cap_x_q8;
    i16               v_cap_y_q8_pos;
    i16               v_cap_y_q8_neg;
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
    u16               cam_attract_r;
    u8                health;
    u8                health_max;
    bool8             interactable_hovered;
    u8                light_radius;
    u8                light_strength;
    enemy_s           enemy;
    //
    ropenode_s       *ropenode;
    rope_s           *rope;
    obj_handle_s      linked_solid;
    //
    i32               n_sprites;
    obj_sprite_s      sprites[4];
    char              filename[64];
    //
    void             *heap;
    alignas(4) byte   mem[512];
    u32               magic;
};

obj_handle_s obj_handle_from_obj(obj_s *o);
obj_s       *obj_from_obj_handle(obj_handle_s h);
bool32       obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out);
bool32       obj_handle_valid(obj_handle_s h);
//
obj_s       *obj_create(g_s *g);
void         obj_delete(g_s *g, obj_s *o); // only flags for deletion -> deleted at end of frame
bool32       obj_tag(g_s *g, obj_s *o, i32 tag);
bool32       obj_untag(g_s *g, obj_s *o, i32 tag);
obj_s       *obj_get_tagged(g_s *g, i32 tag);
void         objs_cull_to_delete(g_s *g); // removes all flagged objects
bool32       overlap_obj(obj_s *a, obj_s *b);
rec_i32      obj_aabb(obj_s *o);
rec_i32      obj_rec_left(obj_s *o);
rec_i32      obj_rec_right(obj_s *o);
rec_i32      obj_rec_bottom(obj_s *o);
rec_i32      obj_rec_top(obj_s *o);
v2_i32       obj_pos_bottom_center(obj_s *o);
v2_i32       obj_pos_center(obj_s *o);
bool32       map_blocked(g_s *g, obj_s *o, rec_i32 r, i32 m);
bool32       map_blocked_pt(g_s *g, obj_s *o, i32 x, i32 y, i32 m);
void         obj_move(g_s *g, obj_s *o, i32 dx, i32 dy);
b32          obj_step(g_s *g, obj_s *o, i32 sx, i32 sy, b32 can_slide, i32 m_push);
bool32       obj_try_wiggle(g_s *g, obj_s *o);
void         obj_on_squish(g_s *g, obj_s *o);
bool32       obj_grounded(g_s *g, obj_s *o);
bool32       obj_grounded_at_offs(g_s *g, obj_s *o, v2_i32 offs);
bool32       obj_would_fall_down_next(g_s *g, obj_s *o, i32 xdir); // not on ground returns false
void         squish_delete(g_s *g, obj_s *o);
v2_i32       obj_constrain_to_rope(g_s *g, obj_s *o);
void         obj_on_hooked(g_s *g, obj_s *o);
obj_s       *obj_closest_interactable(g_s *g, v2_i32 pos);
void         obj_move_by_q8(g_s *g, obj_s *o, i32 dx_q8, i32 dy_q8);
void         obj_move_by_v_q8(g_s *g, obj_s *o);
void         obj_v_q8_mul(obj_s *o, i32 mx_q8, i32 my_q8);
void         obj_vx_q8_mul(obj_s *o, i32 mx_q8);
void         obj_vy_q8_mul(obj_s *o, i32 my_q8);
bool32       obj_blocked_by_map_or_objs(g_s *g, obj_s *o, i32 sx, i32 sy);
bool32       obj_on_platform(g_s *g, obj_s *o, i32 x, i32 y, i32 w);

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void    obj_apply_movement(obj_s *o);
enemy_s enemy_default();

#endif