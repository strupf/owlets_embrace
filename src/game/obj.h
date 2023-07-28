/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */
/*
 * The obj (object) is a general purpose monolithic thing which could represent
 * nearly everything in the game. See also:
 *   Why an ECS just ain't it - feat. Ryan Fleury (youtu.be/UolgW-Ff4bA)
 *   Handmade Hero Day 277 - The Sparse Entity System (youtu.be/wqpxe-s9xyw)
 */

#ifndef OBJ_H
#define OBJ_H

#include "gamedef.h"

enum actor_flag {
        ACTOR_FLAG_CLIMB_SLOPES = 0x1, // object can climb slopes
        ACTOR_FLAG_GLUE_GROUND  = 0x2, // avoids bumbing down a slope
};

struct obj_s {
        int gen;
        int index;
        u32 objflags;

        obj_s *parent;
        obj_s *next;
        obj_s *prev;
        obj_s *fchild;
        obj_s *lchild;

        i32    w;
        i32    h;
        v2_i32 pos;
        v2_i32 pos_new;
        v2_i32 subpos_q8;
        v2_i32 vel_q8;
        v2_i32 gravity_q8;
        v2_i32 drag_q8;
        i32    actorflags;
};

struct objhandle_s {
        int    gen;
        obj_s *o;
};

enum hero_inp {
        HERO_INP_LEFT  = 0x01,
        HERO_INP_RIGHT = 0x02,
        HERO_INP_UP    = 0x04,
        HERO_INP_DOWN  = 0x08,
        HERO_INP_JUMP  = 0x10,
};

enum hero_const {
        HERO_C_JUMP_INIT = -1000,
        HERO_C_ACCX      = 100,
};

struct hero_s {
        objhandle_s obj;
        i32         jumpticks;
        i32         edgeticks;
        int         inp;  // input mask
        int         inpp; // input mask previous frame
};

bool32      objhandle_is_valid(objhandle_s h);
bool32      objhandle_is_null(objhandle_s h);
obj_s      *obj_from_handle(objhandle_s h);
bool32      try_obj_from_handle(objhandle_s h, obj_s **o);
objhandle_s objhandle_from_obj(obj_s *o);
bool32      obj_contained_in_array(obj_s *o, obj_s **arr, int num);
//
obj_s      *obj_create(game_s *g);
void        obj_delete(game_s *g, obj_s *o);
bool32      obj_is_direct_child(obj_s *o, obj_s *parent);
bool32      obj_is_child_any_depth(obj_s *o, obj_s *parent);
bool32      obj_are_siblings(obj_s *a, obj_s *b);
rec_i32     obj_aabb(obj_s *o);
rec_i32     obj_rec_left(obj_s *o);  // these return a rectangle strip
rec_i32     obj_rec_right(obj_s *o); // just next to the object's aabb
rec_i32     obj_rec_bottom(obj_s *o);
rec_i32     obj_rec_top(obj_s *o);
bool32      obj_step_x(game_s *g, obj_s *o, int sx);
bool32      obj_step_y(game_s *g, obj_s *o, int sy);
void        obj_move_x(game_s *g, obj_s *o, int dx);
void        obj_move_y(game_s *g, obj_s *o, int dy);
void        hero_update(game_s *g, obj_s *o, hero_s *h);

#endif