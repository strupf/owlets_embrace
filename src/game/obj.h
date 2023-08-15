// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
/*
 * The obj (object) is a general purpose monolithic thing which could represent
 * nearly everything in the game - it's mostly used for simplicity reasons.
 * It is yet to be seen if that turns out to be a performance issue.
 * See also:
 *   Why an ECS just ain't it - feat. Ryan Fleury (youtu.be/UolgW-Ff4bA)
 *   Handmade Hero Day 277 - The Sparse Entity System (youtu.be/wqpxe-s9xyw)
 */

#ifndef OBJ_H
#define OBJ_H

#include "gamedef.h"
#include "objflags.h"
#include "pathmovement.h"
#include "rope.h"

// movement is going to work similar like in Celeste
// maddythorson.medium.com/celeste-and-towerfall-physics-d24bd2ae0fc5

enum actor_flag {
        ACTOR_FLAG_CLIMB_SLOPES       = 0x1, // can climb 45 slopes
        ACTOR_FLAG_CLIMB_STEEP_SLOPES = 0x2, // can climb steep slopes
        ACTOR_FLAG_GLUE_GROUND        = 0x4, // avoids bumbing down a slope
};

typedef struct {
        void (*action)(game_s *g, void *arg);
        void *actionarg;
        int   x;
} pickupdata_s;

typedef enum {
        DOORTYPE_KEY,
        DOORTYPE_ACTIVATION,
        DOORTYPE_HIT,
} doortype_e;

typedef enum {
        DOOROPEN_SLIDE_TOP,
        DOOROPEN_DELETE,
        DOOROPEN_CUSTOM,
} dooropen_e;

typedef struct {
        doortype_e type;
        dooropen_e open;
        bool32     triggered;
        int        moved;
} doordata_s;

typedef void (*objfunc_f)(game_s *g, obj_s *o, void *arg);
typedef void (*objtrigger_f)(game_s *g, obj_s *o, int triggerID);

struct obj_s {
        int        gen;
        int        index;
        objflags_s flags;
        int        dir;
        int        p1;
        int        p2;
        int        ID;
        int        facing;

        obj_s *parent;
        obj_s *next;
        obj_s *prev;
        obj_s *fchild;
        obj_s *lchild;

        i32         w;
        i32         h;
        v2_i32      pos;
        v2_i32      pos_new;
        v2_i32      subpos_q8;
        v2_i32      vel_q8;
        v2_i32      gravity_q8;
        v2_i32      drag_q8;
        bool32      squeezed;
        i32         actorflags;
        objhandle_s linkedsolid;
        bool32      soliddisabled;

        objfunc_f    onsqueeze;
        objfunc_f    oninteract;
        objfunc_f    think_1;
        objfunc_f    think_2;
        objtrigger_f ontrigger;

        void *userarg;

        bool32      attached;
        ropenode_s *ropenode;
        rope_s     *rope;

        pickupdata_s pickup;
        doordata_s   door;

        char        filename[64];
        pathmover_s path;
};

typedef struct obj_filter_s {
        objflags_s    op_flag[2];
        objflag_op_e  op_func[2];
        objflags_s    cmp_flag;
        objflag_cmp_e cmp_func;
} obj_filter_s;

bool32      obj_matches_filter(obj_s *o, obj_filter_s filter);
bool32      objhandle_is_valid(objhandle_s h);
bool32      objhandle_is_null(objhandle_s h);
obj_s      *obj_from_handle(objhandle_s h);
bool32      try_obj_from_handle(objhandle_s h, obj_s **o);
objhandle_s objhandle_from_obj(obj_s *o);
bool32      obj_contained_in_array(obj_s *o, obj_s **arr, int num);
void        objset_add_all(game_s *g, objset_s *set);
//
obj_s      *obj_create(game_s *g);
void        obj_delete(game_s *g, obj_s *o); // object still lives until the end of the frame
void        obj_set_flags(game_s *g, obj_s *o, objflags_s flags);
bool32      obj_is_direct_child(obj_s *o, obj_s *parent);
bool32      obj_is_child_any_depth(obj_s *o, obj_s *parent);
bool32      obj_are_siblings(obj_s *a, obj_s *b);
v2_i32      obj_aabb_center(obj_s *o);
rec_i32     obj_aabb(obj_s *o);
rec_i32     obj_rec_left(obj_s *o);  // these return a rectangle strip
rec_i32     obj_rec_right(obj_s *o); // just next to the object's aabb
rec_i32     obj_rec_bottom(obj_s *o);
rec_i32     obj_rec_top(obj_s *o);
void        actor_move_x(game_s *g, obj_s *o, int dx);
void        actor_move_y(game_s *g, obj_s *o, int dy);
void        solid_move(game_s *g, obj_s *o, int dx, int dy);
void        obj_apply_movement(obj_s *o);
obj_s      *interactable_closest(game_s *g, v2_i32 p);
void        interact_open_dialogue(game_s *g, obj_s *o);

#endif