// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    FISH_ST_IDLE,
    FISH_ST_AGGRESSIVE,
    FISH_ST_JUMPING,
    FISH_ST_OUT_OF_WATER,
};

typedef struct {
    i32 x;
} fish_s;

bool32 fish_in_water(g_s *g, obj_s *o);
void   fish_on_update(g_s *g, obj_s *o);
void   fish_on_animate(g_s *g, obj_s *o);

void fish_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_FISH;
    o->on_update  = fish_on_update;
    o->on_animate = fish_on_animate;
    o->flags      = OBJ_FLAG_ACTOR |
               OBJ_FLAG_RENDER_AABB;
    o->w     = 16;
    o->h     = 16;
    o->pos.x = mo->x + (mo->w - o->w) / 2;
    o->pos.y = mo->y + (mo->h - o->h) / 2;
}

void fish_on_update(g_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_hero(g);

    o->v_q8.y += 60;
    bool32 inwater = fish_in_water(g, o);

    switch (o->state) {
    case FISH_ST_IDLE: {
        break;
    }
    case FISH_ST_JUMPING: {
        if (inwater) {
            o->state = FISH_ST_IDLE;
        }
        break;
    }
    case FISH_ST_AGGRESSIVE: {
        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void fish_on_animate(g_s *g, obj_s *o)
{
    bool32 inwater = fish_in_water(g, o);
}

bool32 fish_in_water(g_s *g, obj_s *o)
{
    return (o->h == water_depth_rec(g, obj_aabb(o)));
}