// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define TOGGLEBLOCK_TR_ENABLE  0
#define TOGGLEBLOCK_TR_DISABLE 1

typedef struct {
    i32 trigger[2];
} obj_toggleblock_s;

void toggleblock_on_animate(g_s *g, obj_s *o)
{
}

void toggleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    if (o->state == 0) return;

    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = asset_texrec(TEXID_TOGGLE, 0, 0, 16, 16);
    v2_i32    pos = v2_i32_add(o->pos, cam);
    i32       nx  = o->w >> 4;
    i32       ny  = o->h >> 4;

    render_tile_terrain_block(ctx, pos, nx, ny, TILE_TYPE_CRUMBLE);
}

static void toggleblock_set_state(g_s *g, obj_s *o, i32 state)
{
    o->state = state;
    i32 b    = state == 1 ? TILE_BLOCK : TILE_EMPTY;
    // i32 t    = b == TILE_BLOCK ? TILE_TYPE_DIRT : 0;
    i32 t    = 0;
    tile_map_set_collision(g, obj_aabb(o), b, t);
}

void toggleblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    obj_toggleblock_s *ot = (obj_toggleblock_s *)o->mem;

    if (trigger == ot->trigger[o->state]) {
        toggleblock_set_state(g, o, 1 - o->state);
    }
}

void toggleblock_load(g_s *g, map_obj_s *mo)
{
    obj_s             *o  = obj_create(g);
    obj_toggleblock_s *ot = (obj_toggleblock_s *)o->mem;
    o->ID                 = OBJID_TOGGLEBLOCK;
    o->render_priority    = 0;
    o->state              = 0;
    o->pos.x              = mo->x;
    o->pos.y              = mo->y;
    o->w                  = mo->w;
    o->h                  = mo->h;

    ot->trigger[TOGGLEBLOCK_TR_ENABLE]  = map_obj_i32(mo, "Trigger_enable");
    ot->trigger[TOGGLEBLOCK_TR_DISABLE] = map_obj_i32(mo, "Trigger_disable");
    toggleblock_set_state(g, o, map_obj_bool(mo, "Enabled"));
}
