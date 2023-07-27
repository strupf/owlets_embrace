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

struct obj_s {
        int gen;
        int index;
        u32 objflags;

        obj_s *parent;
        obj_s *next;
        obj_s *prev;
        obj_s *fchild;
        obj_s *lchild;

        void (*on_parent_add)(game_s *g, obj_s *o, obj_s *parent);
        void (*on_parent_del)(game_s *g, obj_s *o, obj_s *parent);
        void (*on_child_add)(game_s *g, obj_s *o, obj_s *child);
        void (*on_child_del)(game_s *g, obj_s *o, obj_s *child);

        i32    w;
        i32    h;
        v2_i32 pos;
        v2_i32 subpos_q8;
        v2_i32 vel_q8;
        v2_i32 gravity_q8;
        v2_i32 drag_q8;
};

struct objhandle_s {
        int    gen;
        obj_s *o;
};

bool32      objhandle_is_valid(objhandle_s h);
bool32      objhandle_is_null(objhandle_s h);
obj_s      *obj_from_objhandle(objhandle_s h);
bool32      objhandle_try_dereference(objhandle_s h, obj_s **o);
objhandle_s objhandle_from_obj(obj_s *o);
bool32      obj_contained_in_array(obj_s *o, obj_s **arr, int num);
//
obj_s      *obj_create(game_s *g);
void        obj_delete(game_s *g, obj_s *o);
bool32      obj_is_direct_child(obj_s *o, obj_s *parent);
bool32      obj_is_child_any_depth(obj_s *o, obj_s *parent);
bool32      obj_are_siblings(obj_s *a, obj_s *b);
rec_i32     obj_aabb(obj_s *o);
void        obj_move_x(game_s *g, obj_s *o, i32 dx);
void        obj_move_y(game_s *g, obj_s *o, i32 dy);

#endif