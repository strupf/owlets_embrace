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

#define CRUMBLE_X_OFFS -4

static void crumbleblock_set_block(game_s *g, obj_s *o, int b)
{
    int tx = o->pos.x >> 4;
    int ty = o->pos.y >> 4;
    int n  = o->w >> 4;
    for (int i = 0; i < n; i++) {
        g->tiles[tx + i + ty * g->tiles_x].collision = b;
    }

    if (b == TILE_BLOCK) {
        game_on_solid_appear(g);
    }
}

obj_s *crumbleblock_create(game_s *g)
{
    obj_s *o             = obj_create(g);
    o->ID                = OBJ_ID_CRUMBLEBLOCK;
    o->flags             = OBJ_FLAG_SPRITE;
    o->render_priority   = 0;
    o->n_sprites         = 1;
    o->w                 = 16;
    sprite_simple_s *spr = &o->sprites[0];
    spr->offs.x          = CRUMBLE_X_OFFS;
    o->state             = CRUMBLE_STATE_IDLE;
    o->substate          = TILE_BLOCK;
    return o;
}

void crumbleblock_load(game_s *g, map_obj_s *mo)
{
    obj_s *o             = crumbleblock_create(g);
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
    o->w                 = mo->w;
    o->h                 = mo->h;
    int n                = o->w >> 4;
    o->substate          = map_obj_bool(mo, "Platform") ? TILE_ONE_WAY : TILE_BLOCK;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_CRUMBLE, 0, (n - 1) * 16, 8 + n * 16, 16);

    crumbleblock_set_block(g, o, o->substate);
}

void crumbleblock_on_update(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];

    switch (o->state) {
    case CRUMBLE_STATE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;
        rec_i32 r = {o->pos.x, o->pos.y, o->w, 1};
        if (0 <= ohero->vel_q8.y && overlap_rec(obj_rec_bottom(ohero), r)) {
            o->state = CRUMBLE_STATE_BREAKING;
            o->timer = CRUMBLE_TICKS_BREAK;
        }
        break;
    }
    case CRUMBLE_STATE_BREAKING: {
        o->timer--;
        if (0 < o->timer) {
            spr->offs.x = rngr_sym_i32(1) + CRUMBLE_X_OFFS;
            spr->offs.y = rngr_sym_i32(1);
        } else { // break
            crumbleblock_set_block(g, o, TILE_EMPTY);
            o->flags &= ~OBJ_FLAG_SPRITE;
            o->state = CRUMBLE_STATE_RESPAWNING;
            o->timer = CRUMBLE_TICKS_RESPAWN;
        }
        break;
    }
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (0 < o->timer) break;

        o->state = CRUMBLE_STATE_IDLE;
        o->flags |= OBJ_FLAG_SPRITE;
        spr->offs.x = CRUMBLE_X_OFFS;
        spr->offs.y = 0;
        crumbleblock_set_block(g, o, o->substate);
        game_on_solid_appear(g);
        break;
    }
    }
}