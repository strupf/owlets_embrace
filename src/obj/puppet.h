// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PUPPET_H
#define PUPPET_H

#include "gamedef.h"

enum {
    PUPPET_OWL_ANIMID_NONE,
    //
    PUPPET_OWL_ANIMID_IDLE,
    PUPPET_OWL_ANIMID_WALK_SLOW,
    PUPPET_OWL_ANIMID_WALK,
    PUPPET_OWL_ANIMID_SPRINT,
    PUPPET_OWL_ANIMID_UPGR_RISE,
    PUPPET_OWL_ANIMID_UPGR_INTENSE,
    PUPPET_OWL_ANIMID_UPGR_CALM,
    PUPPET_OWL_ANIMID_SHOOK,
    PUPPET_OWL_ANIMID_AVENGE,
    PUPPET_OWL_ANIMID_QUICKDUCK,
    PUPPET_OWL_ANIMID_HOLD_ARM,
    PUPPET_OWL_ANIMID_PRESENT_ABOVE,              // receive item frame
    PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP,         // receive item frame
    PUPPET_OWL_ANIMID_PRESENT_ABOVE_TO_IDLE,      // receive item frame
    PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP_TO_IDLE, // receive item frame
    PUPPET_OWL_ANIMID_OFF_BALANCE,
    PUPPET_OWL_ANIMID_OFF_BALANCE_COMP,
    PUPPET_OWL_ANIMID_FALL_ASLEEP,
    PUPPET_OWL_ANIMID_SLEEP,
    PUPPET_OWL_ANIMID_SLEEP_WAKEUP,
};

enum {
    PUPPET_COMPANION_ANIMID_NONE,
    //
    PUPPET_COMPANION_ANIMID_FLY,
    PUPPET_COMPANION_ANIMID_SAD,
    PUPPET_COMPANION_ANIMID_SIT,
    PUPPET_COMPANION_ANIMID_NOD,
    PUPPET_COMPANION_ANIMID_HUH,
    PUPPET_COMPANION_ANIMID_BUMP_ONCE,
    PUPPET_COMPANION_ANIMID_NOD_ONCE,
    PUPPET_COMPANION_ANIMID_TUMBLE,
    PUPPET_COMPANION_ANIMID_SLEEP,
    PUPPET_COMPANION_ANIMID_WAKEUP,
};

enum {
    PUPPET_MOLE_ANIMID_NONE,
    //
    PUPPET_MOLE_ANIMID_IDLE,
    PUPPET_MOLE_ANIMID_WALK,
    PUPPET_MOLE_ANIMID_DIG_OUT,
    PUPPET_MOLE_ANIMID_DIG_IN,
};

typedef struct puppet_s {
    v2_i32 p_src;
    v2_i32 p_dst;
    void (*arrived_cb)(g_s *g, obj_s *o, void *ctx);
    ease_i32 movefunc;
    void    *arrived_ctx;
    u16      animID;
    u16      anim_t;
} puppet_s;

obj_s *puppet_create(g_s *g, i32 objID_puppet);
void   puppet_on_animate(g_s *g, obj_s *o);

// if animID not 0: change animation
// if facing not 0: change facing
void puppet_set_anim(obj_s *o, i32 animID, i32 facing);

// move companion to position over t ticks
// call function on arrival
void puppet_move(obj_s *o, v2_i32 p, i32 t);
void puppet_move_ext(obj_s *o, v2_i32 p, i32 t, ease_i32 movefunc, bool32 relative,
                     void (*arrived_cb)(g_s *g, obj_s *o, void *ctx),
                     void *ctx);

obj_s *puppet_owl_put(g_s *g, obj_s *ohero);
void   puppet_owl_replace_and_del(g_s *g, obj_s *ohero, obj_s *o);
obj_s *puppet_companion_put(g_s *g, obj_s *ocomp);
void   puppet_companion_replace_and_del(g_s *g, obj_s *ocomp, obj_s *opuppet);
obj_s *puppet_mole_create(g_s *g, v2_i32 pfeet);

#endif