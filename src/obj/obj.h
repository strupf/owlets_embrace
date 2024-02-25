// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#include "gamedef.h"
#include "rope.h"

#define NUM_OBJ 256

enum {
    OBJ_ID_NULL,
    OBJ_ID_HERO,
    OBJ_ID_HOOK,
    OBJ_ID_SIGN,
    OBJ_ID_SIGN_POPUP,
    OBJ_ID_SOLID,
    OBJ_ID_DOOR_SWING,
    OBJ_ID_SAVEPOINT,
    OBJ_ID_CRUMBLEBLOCK,
    OBJ_ID_SWITCH,
    OBJ_ID_TOGGLEBLOCK,
    OBJ_ID_CLOCKPULSE,
    OBJ_ID_SHROOMY,
    OBJ_ID_CRAWLER,
    OBJ_ID_CRAWLER_CATERPILLAR,
    OBJ_ID_CARRIER,
    OBJ_ID_HEROUPGRADE,
    OBJ_ID_MOVINGPLATFORM,
    OBJ_ID_NPC,
    OBJ_ID_CHARGER,
    OBJ_ID_TELEPORT,
    OBJ_ID_JUGGERNAUT,
    OBJ_ID_COLLECTIBLE,
    OBJ_ID_STALACTITE,
    OBJ_ID_WALKER,
    OBJ_ID_FLYER,
    OBJ_ID_LOGICFLAGGER,
    OBJ_ID_TRIGGERAREA,
    OBJ_ID_PUSHABLEBOX,
    OBJ_ID_SPIKES,
    OBJ_ID_KEY,
};

enum {
    OBJ_TAG_HERO,
    OBJ_TAG_HOOK,
    //
    NUM_OBJ_TAGS
};

#define OBJ_FLAG_MOVER              ((u64)1 << 0)
#define OBJ_FLAG_TILE_COLLISION     ((u64)1 << 1)
#define OBJ_FLAG_INTERACTABLE       ((u64)1 << 2)
#define OBJ_FLAG_ACTOR              ((u64)1 << 3)
#define OBJ_FLAG_SOLID              ((u64)1 << 4)
#define OBJ_FLAG_PLATFORM           ((u64)1 << 5)
#define OBJ_FLAG_PLATFORM_HERO_ONLY ((u64)1 << 6) // only acts as a platform for the hero
#define OBJ_FLAG_KILL_OFFSCREEN     ((u64)1 << 7)
#define OBJ_FLAG_HOOKABLE           ((u64)1 << 8)
#define OBJ_FLAG_SPRITE             ((u64)1 << 9)
#define OBJ_FLAG_ENEMY              ((u64)1 << 10)
#define OBJ_FLAG_COLLECTIBLE        ((u64)1 << 11)
#define OBJ_FLAG_HURT_ON_TOUCH      ((u64)1 << 12)
#define OBJ_FLAG_CARRYABLE          ((u64)1 << 13)
#define OBJ_FLAG_CLAMP_ROOM_X       ((u64)1 << 15)
#define OBJ_FLAG_CLAMP_ROOM_Y       ((u64)1 << 16)
#define OBJ_FLAG_RENDER_AABB        ((u64)1 << 63)

#define OBJ_FLAG_CLAMP_TO_ROOM  (OBJ_FLAG_CLAMP_ROOM_X | OBJ_FLAG_CLAMP_ROOM_Y)
#define OBJ_FLAG_ACTOR_PLATFORM (OBJ_FLAG_ACTOR | OBJ_FLAG_PLATFORM)

enum {
    OBJ_BUMPED_X_NEG    = 1 << 0,
    OBJ_BUMPED_X_POS    = 1 << 1,
    OBJ_BUMPED_Y_NEG    = 1 << 2,
    OBJ_BUMPED_Y_POS    = 1 << 3,
    OBJ_BUMPED_SQUISH   = 1 << 4,
    OBJ_BUMPED_ON_HEAD  = 1 << 5,
    OBJ_BUMPED_X_BOUNDS = 1 << 6,
    OBJ_BUMPED_Y_BOUNDS = 1 << 7,
    OBJ_BUMPED_X        = OBJ_BUMPED_X_NEG | OBJ_BUMPED_X_POS,
    OBJ_BUMPED_Y        = OBJ_BUMPED_Y_NEG | OBJ_BUMPED_Y_POS,
};

enum {
    OBJ_MOVER_SLOPES         = 1 << 0,
    OBJ_MOVER_SLOPES_HI      = 1 << 1,
    OBJ_MOVER_GLUE_GROUND    = 1 << 2,
    OBJ_MOVER_AVOID_HEADBUMP = 1 << 3,
    OBJ_MOVER_ONE_WAY_PLAT   = 1 << 4,
};

typedef void (*obj_action_s)(game_s *g, obj_s *o);

typedef union {
    struct {
        u16 index;
        u16 gen;
    };
    u32 u;
} obj_UID_s;

typedef struct {
    obj_s    *o;
    obj_UID_s UID;
} obj_handle_s;

typedef struct {
    texrec_s trec;
    v2_i32   offs;
    int      flip;
    int      mode;
} sprite_simple_s;

typedef struct {
    int    sndID_hurt;
    int    sndID_die;
    int    drops;
    int    invincible;
    bool32 cannot_be_hurt;
} enemy_s;

#define OBJ_MAGIC 0xDEADBEEFU
struct obj_s {
    obj_s          *next; // linked list
    //
    obj_UID_s       UID;
    u32             ID;
    flags64         flags;
    flags32         tags;
    //
    i32             render_priority;
    flags32         bumpflags; // has to be cleared manually
    flags32         moverflags;
    int             w;
    int             h;
    v2_i32          posprev;
    v2_i32          pos; // position in pixels
    v2_i32          subpos_q8;
    v2_i32          vel_q8;
    v2_i32          vel_prev_q8;
    v2_i32          vel_cap_q8;
    v2_i32          drag_q8;
    v2_i32          gravity_q8;
    v2_i32          tomove;
    //
    int             trigger;
    int             facing; // -1 left, +1 right
    // some generic behaviour fields
    int             action;
    int             subaction;
    int             state;
    int             animation;
    int             timer;
    int             subtimer;
    int             substate;
    //
    int             collectible_type;
    int             collectible_amount;
    int             health;
    int             health_max;
    int             invincible_tick;
    enemy_s         enemy;
    //
    ropenode_s     *ropenode;
    rope_s         *rope;
    obj_handle_s    linked_solid;
    obj_handle_s    obj_handles[4];
    //
    int             subattack;
    int             attack;
    int             attack_tick;
    bool32          facing_locked;
    int             n_sprites;
    sprite_simple_s sprites[4];
    char            filename[64];
    //
    char            mem[256];
    u32             magic;
};

typedef struct {
    int    n;
    obj_s *o[NUM_OBJ];
} obj_arr_s;

obj_handle_s obj_handle_from_obj(obj_s *o);
obj_s       *obj_from_obj_handle(obj_handle_s h);
bool32       obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out);
bool32       obj_handle_valid(obj_handle_s h);
//
obj_s       *obj_create(game_s *g);
void         obj_delete(game_s *g, obj_s *o); // only flags for deletion -> deleted at end of frame
bool32       obj_tag(game_s *g, obj_s *o, int tag);
bool32       obj_untag(game_s *g, obj_s *o, int tag);
obj_s       *obj_get_tagged(game_s *g, int tag);
void         objs_cull_to_delete(game_s *g); // removes all flagged objects
bool32       overlap_obj(obj_s *a, obj_s *b);
rec_i32      obj_aabb(obj_s *o);
rec_i32      obj_rec_left(obj_s *o);
rec_i32      obj_rec_right(obj_s *o);
rec_i32      obj_rec_bottom(obj_s *o);
rec_i32      obj_rec_top(obj_s *o);
v2_i32       obj_pos_bottom_center(obj_s *o);
v2_i32       obj_pos_center(obj_s *o);
bool32       actor_try_wiggle(game_s *g, obj_s *o);
void         actor_move(game_s *g, obj_s *o, v2_i32 dt);
void         platform_move(game_s *g, obj_s *o, v2_i32 dt);
void         solid_move(game_s *g, obj_s *o, v2_i32 dt);
void         obj_on_squish(game_s *g, obj_s *o);
bool32       obj_grounded(game_s *g, obj_s *o);
bool32       obj_grounded_at_offs(game_s *g, obj_s *o, v2_i32 offs);
bool32       obj_would_fall_down_next(game_s *g, obj_s *o, int xdir); // not on ground returns false
void         squish_delete(game_s *g, obj_s *o);
v2_i32       obj_constrain_to_rope(game_s *g, obj_s *o);

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void    obj_apply_movement(obj_s *o);
enemy_s enemy_default();
#endif