// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BUDPLANT_STATIONARY,
    BUDPLANT_WALKING,
};

enum {
    BUDPLANT_ST_IDLE,
};

typedef struct {
    i32 x;
} budplant_s;

void budplant_on_update(game_s *g, obj_s *o);
void budplant_on_animate(game_s *g, obj_s *o);

void budplant_load(game_s *g, map_obj_s *mo)
{
    obj_s      *o  = obj_create(g);
    budplant_s *bp = (budplant_s *)o->mem;

    o->ID    = OBJ_ID_BUDPLANT;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB;
    o->w          = 16;
    o->h          = 16;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->on_update  = budplant_on_update;
    o->on_animate = budplant_on_animate;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
}

void budplant_on_update(game_s *g, obj_s *o)
{
    budplant_s *bp = (budplant_s *)o->mem;

    switch (o->state) {
    case BUDPLANT_ST_IDLE: {
        o->timer++;
        if (100 <= o->timer) {
            o->timer  = 0;
            v2_i32 pv = {rngr_sym_i32(400), -2000};
            obj_s *pr = projectile_create(g, o->pos, pv, 0);
        }
        break;
    }
    default: break;
    }
}

void budplant_on_animate(game_s *g, obj_s *o)
{
    budplant_s   *bp  = (budplant_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    i32 frameID = 0;
    i32 animID  = 0;

    switch (o->state) {
    case BUDPLANT_ST_IDLE: {

        break;
    }
    default: break;
    }
}