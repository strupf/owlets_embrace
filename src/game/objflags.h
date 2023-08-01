// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
/*
 * Object flags is a bitset because the objects will probably have
 * more than 32 flags. Using a separate structure the set can be
 * extended easily.
 */

#ifndef OBJFLAGS_H
#define OBJFLAGS_H

#include "gamedef.h"

struct objflags_s {
        u32 i[2];
};

static inline objflags_s objflags_0()
{
        objflags_s r = {0};
        return r;
}

static inline objflags_s objflags_F()
{
        objflags_s r = {0xFFFFFFFFU, 0xFFFFFFFFU};
        return r;
}

// use variadic arguments so I can just use an enum for the
// flags instead of setting each bit manually
static objflags_s i_objflags_create(int n, ...)
{
        objflags_s r = {0};
        va_list    ap;
        va_start(ap, n);
        while (1) {
                int i = va_arg(ap, int);
                if (i == -1) break;
                r.i[i >> 5] |= 1 << (i & 31);
        }
        va_end(ap);
        return r;
}
#define objflags_create(...) i_objflags_create(-1, __VA_ARGS__, -1)

static inline objflags_s objflags_set(objflags_s a, int bit)
{
        objflags_s r = a;
        r.i[bit >> 5] |= (1U << bit);
        return r;
}

static inline objflags_s objflags_unset(objflags_s a, int bit)
{
        objflags_s r = a;
        r.i[bit >> 5] &= ~(1U << bit);
        return r;
}

static inline bool32 objflags_cmp_zero(objflags_s a)
{
        return (a.i[0] == 0 && a.i[1] == 0);
}

static inline bool32 objflags_cmp_nzero(objflags_s a)
{
        return (a.i[0] != 0 || a.i[1] != 0);
}

static inline bool32 objflags_cmp_eq(objflags_s a, objflags_s b)
{
        return (a.i[0] == b.i[0] && a.i[1] == b.i[1]);
}

static inline bool32 objflags_cmp_neq(objflags_s a, objflags_s b)
{
        return (a.i[0] != b.i[0] || a.i[1] != b.i[1]);
}

static inline objflags_s objflags_not(objflags_s a)
{
        objflags_s r = {~a.i[0],
                        ~a.i[1]};
        return r;
}

static inline objflags_s objflags_nand(objflags_s a, objflags_s b)
{
        objflags_s r = {~(a.i[0] & b.i[0]),
                        ~(a.i[1] & b.i[1])};
        return r;
}

static inline objflags_s objflags_and(objflags_s a, objflags_s b)
{
        objflags_s r = {a.i[0] & b.i[0],
                        a.i[1] & b.i[1]};
        return r;
}

static inline objflags_s objflags_or(objflags_s a, objflags_s b)
{
        objflags_s r = {a.i[0] | b.i[0],
                        a.i[1] | b.i[1]};
        return r;
}

static inline objflags_s objflags_xor(objflags_s a, objflags_s b)
{
        objflags_s r = {a.i[0] ^ b.i[0],
                        a.i[1] ^ b.i[1]};
        return r;
}

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

static bool32 objflags_cmp(objflags_s a, objflags_s b, int cmp)
{
        switch (cmp) {
        case OBJFLAGS_CMP_ZERO: return objflags_cmp_zero(a);
        case OBJFLAGS_CMP_NZERO: return objflags_cmp_nzero(a);
        case OBJFLAGS_CMP_EQ: return objflags_cmp_eq(a, b);
        case OBJFLAGS_CMP_NEQ: return objflags_cmp_neq(a, b);
        }
        return 0;
}

static objflags_s objflags_op(objflags_s a, objflags_s b, int op)
{
        switch (op) {
        case OBJFLAGS_OP_PASSTHROUGH: return a;
        case OBJFLAGS_OP_NAND: return objflags_nand(a, b);
        case OBJFLAGS_OP_AND: return objflags_and(a, b);
        case OBJFLAGS_OP_XOR: return objflags_xor(a, b);
        case OBJFLAGS_OP_OR: return objflags_or(a, b);
        case OBJFLAGS_OP_NOT: return objflags_not(a);
        }
        return a;
}

#endif