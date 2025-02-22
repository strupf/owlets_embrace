// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    STALACTITE_IDLE,
    STALACTITE_SHAKING,
    STALACTITE_FALLING,
};

#define STALACTITE_TICKS_SHAKE 30
#define STALACTITE_W           24
#define STALACTITE_H           16
#define STALACTITE_SPR_OFFX    -8
#define STALACTITE_SPR_OFFY    -8

typedef struct {
    i32     og_x;
    i32     og_y;
    rec_i32 checkr;
} stalactite_s;

void stalactite_load(g_s *g, map_obj_s *mo)
{
    obj_s *o_spawn          = obj_create(g);
    o_spawn->ID             = OBJID_STALACTITE_SPAWN;
    o_spawn->w              = STALACTITE_W;
    o_spawn->h              = STALACTITE_H;
    o_spawn->pos.x          = mo->x + (mo->w - STALACTITE_W) / 2;
    o_spawn->pos.y          = mo->y;
    o_spawn->n_sprites      = 1;
    obj_sprite_s *spr_spawn = &o_spawn->sprites[0];
    spr_spawn->trec         = asset_texrec(TEXID_STALACTITE, 0, 0, 32, 32);
    spr_spawn->offs.x       = STALACTITE_SPR_OFFX;
    spr_spawn->offs.y       = STALACTITE_SPR_OFFY;

    obj_s        *o = obj_create(g);
    stalactite_s *s = (stalactite_s *)o->mem;
    o->ID           = OBJID_STALACTITE;
    o->flags        = OBJ_FLAG_KILL_OFFSCREEN;
    o->w            = STALACTITE_W;
    o->h            = STALACTITE_H;
    o->pos.x        = o_spawn->pos.x;
    o->pos.y        = o_spawn->pos.y;
    o->moverflags =
        OBJ_MOVER_ONE_WAY_PLAT |
        OBJ_MOVER_TERRAIN_COLLISIONS;
    o->n_sprites = 1;

    i32 tx      = o->pos.x >> 4;
    s->checkr.x = o->pos.x;
    s->checkr.y = o->pos.y;
    s->checkr.w = o->w;
    s->checkr.h = 16;
    for (i32 ty = (o->pos.y >> 4) + 1; ty < g->tiles_y; ty++) {
        i32 t = g->tiles[tx + ty * g->tiles_x].collision;
        if (TILE_IS_SHAPE(t)) break;
        s->checkr.h += 16;
    }
}

void stalactite_on_update(g_s *g, obj_s *o)
{
    stalactite_s *s = (stalactite_s *)o->mem;

    if (o->bumpflags) {
        stalactite_burst(g, o);
        return;
    }

    switch (o->state) {
    case STALACTITE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (ohero && overlap_rec(s->checkr, obj_aabb(ohero))) {
            o->state = STALACTITE_SHAKING;
            o->timer = 0;
        }
        break;
    }
    case STALACTITE_SHAKING: {
        o->timer++;
        if (STALACTITE_TICKS_SHAKE <= o->timer) {
            o->state = STALACTITE_FALLING;
            o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
            o->timer = 0;
        }
        break;
    }
    case STALACTITE_FALLING: {
        o->timer++;
        o->v_q8.y = min_i32(o->v_q8.y + 70, 256 * 5);
        obj_move_by_v_q8(g, o);
        break;
    }
    }
}

void stalactite_on_animate(g_s *g, obj_s *o)
{
    i32           fr_x = 0;
    i32           fr_y = 0;
    obj_sprite_s *spr  = &o->sprites[0];
    o->n_sprites       = 1;
    spr->offs.x        = STALACTITE_SPR_OFFX;
    spr->offs.y        = STALACTITE_SPR_OFFY;

    switch (o->state) {
    case STALACTITE_IDLE: {
        fr_x = 1;
        break;
    }
    case STALACTITE_SHAKING: {
        i32 i1 = STALACTITE_TICKS_SHAKE / 2;
        i32 i0 = min_i32(o->timer, i1);
        fr_x   = lerp_i32(1, 3, i0, i1);
        spr->offs.x += rngr_i32(-1, +1);
        break;
    }
    case STALACTITE_FALLING: {
        i32 a_add = min_i32(o->timer >> 3, 4);
        o->animation += a_add;
        fr_y = 1;
        fr_x = (o->animation >> 3) & 3;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_STALACTITE, fr_x * 32, fr_y * 32, 32, 32);
}

void stalactite_burst(g_s *g, obj_s *o)
{
    obj_delete(g, o);
}