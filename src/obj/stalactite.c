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

void stalactite_on_update(game_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];

    o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
    o->flags &= ~OBJ_FLAG_MOVER;

    switch (o->state) {
    case STALACTITE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;

        stalactite_s *s = (stalactite_s *)o->mem;
        if (overlap_rec(s->checkr, obj_aabb(ohero))) {
            o->state = STALACTITE_SHAKING;
        }
        break;
    }
    case STALACTITE_SHAKING: {
        spr->offs.x = rngr_i32(-1, +1);
        spr->offs.y = rngr_i32(-1, +1);
        o->timer++;
        if (o->timer < 30) break;
        o->state = STALACTITE_FALLING;
        o->flags |= OBJ_FLAG_MOVER;
        spr->offs.x = 0;
        spr->offs.y = 0;
        break;
    }
    case STALACTITE_FALLING: {
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        o->flags |= OBJ_FLAG_MOVER;
        if (obj_grounded(g, o)) {
            o->state = STALACTITE_STUCK;
            o->flags &= ~OBJ_FLAG_MOVER;
            if (1000 < o->vel_q8.y) {
                cam_screenshake(&g->cam, 10, 3);
            }

            for (i32 yy = -1; yy <= +1; yy += 2) {
                for (i32 xx = -1; xx <= +1; xx += 2) {
                    v2_i32 vel = {xx * 200, yy * 200};
                    projectile_create(g,
                                      obj_pos_center(o),
                                      vel,
                                      PROJECTILE_ID_STALACTITE_BREAK);
                }
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

void stalactite_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_STALACTITE;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->on_update       = stalactite_on_update;
    obj_sprite_s *spr  = &o->sprites[0];
    o->render_priority = -1;
    o->moverflags      = OBJ_MOVER_ONE_WAY_PLAT;
    o->n_sprites       = 1;
    o->w               = 32;
    o->h               = 16;
    o->gravity_q8.y    = 70;
    o->drag_q8.y       = 255;
    spr->trec          = asset_texrec(TEXID_MISCOBJ, 224, 0, 32, 32);
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;

    stalactite_s *s = (stalactite_s *)o->mem;

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
