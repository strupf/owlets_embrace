// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef COMPANION_H
#define COMPANION_H

#include "gamedef.h"

enum {
    COMPANION_CS_ANIM_NONE,
    //
    COMPANION_CS_ANIM_FLY,
    COMPANION_CS_ANIM_SIT,
    COMPANION_CS_ANIM_SAD,
    COMPANION_CS_ANIM_NOD,
};

void companion_cs_start(obj_s *o);
void companion_cs_leave(obj_s *o);

// if animID not 0: change animation
// if facing not 0: change facing
void companion_cs_set_anim(obj_s *o, i32 animID, i32 facing);

// move companion to position over t ticks
// call function on arrival
void companion_cs_move_to(obj_s *o, v2_i32 p, i32 t,
                          void (*arrived_cb)(g_s *g, obj_s *o));

#endif