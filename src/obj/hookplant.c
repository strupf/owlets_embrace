// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HOOKPLANT_IDLE,
    HOOKPLANT_GRABBED,
    HOOKPLANT_HIDDEN,
};

void hookplant_on_hook(obj_s *o)
{
    o->state = 1;
}
void hookplant_on_update(game_s *g, obj_s *o);
void hookplant_on_animate(game_s *g, obj_s *o);

void hookplant_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HOOKPLANT;
    o->flags = OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SOLID;
    o->mass       = 1;
    o->on_update  = hookplant_on_update;
    o->on_animate = hookplant_on_animate;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
}

void hookplant_on_update(game_s *g, obj_s *o)
{
    switch (o->state) {
    case HOOKPLANT_IDLE: break;
    case HOOKPLANT_GRABBED: {
        o->flags |= OBJ_FLAG_MOVER;
        o->timer++;
        if (o->timer < 10) break;
        if (25 <= o->timer) {
            obj_s *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
            ohero->vel_q8 = v2_mulq(ohero->vel_q8, 250, 8);
            hero_unhook(g, ohero);
            obj_delete(g, o);
            break;
        }
        o->tomove.y = -(o->timer - 10);
        u32 ll      = g->hero_mem.rope.len_max_q4;
        rope_set_len_max_q4(&g->hero_mem.rope, ll / 2);
        break;
    }
    }
}

void hookplant_on_animate(game_s *g, obj_s *o)
{
}