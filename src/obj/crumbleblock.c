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

static void crumbleblock_set_block(game_s *g, obj_s *o, int b)
{
    int tx = o->pos.x >> 4;
    int ty = o->pos.y >> 4;

    g->tiles[tx + ty * g->tiles_x].collision = b;
    if (b != TILE_EMPTY) {
        game_on_solid_appear(g);
    }
}

obj_s *crumbleblock_create(game_s *g)
{
    obj_s *o             = obj_create(g);
    o->ID                = OBJ_ID_CRUMBLEBLOCK;
    o->flags             = OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_TILESET_TERRAIN, 16, 0, 16, 16);
    o->state             = CRUMBLE_STATE_IDLE;
    return o;
}

void crumbleblock_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = crumbleblock_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->w     = mo->w;
    o->h     = mo->h;
    crumbleblock_set_block(g, o, TILE_BLOCK);
}

void crumbleblock_update(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];

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
        if (0 < o->timer) {
            spr->offs.x = rngr_sym_i32(1);
            spr->offs.y = rngr_sym_i32(1);
        } else { // break
            crumbleblock_set_block(g, o, TILE_EMPTY);
            o->flags &= ~OBJ_FLAG_SPRITE;
            o->state = CRUMBLE_STATE_RESPAWNING;
            o->timer = CRUMBLE_TICKS_RESPAWN;
        }
    } break;
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (0 < o->timer) break;

        o->state = CRUMBLE_STATE_IDLE;
        o->flags |= OBJ_FLAG_SPRITE;
        spr->offs.x = 0;
        spr->offs.y = 0;
        crumbleblock_set_block(g, o, TILE_BLOCK);
        game_on_solid_appear(g);
    } break;
    }
}