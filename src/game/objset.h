// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
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

/* this is supposed to be a "const list", as in, don't modify the
 * actual object array because it's a pointer into the packed object
 * array of the backing objset
 */
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

#endif