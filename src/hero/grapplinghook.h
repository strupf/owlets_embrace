// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GRAPPLINGHOOK_H
#define GRAPPLINGHOOK_H

#include "gamedef.h"
#include "rope.h"

enum {
    GRAPPLINGHOOK_FLYING,
    GRAPPLINGHOOK_HOOKED_SOLID,
    GRAPPLINGHOOK_HOOKED_TERRAIN,
    GRAPPLINGHOOK_HOOKED_OBJ,
};

typedef struct {
    i32          state;
    v2_i32       p;
    v2_i16       p_q8;
    v2_i16       v_q8;
    obj_handle_s o1;
    obj_handle_s o2;
    v2_i16       solid_offs;
    ropenode_s  *rn;
    rope_s       rope;
} grapplinghook_s;
#endif