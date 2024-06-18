// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 pos_og;
    v2_i32 pos_activated;
} hooklever_s;

#define HOOKLEVER_TICKS 30

void hooklever_on_update(game_s *g, obj_s *o)
{
    const i32 subt = o->subtimer;

    switch (o->state) {
    case 0: {
        obj_s *ohook = obj_get_tagged(g, OBJ_TAG_HOOK);

        if (!ohook) break;
        rec_i32 rhook = obj_aabb(ohook);
        rhook.x--;
        rhook.y--;
        rhook.w += 2;
        rhook.h += 2;
        if (!overlap_rec(rhook, obj_aabb(o))) break;

        rope_s *r = ohook->rope;
        if (!r) break;

        if (rope_stretch_q8(g, r) <= 256) break;

        o->subtimer++;
        if (20 <= o->subtimer) {
            o->subtimer = 0;
            o->state    = 1;
            o->timer    = 0;
            o->tomove.y = 16;
            saveID_put(g, o->save_ID);
        }
        break;
    }
    case 1: {
        o->timer++;
        if (o->timer == HOOKLEVER_TICKS) {
            game_on_trigger(g, o->trigger);
        }
        break;
    }
    }

    if (subt == o->subtimer) {
        o->subtimer = 0;
    }

    obj_sprite_s *spr = &o->sprites[0];

    switch (o->state) {
    case 0: {
        break;
    }
    case 1: {
        i32 dy = (30 * min_i(o->timer, HOOKLEVER_TICKS)) / HOOKLEVER_TICKS;
        break;
    }
    }
}

void hooklever_load(game_s *g, map_obj_s *mo)
{
    obj_s *o   = obj_create(g);
    o->ID      = OBJ_ID_HOOKLEVER;
    o->save_ID = mo->ID;
    o->flags   = OBJ_FLAG_HOOKABLE |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;
    o->on_update    = hooklever_on_update;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    hooklever_s *hl = (hooklever_s *)o->mem;
    hl->pos_og      = o->pos;
    o->w            = 32;
    o->h            = 32;
    o->trigger      = map_obj_i32(mo, "trigger");
    o->state        = 0;
    o->mass         = 2;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;

    if (saveID_has(g, o->save_ID)) {
        o->pos.y += 16;
        o->state = 1;
    }
}
