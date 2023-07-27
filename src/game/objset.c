/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "objset.h"
#include "obj.h"

objset_s *objset_create(void *(*allocfunc)(size_t))
{
        objset_s *set = (objset_s *)allocfunc(sizeof(objset_s));
        objset_clr(set);
        return set;
}

bool32 objset_add(objset_s *set, obj_s *o)
{
        int i = o->index;
        if (i == 0 || set->s[i] != 0) return 0;
        int n     = ++set->n;
        set->s[i] = n;
        set->d[n] = i;
        set->o[n] = o;
        return 1;
}

bool32 objset_del(objset_s *set, obj_s *o)
{
        int i = o->index;
        int j = set->s[i];
        if (j == 0) return 0;
        int n     = set->n--;
        int k     = set->d[n];
        set->d[n] = 0;
        set->d[j] = k;
        set->s[k] = j;
        set->s[i] = 0;
        set->o[j] = set->o[n];
        return 1;
}

bool32 objset_contains(objset_s *set, obj_s *o)
{
        int i = o->index;
        return (set->s[i] != 0);
}

void objset_clr(objset_s *set)
{
        static const objset_s objset0 = {0};

        *set = objset0;
}

int objset_len(objset_s *set)
{
        return set->n;
}

obj_s *objset_at(objset_s *set, int i)
{
        ASSERT(0 <= i && i < set->n);
        return set->o[1 + i];
}
