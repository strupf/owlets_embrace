/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */
/*
 * An objset is a sparse array which can only contain
 * unique object pointers. Insertion, deletion and finding
 * is O(1). The actual object pointers are packed and start
 * at index 1 (0 is invalid).
 *
 * The zero object at index 0 serves as a tombstone object
 * for quickier containment checks and zeroing:
 * Usually one would have to check dense[sparse[i]] == i for
 * containment. By disallowing 0 as a lookup key we simply can
 * check sparse[i] != 0 for containment.
 *
 * Also we can simply memset 0 the structure for clearance.
 */

#ifndef OBJSET_H
#define OBJSET_H

#include "gamedef.h"

struct objset_s {
        int    n;
        obj_s *o[NUM_OBJS];
        u16    d[NUM_OBJS];
        u16    s[NUM_OBJS];
};

objset_s *objset_create(void *(*allocfunc)(size_t));
bool32    objset_add(objset_s *set, obj_s *o);
bool32    objset_del(objset_s *set, obj_s *o);
bool32    objset_contains(objset_s *set, obj_s *o);
void      objset_clr(objset_s *set);
int       objset_len(objset_s *set);
obj_s    *objset_at(objset_s *set, int i);

#endif