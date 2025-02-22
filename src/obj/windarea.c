// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 x;
} windarea_s;

void windarea_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o = obj_create(g);
    windarea_s *w = (windarea_s *)o->mem;
    o->ID         = OBJID_WINDAREA;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
}

void windarea_on_animate(g_s *g, obj_s *o)
{
    windarea_s *w = (windarea_s *)o->mem;
}

void windarea_on_update(g_s *g, obj_s *o)
{
    obj_s *oh = obj_get_hero(g);
    if (!oh) return;
    hero_s *h = (hero_s *)oh->heap;

    if (!overlap_rec(obj_aabb(oh), obj_aabb(o))) return;

    if (64 < oh->v_q8.y) {
        obj_vy_q8_mul(oh, 240);
    }

    i32 scl = min_i32(oh->pos.y - o->pos.y + oh->h, 32);
    oh->v_q8.y -= lerp_i32(0, 140, scl, 32);

    oh->v_q8.y = max_i32(oh->v_q8.y, -256 * 6);
    h->gliding = min_i32(h->gliding + 2, 16);
    hero_stamina_modify(oh, 32);
}

void windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    windarea_s *w = (windarea_s *)o->mem;
}
