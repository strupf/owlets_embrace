// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// hitboxes are deferred to the end of the frame instant of being applied directly

#ifndef HITBOX_H
#define HITBOX_H

#include "gamedef.h"

#define HITBOX_NUM 256

enum {
    HITBOXID_NULL,
    //
    HITBOXID_OWL_0,
    HITBOXID_OWL_WING,
    HITBOXID_OWL_BEAK,
    HITBOXID_OWL_DOWN,
    HITBOXID_OWL_STOMP,
    HITBOXID_OWL_POWERSTOMP,
    HITBOXID_OWL_1,
    //
    HITBOXID_ENEMY_0,
    HITBOXID_ENEMY_AABB,
    HITBOXID_CRAB_SLASH,
    HITBOXID_FLYBLOB,
    HITBOXID_ENEMY_1,
};

enum {
    HITBOX_TYPE_NULL,
    HITBOX_TYPE_PARENT,
    HITBOX_TYPE_REC,
    HITBOX_TYPE_CIR,
    HITBOX_TYPE_LIN,
};

enum {
    HITBOX_FLAG_GROUP_TRIGGERS_CALLBACK = 1 << 0, // whether the hit objects causes the set callback to run
    HITBOX_FLAG_GROUP_PLAYER            = 1 << 1,
    HITBOX_FLAG_GROUP_ENEMY             = 1 << 2,
    HITBOX_FLAG_GROUP_ENVIRONMENT       = 1 << 3,

    //
    HITBOX_FLAG_HIT = 1 << 7, // set by engine!
};

typedef struct hitbox_s hitbox_s;

typedef struct {
    i32 damage;
    i16 dx_q4;
    i16 dy_q4;
} hitbox_res_s;

typedef void (*hitbox_cb_f)(g_s *g, hitbox_s *hb, void *ctx);

typedef struct {
    i32 x1;
    i32 y1;
    i32 x2;
    i32 y2;
} hitbox_type_rec_s;

typedef struct {
    i32 x;
    i32 y;
    i32 r;
} hitbox_type_cir_s;

typedef struct {
    i32 x;
    i32 y;
    i16 dx;
    i16 dy;
    i16 r;
} hitbox_type_lin_s;

struct hitbox_s {
    ALIGNAS(32)
    u32          UID;    // unique ID, incremented per hitbox
    hitbox_cb_f  cb;     // void f(g_s *g, hitbox_s *hb, void *arg); callback function notify the caller that this hitbox hit
    void        *cb_arg; // user argument for callback
    obj_handle_s user;
    u8           flags_group;
    u8           ID;          // identifier indicating what specific kind of attack
    u8           parent_offs; // if has parent: distance in array to parent

    u8  type;
    i8  damage;
    u8  flags;
    i16 dx_q4;
    i16 dy_q4;
    union {
        i32               n_c; // number of children if parent
        hitbox_type_rec_s rec;
        hitbox_type_cir_s cir;
        hitbox_type_lin_s lin;
    } u;
};

u32       hitbox_UID_gen(g_s *g);
hitbox_s *hitbox_gen(g_s *g, u32 UID, i32 ID, hitbox_cb_f cb, void *cb_arg); // add a hitbox with UID; if 0 then the hitbox will receive a new UID
void      hitbox_set_user(hitbox_s *hb, obj_s *o);                           // sets a user to not be able to hit itself even if in the same group
void      hitbox_set_flags_group(hitbox_s *hb, i32 flags);
void      hitbox_type_parent(hitbox_s *hb);
void      hitbox_parent_add_child(hitbox_s *hb_parent, hitbox_s *hb_child);
void      hitbox_type_recr(hitbox_s *hb, rec_i32 r);
void      hitbox_type_recxy(hitbox_s *hb, i32 x1, i32 y1, i32 x2, i32 y2);
void      hitbox_type_rec(hitbox_s *hb, i32 x, i32 y, i32 w, i32 h);
void      hitbox_type_cir(hitbox_s *hb, i32 x, i32 y, i32 r);
void      hitboxes_flush(g_s *g);
#endif