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

enum obj_flag {
        OBJ_FLAG_NONE,
        OBJ_FLAG_DUMMY,
        OBJ_FLAG_ACTOR,
        OBJ_FLAG_SOLID,
        OBJ_FLAG_HERO,
        OBJ_FLAG_NEW_AREA_COLLIDER,
        OBJ_FLAG_HOOK,
        //
        NUM_OBJ_FLAGS
};

enum obj_tag {
        OBJ_TAG_DUMMY,
        OBJ_TAG_HERO,
        //
        NUM_OBJ_TAGS
};

enum actor_flag {
        ACTOR_FLAG_CLIMB_SLOPES       = 0x1, // can climb 45 slopes
        ACTOR_FLAG_CLIMB_STEEP_SLOPES = 0x2, // can climb steep slopes
        ACTOR_FLAG_GLUE_GROUND        = 0x4, // avoids bumbing down a slope
};

struct obj_s {
        int        gen;
        int        index;
        objflags_s flags;

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
        i32         actorflags;
        objhandle_s linkedsolid;
        bool32      soliddisabled;

        ropenode_s *ropenode;
        rope_s     *rope;

        bool32 attached;

        char        new_mapfile[64];
        pathmover_s path;
};

bool32      objhandle_is_valid(objhandle_s h);
bool32      objhandle_is_null(objhandle_s h);
obj_s      *obj_from_handle(objhandle_s h);
bool32      try_obj_from_handle(objhandle_s h, obj_s **o);
objhandle_s objhandle_from_obj(obj_s *o);
bool32      obj_contained_in_array(obj_s *o, obj_s **arr, int num);
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
void        obj_move_x(game_s *g, obj_s *o, int dx);
void        obj_move_y(game_s *g, obj_s *o, int dy);
bool32      actor_step_x(game_s *g, obj_s *o, int sx);
bool32      actor_step_y(game_s *g, obj_s *o, int sy);
void        solid_move(game_s *g, obj_s *o, int dx, int dy);
void        obj_apply_movement(obj_s *o);
//
void        objset_add_all_in_radius(objset_s       *set,
                                     const objset_s *src,
                                     v2_i32 p, i32 r);
void        objset_add_all_matching(objset_s       *set,
                                    const objset_s *src,
                                    objflags_s flags, int cmpf);
void        objset_del_all_in_radius(objset_s *set, v2_i32 p, i32 r);
void        objset_del_matching(objset_s *set, objflags_s flags, int cmpf);

#endif