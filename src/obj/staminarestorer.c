// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    STAMINARESTORER_ACTIVE,
    STAMINARESTORER_COLLECT,
    STAMINARESTORER_HIDDEN,
    STAMINARESTORER_RESPAWN
};

#define STAMINARESTORER_TICK_COLLECT 18
#define STAMINARESTORER_TICK_RESPAWN 10

void staminarestorer_on_animate(g_s *g, obj_s *o);

void staminarestorer_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->UUID       = mo->UUID;
    o->ID         = OBJID_STAMINARESTORER;
    o->on_animate = staminarestorer_on_animate;
    o->w          = 8;
    o->h          = 8;
    o->pos.x      = mo->x + (mo->w - o->w) / 2;
    o->pos.y      = mo->y + (mo->h - o->h) / 2;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
}

void staminarestorer_on_animate(g_s *g, obj_s *o)
{
    static const u8 frame_ticks[6] = {6, 4, 5, 6, 5, 4};

    obj_sprite_s *spr = &o->sprites[0];
    o->timer++;

    switch (o->state) {
    default: break;
    case STAMINARESTORER_COLLECT: {
        if (STAMINARESTORER_TICK_COLLECT <= o->timer) {
            o->state = STAMINARESTORER_HIDDEN;
            o->timer = 0;
        }
        break;
    }
    case STAMINARESTORER_RESPAWN: {
        if (STAMINARESTORER_TICK_RESPAWN <= o->timer) {
            o->state = STAMINARESTORER_ACTIVE;
            o->timer = 0;
        }
        break;
    }
    }

    switch (o->state) {
    default: break;
    case STAMINARESTORER_HIDDEN: {
        o->n_sprites = 0;
        break;
    }
    case STAMINARESTORER_ACTIVE: {
        o->n_sprites = 1;
        o->animation++;
        i32 fID = 0;
        for (i32 n = 0, t = 0; n < ARRLEN(frame_ticks); n++) {
            t += frame_ticks[n];
            if (o->animation < t) {
                fID = n;
                break;
            }
            if (n == ARRLEN(frame_ticks) - 1) {
                o->animation %= t;
            }
        }

        spr->trec   = asset_texrec(TEXID_STAMINARESTORE, 0, fID * 32, 32, 32);
        spr->offs.x = (o->w - 32) >> 1;
        spr->offs.y = (o->h - 32) >> 1;
        spr->offs.y += (3 * sin_q15(o->timer << 11)) >> 15;
        break;
    }
    case STAMINARESTORER_COLLECT: {
        o->n_sprites = 1;
        i32 fID      = lerp_i32(0, 7, o->timer, STAMINARESTORER_TICK_COLLECT);
        spr->trec    = asset_texrec(TEXID_PARTICLES, fID * 64, 128, 64, 64);
        spr->offs.x  = (o->w - 64) >> 1;
        spr->offs.y  = (o->h - 64) >> 1;
        break;
    }
    case STAMINARESTORER_RESPAWN: {
        o->n_sprites = 1;
        i32 fID      = lerp_i32(6, 0, o->timer, STAMINARESTORER_TICK_RESPAWN);
        spr->trec    = asset_texrec(TEXID_PARTICLES, fID * 64, 128, 64, 64);
        spr->offs.x  = (o->w - 64) >> 1;
        spr->offs.y  = (o->h - 64) >> 1;
        break;
    }
    }
}

bool32 staminarestorer_try_collect(g_s *g, obj_s *o, obj_s *ohero)
{
    if (o->state == STAMINARESTORER_ACTIVE ||
        o->state == STAMINARESTORER_RESPAWN) {
        o->state = STAMINARESTORER_COLLECT;
        o->timer = 0;
        // hero_stamina_add_ui(ohero, HERO_TICKS_PER_STAMINA_UPGRADE);
        return 1;
    }
    return 0;
}

void staminarestorer_respawn_all(g_s *g, obj_s *o)
{
    for (obj_each(g, i)) {
        if (i->ID != OBJID_STAMINARESTORER) continue;
        if (i->state != STAMINARESTORER_HIDDEN) continue;

        i->state = STAMINARESTORER_RESPAWN;
        i->timer = 0;
    }
}