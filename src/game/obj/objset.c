// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

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
        if (set->n > 0)
                *set = (const objset_s){0};
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

obj_listc_s objset_list(objset_s *set)
{
        obj_listc_s l = {&set->o[1], set->n};
        return l;
}

void objset_print(objset_s *set)
{
        for (int n = 0; n < NUM_OBJS; n++) {
                PRINTF("%i| s: %i d: %i | o: %i %i\n", n, set->s[n], set->d[n], set->o[n] ? set->o[n]->index : -1, set->o[n] ? set->o[n]->ID : -1);
        }
}

static void objset_swap(objset_s *set, int i, int j)
{
        int si = set->s[i];
        int sj = set->s[j];
        SWAP(obj_s *, set->o[si], set->o[sj]);
        SWAP(int, set->d[si], set->d[sj]);
        SWAP(int, set->s[i], set->s[j]);
}

void objset_sort(objset_s *set, int (*cmpf)(const obj_s *a, const obj_s *b))
{
        while (1) {
                bool32 swapped = 0;
                for (int n = 1; n < set->n; n++) {
                        obj_s *a = set->o[n + 1];
                        obj_s *b = set->o[n];
                        if (cmpf(a, b) > 0) {
                                objset_swap(set, a->index, b->index);
                                swapped = 1;
                        }
                }
                if (!swapped) break;
        }
}