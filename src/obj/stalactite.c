// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    STALACTITE_IDLE,
    STALACTITE_SHAKING,
    STALACTITE_FALLING,
    STALACTITE_STUCK,
};

typedef struct {
    rec_i32 checkr;
} stalactite_s;

void stalactite_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_STALACTITE;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_ACTOR_PLATFORM |
               OBJ_FLAG_KILL_OFFSCREEN;
    sprite_simple_s *spr = &o->sprites[0];
    o->render_priority   = -1;
    o->moverflags        = OBJ_MOVER_ONE_WAY_PLAT;
    o->n_sprites         = 1;
    o->w                 = 32;
    o->h                 = 16;
    o->gravity_q8.y      = 30;
    o->drag_q8.y         = 254;
    spr->trec            = asset_texrec(TEXID_MISCOBJ, 224, 0, 32, 32);
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;

    stalactite_s *s = (stalactite_s *)o->mem;

    int tx      = o->pos.x >> 4;
    s->checkr.x = o->pos.x;
    s->checkr.y = o->pos.y;
    s->checkr.w = o->w;
    s->checkr.h = 16;
    for (int ty = (o->pos.y >> 4) + 1; ty < g->tiles_y; ty++) {
        int t = g->tiles[tx + ty * g->tiles_x].collision;
        if ((TILE_BLOCK <= t && t < NUM_TILE_BLOCKS)) break;
        s->checkr.h += 16;
    }
}

void stalactite_on_update(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];

    o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
    o->flags &= ~OBJ_FLAG_MOVER;

    switch (o->state) {
    case STALACTITE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;

        stalactite_s *s = (stalactite_s *)o->mem;
        if (overlap_rec(s->checkr, obj_aabb(ohero))) {
            o->state = STALACTITE_SHAKING;
            snd_play_ext(SNDID_CRUMBLE, 1.f, 2.f);
        }
        break;
    }
    case STALACTITE_SHAKING: {
        spr->offs.x = rngr_i32(-1, +1);
        spr->offs.y = rngr_i32(-1, +1);
        o->timer++;
        if ((o->timer & 15) == 0) {
            snd_play_ext(SNDID_STEP, 1.f, 1.5f);
        }
        if (o->timer < 30) break;
        o->state = STALACTITE_FALLING;
        o->flags |= OBJ_FLAG_MOVER;
        spr->offs.x = 0;
        spr->offs.y = 0;
        snd_play_ext(SNDID_SWITCH, 1.f, 0.7f);
        break;
    }
    case STALACTITE_FALLING: {
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        o->flags |= OBJ_FLAG_MOVER;
        if (obj_grounded(g, o)) {
            snd_play_ext(SNDID_STEP, 1.f, 1.f);
            o->state = STALACTITE_STUCK;
            o->flags &= ~OBJ_FLAG_MOVER;
            if (1000 < o->vel_q8.y) {
                cam_screenshake(&g->cam, 10, 3);
            }
            o->vel_q8.y = 0;
        }
        if (o->bumpflags & OBJ_BUMPED_X) {
            o->vel_q8.x = 0;
        }
        if (o->bumpflags & OBJ_BUMPED_Y) {
            o->vel_q8.y = 0;
        }
        o->bumpflags = 0;
        break;
    }
    case STALACTITE_STUCK: {
        if (obj_grounded(g, o)) break;
        o->state = STALACTITE_FALLING;

        break;
    }
    }
}