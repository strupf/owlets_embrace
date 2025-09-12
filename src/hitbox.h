// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

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

typedef struct hitbox_s hitbox_s;
typedef void (*hitbox_on_hit_f)(g_s *g, hitbox_s *hb, void *ctx);

struct hitbox_s {
    ALIGNAS(32)
    //
    u32             UID; // unique ID, incremented per hitbox
    u8              ID;
    i8              misc;  // custom data, hitbox specific
    i8              dx;    // custom data, hitbox specific
    i8              dy;    // custom data, hitbox specific
    i16             pos_x; // top left in case of rec, center in case of cir
    i16             pos_y;
    u16             cir_r; // if nonzero: circle, else rectangle
    u16             rec_w;
    u16             rec_h;
    hitbox_on_hit_f cb;  // void f(g_s *g, hitbox_s *hb, void *ctx);
    void           *ctx; // context pointer for callback method
};

u32       hitbox_UID_gen(g_s *g);
hitbox_s *hitbox_new(g_s *g);
void      hitbox_set_callback(hitbox_s *h, hitbox_on_hit_f f, void *ctx); // void f(g_s *g, hitbox_s *hb, void *ctx);
void      hitbox_set_rec(hitbox_s *h, rec_i32 r);
bool32    hitbox_from_owl(hitbox_s *h);
bool32    hitbox_from_enemy(hitbox_s *h);
bool32    hitbox_hits_obj(hitbox_s *h, obj_s *o);
bool32    hitbox_try_apply_to_enemies(g_s *g, hitbox_s *hboxes, i32 n_hb);

#endif