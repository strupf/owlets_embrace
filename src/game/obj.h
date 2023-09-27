// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#include "game_def.h"

enum {
        OBJ_ID_NULL = 100,
        OBJ_ID_HERO,
        OBJ_ID_HOOK,
        OBJ_ID_SIGN,
        OBJ_ID_NPC,
        OBJ_ID_BLOB,
        OBJ_ID_BOAT,
        OBJ_ID_DOOR,
        OBJ_ID_ARROW,
        OBJ_ID_BOMB,
        OBJ_ID_CRUMBLEBLOCK,
        OBJ_ID_SAVEPOINT,
};

enum {
        OBJ_TAG_DUMMY,
        OBJ_TAG_HERO,
        //
        NUM_OBJ_TAGS
};

enum {
        OBJ_BUCKET_ALIVE,
        OBJ_BUCKET_ACTOR,
        OBJ_BUCKET_SOLID,
        OBJ_BUCKET_NEW_AREA_COLLIDER,
        OBJ_BUCKET_PICKUP,
        OBJ_BUCKET_INTERACT,
        OBJ_BUCKET_MOVABLE,
        OBJ_BUCKET_THINK_1,
        OBJ_BUCKET_THINK_2,
        OBJ_BUCKET_HURTABLE,
        OBJ_BUCKET_KILL_OFFSCREEN,
        OBJ_BUCKET_HURTS_ENEMIES,
        OBJ_BUCKET_HURTS_PLAYER,
        OBJ_BUCKET_CAM_ATTRACTOR,
        OBJ_BUCKET_RENDERABLE,
        OBJ_BUCKET_ANIMATE,
        OBJ_BUCKET_ENEMY,
        //
        NUM_OBJ_BUCKETS
};

enum {
        INTERACTABLE_TYPE_DEFAULT,
        INTERACTABLE_TYPE_SPEAK,
        INTERACTABLE_TYPE_READ,
};

#define OBJ_FLAG_NONE              0
#define OBJ_FLAG_ALIVE             (1ULL << 1)
#define OBJ_FLAG_ACTOR             (1ULL << 2)
#define OBJ_FLAG_SOLID             (1ULL << 3)
#define OBJ_FLAG______UNUSED3      (1ULL << 4)
#define OBJ_FLAG_NEW_AREA_COLLIDER (1ULL << 5)
#define OBJ_FLAG_PICKUP            (1ULL << 6)
#define OBJ_FLAG______UNUSED1      (1ULL << 7)
#define OBJ_FLAG_INTERACT          (1ULL << 8)
#define OBJ_FLAG_MOVABLE           (1ULL << 9)
#define OBJ_FLAG_THINK_1           (1ULL << 10)
#define OBJ_FLAG_THINK_2           (1ULL << 11)
#define OBJ_FLAG_HURTABLE          (1ULL << 12)
#define OBJ_FLAG_ENEMY             (1ULL << 13)
#define OBJ_FLAG_KILL_OFFSCREEN    (1ULL << 14)
#define OBJ_FLAG_HURTS_PLAYER      (1ULL << 15)
#define OBJ_FLAG_CAM_ATTRACTOR     (1ULL << 16)
#define OBJ_FLAG______UNUSED2      (1ULL << 17)
#define OBJ_FLAG_RENDERABLE        (1ULL << 18)
#define OBJ_FLAG_ANIMATE           (1ULL << 19)
#define OBJ_FLAG_HURTS_ENEMIES     (1ULL << 20)

enum {
        OBJFLAGS_CMP_ZERO,
        OBJFLAGS_CMP_NZERO,
        OBJFLAGS_CMP_EQ,
        OBJFLAGS_CMP_NEQ,
};

enum {
        OBJFLAGS_OP_PASSTHROUGH,
        OBJFLAGS_OP_AND,
        OBJFLAGS_OP_NAND,
        OBJFLAGS_OP_XOR,
        OBJFLAGS_OP_NOT,
        OBJFLAGS_OP_OR,
};

static bool32 objflags_cmp(flags64 a, flags64 b, int cmp)
{
        switch (cmp) {
        case OBJFLAGS_CMP_ZERO: return a == 0;
        case OBJFLAGS_CMP_NZERO: return a != 0;
        case OBJFLAGS_CMP_EQ: return a == b;
        case OBJFLAGS_CMP_NEQ: return a != b;
        }
        return 0;
}

static flags64 objflags_op(flags64 a, flags64 b, int op)
{
        switch (op) {
        case OBJFLAGS_OP_PASSTHROUGH: return a;
        case OBJFLAGS_OP_NAND: return (a & ~b);
        case OBJFLAGS_OP_AND: return (a & b);
        case OBJFLAGS_OP_XOR: return (a ^ b);
        case OBJFLAGS_OP_OR: return (a | b);
        case OBJFLAGS_OP_NOT: return (~a);
        }
        return a;
}

enum {
        DAMAGE_SRC_SPIKE        = 0x0001,
        DAMAGE_SRC_LAVA         = 0x0002,
        //
        DAMAGE_SRC_ENEMY_LIGHT  = 0x0100,
        DAMAGE_SRC_ENEMY_NORMAL = 0x0200,
        DAMAGE_SRC_ENEMY_HARD   = 0x0400,
        //
        DAMAGE_SRC_HERO_LIGHT   = 0x1000,
        DAMAGE_SRC_HERO_NORMAL  = 0x2000,
        DAMAGE_SRC_HERO_HARD    = 0x4000,
};

struct objhandle_s {
        int    gen;
        obj_s *o;
};

typedef struct {
        bool32  hidden;
        int     texID;
        int     flags;
        int     mode;
        rec_i32 r;
        v2_i32  offset;
} objsprite_s;

struct obj_s {
        u32 magic;

        int     gen;
        int     index;
        flags64 flags;
        flags32 tags;
        int     ID;
        u32     tiledID; // object ID from tiled map editor
        int     facing;
        int     invincibleticks;
        int     die_animation;

        obj_s *colliders[64];
        int    n_colliders;
        void (*handleobjcollision)(game_s *g, obj_s *o);

        int damage;
        int health;
        int healthmax;

        // some more or less generic variables
        int timer;
        int state;
        int substate;
        int type;

        i32 w;
        i32 h;

        v2_i32      pos;
        v2_i32      subpos_q8;
        v2_i32      vel_q8;
        v2_i32      gravity_q8;
        v2_i32      drag_q8;
        v2_i32      tomove; // distance to move the obj
        v2_i32      vel_prev_q8;
        flags32     actorflags;
        bool32      squeezed;
        objhandle_s linkedsolid;
        bool32      soliddisabled;

        void (*animate_func)(game_s *g, obj_s *o);
        void (*onsqueeze)(game_s *g, obj_s *o);
        void (*oninteract)(game_s *g, obj_s *o);
        void (*think_1)(game_s *g, obj_s *o);
        void (*think_2)(game_s *g, obj_s *o);
        void (*ontrigger)(game_s *g, obj_s *o, int triggerID);
        void (*ondelete)(game_s *g, obj_s *o);
        void (*renderfunc)(game_s *g, obj_s *o, v2_i32 camp);
        int animation;
        int interactable_type;

        bool32      attached;
        ropenode_s *ropenode;
        rope_s     *rope;

        char filename[64];
};

typedef struct { // struct with additional memory to use inheritance
        obj_s o;
        char  mem[0x1000];
        u32   magic;
} obj_generic_s;

typedef struct {
        obj_s *a;
        obj_s *b;
} obj_pair_s;

typedef struct {
        rec_i32 r;
        int     flags;
        int     damage;
} hitbox_s;

obj_s      *obj_get_tagged(game_s *g, int tag);
bool32      obj_tag(game_s *g, obj_s *o, int tag);
bool32      obj_untag(game_s *g, obj_s *o, int tag);
bool32      obj_is_tagged(obj_s *o, int tag);
//
bool32      objhandle_is_valid(objhandle_s h);
bool32      objhandle_is_null(objhandle_s h);
obj_s      *obj_from_handle(objhandle_s h);
objhandle_s objhandle_from_obj(obj_s *o);
bool32      try_obj_from_handle(objhandle_s h, obj_s **o);
//
obj_s      *obj_create(game_s *g);
void        obj_delete(game_s *g, obj_s *o); // schedule for deletion; still lives until cull
bool32      obj_contained_in_array(obj_s *o, obj_s **arr, int num);
v2_i32      obj_aabb_center(obj_s *o);
rec_i32     obj_aabb(obj_s *o);
rec_i32     obj_rec_left(obj_s *o);  // these return a rectangle strip
rec_i32     obj_rec_right(obj_s *o); // just next to the object's aabb
rec_i32     obj_rec_bottom(obj_s *o);
rec_i32     obj_rec_top(obj_s *o);
void        obj_apply_flags(game_s *g, obj_s *o, flags64 flags);
void        obj_set_flags(game_s *g, obj_s *o, flags64 flags);
void        obj_unset_flags(game_s *g, obj_s *o, flags64 flags);
void        obj_interact_open_dialog(game_s *g, obj_s *o);
obj_s      *obj_closest_interactable(game_s *g, v2_i32 pos);
int         obj_get_with_ID(game_s *g, int ID, objset_s *set);
bool32      obj_overlaps_spikes(game_s *g, obj_s *o);

// sparse set / slot map obj - ID, ID - obj
struct objset_s {
        int    n;
        obj_s *o[NUM_OBJS];
        u16    d[NUM_OBJS];
        u16    s[NUM_OBJS];
};

// constant listing into the dense obj array of the slot map
struct obj_listc_s {
        obj_s *const *o;
        const int     n;
};

objset_s   *objset_create(void *(*allocfunc)(size_t));
bool32      objset_add(objset_s *set, obj_s *o);
bool32      objset_del(objset_s *set, obj_s *o);
bool32      objset_contains(objset_s *set, obj_s *o);
void        objset_clr(objset_s *set);
int         objset_len(objset_s *set);
obj_s      *objset_at(objset_s *set, int i);
obj_listc_s objset_list(objset_s *set);
void        objset_sort(objset_s *set, int (*cmpf)(const obj_s *a, const obj_s *b));
void        objset_print(objset_s *set);
//
void        objset_filter_overlap_circ(objset_s *set, v2_i32 p, i32 r, bool32 inv);
void        objset_filter_in_distance(objset_s *set, v2_i32 p, i32 r, bool32 inv);
void        objset_filter_overlap_rec(objset_s *set, rec_i32 r, bool32 inv);

// bucket of obj matching flags
struct objbucket_s {
        objset_s set;
        flags64  op_flag[2];
        int      op_func[2];
        flags64  cmp_flag;
        int      cmp_func;
};

obj_listc_s obj_list_all(game_s *g);
obj_listc_s objbucket_list(game_s *g, int bucketID);
void        objbucket_copy_to_set(game_s *g, int bucketID, objset_s *set);

enum {
        ACTOR_FLAG_CLIMB_SLOPES       = 0x1, // can climb 45 slopes
        ACTOR_FLAG_CLIMB_STEEP_SLOPES = 0x2, // can climb steep slopes
        ACTOR_FLAG_GLUE_GROUND        = 0x4, // avoids bumbing down a slope
};

void   actor_move(game_s *g, obj_s *o, int dx, int dy);
void   solid_move(game_s *g, obj_s *o, int dx, int dy);
bool32 solid_occupies(obj_s *solid, rec_i32 r);
void   obj_apply_movement(obj_s *o);
//
obj_s *arrow_create(game_s *g, v2_i32 p, v2_i32 v_q8);
obj_s *blob_create(game_s *g);
obj_s *door_create(game_s *g);
obj_s *npc_create(game_s *g);
obj_s *bomb_create(game_s *g, v2_i32 p, v2_i32 v_q8);
obj_s *crumbleblock_create(game_s *g);
obj_s *boat_create(game_s *g);
obj_s *savepoint_create(game_s *g);
obj_s *sign_create(game_s *g);

#endif