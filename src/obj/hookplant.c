// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HOOKPLANT_IDLE,
    HOOKPLANT_GRABBED,
    HOOKPLANT_HIDDEN,
};

#define HOOKPLANT_TICKS_RESPAWN 100

typedef struct {
    v2_i32 p;
} hookplant_s;

void hookplant_on_update(g_s *g, obj_s *o);
void hookplant_on_animate(g_s *g, obj_s *o);

void hookplant_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HOOKPLANT;
    o->flags = OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HOOKABLE;
    o->on_update  = hookplant_on_update;
    o->on_animate = hookplant_on_animate;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w + 16;
    o->h          = mo->h + 16;
}

void hookplant_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case HOOKPLANT_IDLE: break;
    case HOOKPLANT_GRABBED: {
        obj_s *ohero = NULL;
        if (!hero_present_and_alive(g, &ohero) || !ohero->rope) {
            o->state = HOOKPLANT_HIDDEN;
            o->timer = 0;
            hero_action_ungrapple(g, ohero);
            break;
        }

        o->timer++;
        if (o->timer < 10) {
            g->rope.len_max_q4 = (g->rope.len_max_q4 * 260) >> 8;
        }

        if (o->timer < 15) {
            g->rope.len_max_q4 /= 2;
            g->rope.len_max_q4 = max_i32(g->rope.len_max_q4, 16);
        } else {
            hero_action_ungrapple(g, ohero);
            o->state = HOOKPLANT_HIDDEN;
            o->timer = 0;
        }

        break;
    }
    case HOOKPLANT_HIDDEN:
        o->timer++;
        if (HOOKPLANT_TICKS_RESPAWN <= o->timer) {
            o->state = HOOKPLANT_IDLE;
            o->timer = 0;
        }
        break;
    }
}

void hookplant_on_animate(g_s *g, obj_s *o)
{
    o->flags |= OBJ_FLAG_RENDER_AABB;
    switch (o->state) {
    case HOOKPLANT_IDLE: break;
    case HOOKPLANT_GRABBED: break;
    case HOOKPLANT_HIDDEN:
        o->flags &= ~OBJ_FLAG_RENDER_AABB;
        break;
    }
}

void hookplant_on_hook(obj_s *o)
{
    hookplant_s *pl = (hookplant_s *)o->mem;
    o->state        = HOOKPLANT_GRABBED;
    o->timer        = 0;
    ropenode_s *rn  = ropenode_neighbour(o->rope, o->ropenode);
    pl->p           = v2_sub(o->ropenode->p, rn->p);
}