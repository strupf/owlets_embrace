// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    STAMINARESTORER_ACTIVE,
    STAMINARESTORER_COLLECT,
    STAMINARESTORER_HIDDEN,
    STAMINARESTORER_RESPAWN
};

#define STAMINARESTORER_TICK_COLLECT 20
#define STAMINARESTORER_TICK_RESPAWN 20

void staminarestorer_on_animate(game_s *g, obj_s *o);

void staminarestorer_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_STAMINARESTORER;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB;
    o->w          = 16;
    o->h          = 16;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->on_animate = staminarestorer_on_animate;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
}

void staminarestorer_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];

    u32 animID  = 0;
    u32 frameID = 0;
    o->flags |= OBJ_FLAG_RENDER_AABB;
    o->n_sprites = 1;
    o->timer++;
    switch (o->state) {
    case STAMINARESTORER_HIDDEN:
        o->flags &= ~OBJ_FLAG_RENDER_AABB;
        o->n_sprites = 0;
        return;
    case STAMINARESTORER_COLLECT:
        o->flags &= ~OBJ_FLAG_RENDER_AABB;
        if (STAMINARESTORER_TICK_COLLECT <= o->timer) {
            o->state = STAMINARESTORER_HIDDEN;
            o->timer = 0;
        }
        break;
    case STAMINARESTORER_RESPAWN:
        if (STAMINARESTORER_TICK_RESPAWN <= o->timer) {
            o->state = STAMINARESTORER_ACTIVE;
            o->timer = 0;
        }
        break;
    default: break;
    }
}

void staminarestorer_try_collect_any(game_s *g, obj_s *ohero)
{
    hero_s *h     = &g->hero_mem;
    rec_i32 haabb = obj_aabb(ohero);

    for (obj_each(g, o)) {
        if (o->ID != OBJ_ID_STAMINARESTORER) continue;
        if (o->state != STAMINARESTORER_ACTIVE &&
            o->state != STAMINARESTORER_RESPAWN) continue;

        if (overlap_rec(haabb, obj_aabb(o))) {
            o->state = STAMINARESTORER_COLLECT;
            o->timer = 1;
            hero_flytime_add_ui(g, ohero, HERO_TICKS_PER_JUMP_UPGRADE);
        }
    }
}

void staminarestorer_respawn_all(game_s *g, obj_s *o)
{
    for (obj_each(g, o)) {
        if (o->ID != OBJ_ID_STAMINARESTORER) continue;
        if (o->state != STAMINARESTORER_HIDDEN) continue;

        o->state = STAMINARESTORER_RESPAWN;
        o->timer = 1;
    }
}