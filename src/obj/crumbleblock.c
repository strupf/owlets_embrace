// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#if 1
enum {
    CRUMBLE_STATE_IDLE,
    CRUMBLE_STATE_BREAKING,
    CRUMBLE_STATE_RESPAWNING,
};

#define CRUMBLE_TICKS_BREAK   100
#define CRUMBLE_TICKS_RESPAWN 100

obj_s *crumbleblock_create(game_s *g)
{
    obj_s *o = (obj_s *)obj_create(g);
    o->ID    = OBJ_ID_CRUMBLEBLOCK;
    o->flags |= OBJ_FLAG_SOLID;
    o->flags |= OBJ_FLAG_SPRITE;

    o->sprites[0].trec = asset_texrec(TEXID_TILESET,
                                      16, 0, 16, 16);
    o->n_sprites       = 1;
    o->w               = 16;
    o->h               = 16;
    o->state           = CRUMBLE_STATE_IDLE;
    return o;
}

void crumbleblock_update(game_s *g, obj_s *o)
{
    switch (o->state) {
    case CRUMBLE_STATE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;

        if (overlap_rec(obj_rec_bottom(ohero), obj_aabb(o))) {
            o->state = CRUMBLE_STATE_BREAKING;
            o->timer = CRUMBLE_TICKS_BREAK;
        }

    } break;
    case CRUMBLE_STATE_BREAKING: {
        o->timer--;
        if (o->timer > 0) {
            o->sprites[0].offs.x = rngr_i32(-1, +1);
            o->sprites[0].offs.y = rngr_i32(-1, +1);
        } else { // break
            o->flags &= ~OBJ_FLAG_SOLID;
            o->flags &= ~OBJ_FLAG_SPRITE;
            o->state = CRUMBLE_STATE_RESPAWNING;
            o->timer = CRUMBLE_TICKS_RESPAWN;
        }
    } break;
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (o->timer > 0) break;

        o->state = CRUMBLE_STATE_IDLE;
        o->flags |= OBJ_FLAG_SOLID;
        o->flags |= OBJ_FLAG_SPRITE;
        o->sprites[0].offs.x = 0;
        o->sprites[0].offs.y = 0;

        rec_i32 aabb = obj_aabb(o);
        for (int i = 0; i < g->obj_nbusy; i++) {
            obj_s *obj = g->obj_busy[i];
            if (obj == o) continue;
            if (!(obj->flags & OBJ_FLAG_ACTOR)) continue;

            if (overlap_rec(aabb, obj_aabb(obj))) {
                actor_try_wiggle(g, obj);
            }
        }
    } break;
    }
}
#endif