// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CRUMBLE_STATE_IDLE,
    CRUMBLE_STATE_BREAKING,
    CRUMBLE_STATE_RESPAWNING,
};

#define CRUMBLE_TICKS_BREAK   100
#define CRUMBLE_TICKS_RESPAWN 100

obj_s *crumbleblock_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CRUMBLEBLOCK;
    o->flags |= OBJ_FLAG_SPRITE;

    o->sprites[0].trec = asset_texrec(TEXID_TILESET_TERRAIN,
                                      16, 0, 16, 16);
    o->n_sprites       = 1;
    o->state           = CRUMBLE_STATE_IDLE;

    int tx                                   = o->pos.x >> 4;
    int ty                                   = o->pos.y >> 4;
    g->tiles[tx + ty * g->tiles_x].collision = TILE_BLOCK;
    return o;
}

void crumbleblock_update(game_s *g, obj_s *o)
{
    int tx = o->pos.x >> 4;
    int ty = o->pos.y >> 4;

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
            g->tiles[tx + ty * g->tiles_x].collision = TILE_EMPTY;
            o->flags &= ~OBJ_FLAG_SPRITE;
            o->state = CRUMBLE_STATE_RESPAWNING;
            o->timer = CRUMBLE_TICKS_RESPAWN;
        }
    } break;
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (o->timer > 0) break;

        o->state = CRUMBLE_STATE_IDLE;
        o->flags |= OBJ_FLAG_SPRITE;
        o->sprites[0].offs.x = 0;
        o->sprites[0].offs.y = 0;

        g->tiles[tx + ty * g->tiles_x].collision = TILE_BLOCK;
        game_on_solid_appear(g);
    } break;
    }
}