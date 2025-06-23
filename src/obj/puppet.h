// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CS_OBJ_H
#define CS_OBJ_H

#include "gamedef.h"

enum {
    PUPPET_HERO_ANIMID_NONE,
    //
    PUPPET_HERO_ANIMID_IDLE,
    PUPPET_HERO_ANIMID_WALK_SLOW,
    PUPPET_HERO_ANIMID_WALK,
    PUPPET_HERO_ANIMID_SPRINT,
    PUPPET_HERO_ANIMID_UPGR_RISE,
    PUPPET_HERO_ANIMID_UPGR_INTENSE,
    PUPPET_HERO_ANIMID_UPGR_CALM,
    PUPPET_HERO_ANIMID_SHOOK,
    PUPPET_HERO_ANIMID_AVENGE,
    PUPPET_HERO_ANIMID_QUICKDUCK,
    PUPPET_HERO_ANIMID_HOLD_ARM,
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

// if animID not 0: change animation
// if facing not 0: change facing
void puppet_set_anim(obj_s *o, i32 animID, i32 facing);

// move companion to position over t ticks
// call function on arrival
void puppet_move(obj_s *o, v2_i32 p, i32 t);
void puppet_move_ext(obj_s *o, v2_i32 p, i32 t, ease_i32 movefunc, bool32 relative,
                     void (*arrived_cb)(g_s *g, obj_s *o, void *ctx),
                     void *ctx);

obj_s *puppet_hero_put(g_s *g, obj_s *ohero);
void   puppet_hero_replace_and_del(g_s *g, obj_s *ohero, obj_s *o);
obj_s *puppet_companion_put(g_s *g, obj_s *ocomp);
void   puppet_companion_replace_and_del(g_s *g, obj_s *ocomp, obj_s *o);

#endif