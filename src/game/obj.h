// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#include "game_def.h"

typedef enum {
        OBJFLAGS_CMP_ZERO,
        OBJFLAGS_CMP_NZERO,
        OBJFLAGS_CMP_EQ,
        OBJFLAGS_CMP_NEQ,
} objflag_cmp_e;

typedef enum {
        OBJFLAGS_OP_PASSTHROUGH,
        OBJFLAGS_OP_AND,
        OBJFLAGS_OP_NAND,
        OBJFLAGS_OP_XOR,
        OBJFLAGS_OP_NOT,
        OBJFLAGS_OP_OR,
} objflag_op_e;

static bool32 objflags_cmp(flags64 a, flags64 b, objflag_cmp_e cmp)
{
        switch (cmp) {
        case OBJFLAGS_CMP_ZERO: return a == 0;
        case OBJFLAGS_CMP_NZERO: return a != 0;
        case OBJFLAGS_CMP_EQ: return a == b;
        case OBJFLAGS_CMP_NEQ: return a != b;
        }
        return 0;
}

static flags64 objflags_op(flags64 a, flags64 b, objflag_op_e op)
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

struct objhandle_s {
        int    gen;
        obj_s *o;
};

struct obj_s {
        int     gen;
        int     index;
        flags64 flags;
        int     dir;
        int     p1;
        int     p2;
        int     ID;
        int     facing;
        int     invincibleticks;
        int     die_animation;
        obj_s  *colliders[16];
        int     n_colliders;
        int     damage;
        int     health;
        int     healthmax;

        struct {
                i32     animation;
                i32     animframe;
                i32     animframe_prev;
                u32     renderpriority;
                rec_i32 spriterec;
                tex_s   spritetex;
                int     spriteflags;
                int     spritemode;
                void (*animate_func)(game_s *g, obj_s *o);
                void (*render_func)(game_s *g, obj_s *o, v2_i32 camp);
        };

        // some more or less generic variables
        int timer;
        int state;
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
        flags32     actorres;
        objhandle_s linkedsolid;
        bool32      soliddisabled;

        void (*onsqueeze)(game_s *g, obj_s *o);
        void (*oninteract)(game_s *g, obj_s *o);
        void (*think_1)(game_s *g, obj_s *o);
        void (*think_2)(game_s *g, obj_s *o);
        void (*ontrigger)(game_s *g, obj_s *o, int triggerID);

        bool32      attached;
        ropenode_s *ropenode;
        rope_s     *rope;

        char filename[64];
};

typedef struct {
        obj_s *a;
        obj_s *b;
} obj_pair_s;

#define OBJ_FLAG_NONE              0
#define OBJ_FLAG_ALIVE             (1ULL << 1)
#define OBJ_FLAG_ACTOR             (1ULL << 2)
#define OBJ_FLAG_SOLID             (1ULL << 3)
#define OBJ_FLAG_HERO              (1ULL << 4)
#define OBJ_FLAG_NEW_AREA_COLLIDER (1ULL << 5)
#define OBJ_FLAG_PICKUP            (1ULL << 6)
#define OBJ_FLAG_UNUSED________    (1ULL << 7)
#define OBJ_FLAG_INTERACT          (1ULL << 8)
#define OBJ_FLAG_MOVABLE           (1ULL << 9)
#define OBJ_FLAG_THINK_1           (1ULL << 10)
#define OBJ_FLAG_THINK_2           (1ULL << 11)
#define OBJ_FLAG_HURTABLE          (1ULL << 12)
#define OBJ_FLAG_ENEMY             (1ULL << 13)
#define OBJ_FLAG_KILL_OFFSCREEN    (1ULL << 14)
#define OBJ_FLAG_HURTS_PLAYER      (1ULL << 15)
#define OBJ_FLAG_CAM_ATTRACTOR     (1ULL << 16)
#define OBJ_FLAG_SPRITE_ANIM       (1ULL << 17)
#define OBJ_FLAG_RENDERABLE        (1ULL << 18)
#define OBJ_FLAG_ANIMATE           (1ULL << 19)
#define OBJ_FLAG_HURTS_ENEMIES     (1ULL << 20)

enum obj_tag {
        OBJ_TAG_DUMMY,
        OBJ_TAG_HERO,
        //
        NUM_OBJ_TAGS
};

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

// bucket of obj matching flags
struct objbucket_s {
        objset_s      set;
        flags64       op_flag[2];
        objflag_op_e  op_func[2];
        flags64       cmp_flag;
        objflag_cmp_e cmp_func;
};

enum obj_bucket {
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
        OBJ_BUCKET_SPRITE_ANIM,
        OBJ_BUCKET_RENDERABLE,
        OBJ_BUCKET_ANIMATE,
        OBJ_BUCKET_ENEMY,
        //
        NUM_OBJ_BUCKETS
};

obj_listc_s objbucket_list(game_s *g, int bucketID);
void        objbucket_copy_to_set(game_s *g, int bucketID, objset_s *set);

enum actor_flag {
        ACTOR_FLAG_CLIMB_SLOPES       = 0x1, // can climb 45 slopes
        ACTOR_FLAG_CLIMB_STEEP_SLOPES = 0x2, // can climb steep slopes
        ACTOR_FLAG_GLUE_GROUND        = 0x4, // avoids bumbing down a slope
};

enum actor_res_flag {
        ACTOR_RES_TOUCHED_X_POS = 0x01,
        ACTOR_RES_TOUCHED_X_NEG = 0x02,
        ACTOR_RES_TOUCHED_Y_POS = 0x04,
        ACTOR_RES_TOUCHED_Y_NEG = 0x08,
        ACTOR_RES_SQUEEZED      = 0x10,
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

#endif