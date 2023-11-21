// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#include "gamedef.h"
#include "rope.h"
#include "spriteanim.h"

#define OBJ_GENERIC_MAGIC 0xDEADBEEFU // used during debug to detect buffer overflows

enum {
    OBJ_ID_NULL,
    OBJ_ID_HERO,
    OBJ_ID_HOOK,
    OBJ_ID_SIGN,
    OBJ_ID_SOLID,
    OBJ_ID_KILLABLE,
    OBJ_ID_DOOR_SLIDE,
    OBJ_ID_SAVEPOINT,
};

enum {
    OBJ_TAG_HERO,
    OBJ_TAG_HOOK,
    //
    NUM_OBJ_TAGS
};
static_assert(NUM_OBJ_TAGS <= 32, "Num obj tags");

#define OBJ_FLAG_MOVER          ((u64)1 << 0)
#define OBJ_FLAG_TILE_COLLISION ((u64)1 << 1)
#define OBJ_FLAG_INTERACTABLE   ((u64)1 << 2)
#define OBJ_FLAG_ACTOR          ((u64)1 << 3)
#define OBJ_FLAG_SOLID          ((u64)1 << 4)
#define OBJ_FLAG_CLAMP_TO_ROOM  ((u64)1 << 5)
#define OBJ_FLAG_KILL_OFFSCREEN ((u64)1 << 6)

enum {
    OBJ_BUMPED_X_NEG  = 1 << 0,
    OBJ_BUMPED_X_POS  = 1 << 1,
    OBJ_BUMPED_Y_NEG  = 1 << 2,
    OBJ_BUMPED_Y_POS  = 1 << 3,
    OBJ_BUMPED_SQUISH = 1 << 4,
    OBJ_BUMPED_X      = OBJ_BUMPED_X_NEG | OBJ_BUMPED_X_POS,
    OBJ_BUMPED_Y      = OBJ_BUMPED_Y_NEG | OBJ_BUMPED_Y_POS,
};

enum {
    OBJ_MOVER_SLOPES      = 1 << 0,
    OBJ_MOVER_GLUE_GROUND = 1 << 1,
};

typedef void (*obj_action_s)(game_s *g, obj_s *o);

typedef struct {
    obj_s *o;
    int    gen;
} obj_handle_s;

struct obj_s {
    GUID_s GUID;
    int    index;
    int    gen;
    int    facing; // -1 left, +1 right

    int     ID;
    flags64 flags;
    flags32 tags;

    flags32 bumpflags; // has to be cleared manually
    flags32 moverflags;
    int     w;
    int     h;
    v2_i32  pos; // position in pixels
    v2_i32  subpos_q8;
    v2_i32  vel_q8;
    v2_i32  vel_prev_q8;
    v2_i32  drag_q8;
    v2_i32  gravity_q8;
    v2_i32  acc_q8;
    v2_i32  tomove;

    int      trigger;
    int      actionID;
    int      subactionID;
    int      state;
    int      animation;
    int      n_hitboxes;
    hitbox_s hitboxes[4];
    int      n_hurtboxes;
    hitbox_s hurtboxes[4];

    i32          jumpticks;
    i32          edgeticks;
    ropenode_s  *ropenode;
    rope_s      *rope;
    int          attached;
    obj_handle_s linked_solid;
    obj_handle_s obj_handles[16];
    spriteanim_s spriteanim[4];

    char filename[64];

    void (*on_squish)(game_s *g, obj_s *o);
};

// generic object with additional memory to be used
// for inheritance
// struct obj_extended {
//         obj_s o;
//         ...
// };

typedef struct obj_generic_s {
    obj_s o;
    char  mem[1024];
#ifdef SYS_DEBUG
    u32 magic; // check memory overwrites
#endif
} obj_generic_s;

obj_handle_s obj_handle_from_obj(obj_s *o);
obj_s       *obj_from_obj_handle(obj_handle_s h);
bool32       obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out);
bool32       obj_handle_valid(obj_handle_s h);
//
obj_s       *obj_create(game_s *g);
void         obj_delete(game_s *g, obj_s *o);
bool32       obj_tag(game_s *g, obj_s *o, int tag);
bool32       obj_untag(game_s *g, obj_s *o, int tag);
obj_s       *obj_get_tagged(game_s *g, int tag);
void         objs_cull_to_delete(game_s *g);
rec_i32      obj_aabb(obj_s *o);
rec_i32      obj_rec_left(obj_s *o);
rec_i32      obj_rec_right(obj_s *o);
rec_i32      obj_rec_bottom(obj_s *o);
rec_i32      obj_rec_top(obj_s *o);
v2_i32       obj_pos_bottom_center(obj_s *o);
v2_i32       obj_pos_center(obj_s *o);
bool32       tiles_solid(game_s *g, rec_i32 r);
void         actor_try_wiggle(game_s *g, obj_s *o);
void         actor_move(game_s *g, obj_s *o, v2_i32 dt);
void         solid_move(game_s *g, obj_s *o, v2_i32 dt);
void         obj_interact(game_s *g, obj_s *o);

void squish_delete(game_s *g, obj_s *o);

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o);

obj_s *obj_hero_create(game_s *g);
obj_s *obj_solid_create(game_s *g);
obj_s *obj_slide_door_create(game_s *g);
obj_s *obj_savepoint_create(game_s *g);

#endif