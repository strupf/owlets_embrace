// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define FLOATER_BUMP_TIMER 20

void floater_on_update(game_s *g, obj_s *o)
{
    o->timer++;
    if (o->subtimer) {
        o->subtimer++;
        if (FLOATER_BUMP_TIMER <= o->subtimer) {
            o->subtimer = 0;
            o->substate = 0;
        }
    }

    if (o->bumpflags) {
        o->subtimer = 1;
        if (o->bumpflags & OBJ_BUMPED_X) {
            if (o->bumpflags & OBJ_BUMPED_X_POS) {
                o->substate = OBJ_BUMPED_X_POS;
            }
            if (o->bumpflags & OBJ_BUMPED_X_NEG) {
                o->substate = OBJ_BUMPED_X_NEG;
            }
            o->vel_q8.x = -o->vel_q8.x;
        }
        if (o->bumpflags & OBJ_BUMPED_Y) {
            if (o->bumpflags & OBJ_BUMPED_Y_POS) {
                o->substate = OBJ_BUMPED_Y_POS;
            }
            if (o->bumpflags & OBJ_BUMPED_Y_NEG) {
                o->substate = OBJ_BUMPED_Y_NEG;
            }
            o->vel_q8.y = -o->vel_q8.y;
        }
    }

    o->bumpflags = 0;
}

void floater_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    switch (o->substate) {
    case OBJ_BUMPED_X_POS: {
        break;
    }
    case OBJ_BUMPED_X_NEG: {
        break;
    }
    case OBJ_BUMPED_Y_POS: {
        break;
    }
    case OBJ_BUMPED_Y_NEG: {
        break;
    }
    default: break;
    }
}

void floater_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_FLOATER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->on_update       = floater_on_update;
    o->on_animate      = floater_on_animate;
    o->w               = 24;
    o->h               = 24;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->health_max      = 1;
    o->health          = o->health_max;
    o->enemy           = enemy_default();
    o->drag_q8.y       = 256;
    o->drag_q8.x       = 256;
    o->render_priority = 1;
    o->n_sprites       = 1;

    i32 d       = map_obj_i32(mo, "dir");
    o->vel_q8.x = 128 * ((d == 0 || d == 2) ? -1 : +1);
    o->vel_q8.y = 128 * ((d == 0 || d == 1) ? -1 : +1);
}
