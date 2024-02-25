// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    OBJKEY_ST_NONE,
    OBJKEY_ST_COLLECTED,
};

typedef struct {
    int x;
} objkey_s;

void objkey_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_KEY;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_RENDER_AABB;
    objkey_s *k = (objkey_s *)o->mem;

    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->w            = mo->w;
    o->h            = mo->h;
    o->gravity_q8.y = 70;
    o->drag_q8.x    = 250;
    o->drag_q8.y    = 254;
}

void objkey_on_update(game_s *g, obj_s *o)
{
    objkey_s *k     = (objkey_s *)o->mem;
    obj_s    *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    switch (o->state) {
    case OBJKEY_ST_NONE: {
        if (!ohero) break;
        if (!overlap_rec(obj_aabb(o), obj_aabb(ohero))) break;
        // collected
        o->state = OBJKEY_ST_COLLECTED;
        o->flags &= ~OBJ_FLAG_ACTOR;
        o->flags &= ~OBJ_FLAG_MOVER;
        o->vel_q8.x = 0;
        o->vel_q8.y = 0;
        break;
    }
    case OBJKEY_ST_COLLECTED: {
        if (!ohero) {
            // drop
            o->state = OBJKEY_ST_NONE;
            o->flags |= OBJ_FLAG_ACTOR;
            o->flags |= OBJ_FLAG_MOVER;
            break;
        }
        break;
    }
    }
}

void objkey_on_animate(game_s *g, obj_s *o)
{
}

void objkey_on_invalid_pos(game_s *g, obj_s *o)
{
}