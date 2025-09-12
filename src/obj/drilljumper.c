// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void drilljumper_on_update(g_s *g, obj_s *o);
void drilljumper_on_animate(g_s *g, obj_s *o);
void drilljumper_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void drilljumper_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->UUID       = mo->UUID;
    o->ID         = OBJID_DRILLJUMPER;
    o->w          = 16;
    o->h          = 16;
    o->on_update  = drilljumper_on_update;
    o->on_animate = drilljumper_on_animate;
    o->on_draw    = drilljumper_on_draw;
}

void drilljumper_on_update(g_s *g, obj_s *o)
{
}

void drilljumper_on_animate(g_s *g, obj_s *o)
{
}

void drilljumper_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);
    gfx_rec_fill(ctx, translate_rec(obj_aabb(o), cam.x, cam.y), PRIM_MODE_BLACK);
}