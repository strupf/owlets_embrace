// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    TOGGLEBLOCK_OFF,
    TOGGLEBLOCK_ON,
};

typedef struct {
    int trigger_to_enable;
    int trigger_to_disable;
} obj_toggleblock_s;

obj_s *toggleblock_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_TOGGLEBLOCK;
    o->w     = 16;
    o->h     = 16;
    o->state = TOGGLEBLOCK_OFF;
    return o;
}

obj_s *toggleblock_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = toggleblock_create(g);

    obj_toggleblock_s *ot  = (obj_toggleblock_s *)o->mem;
    ot->trigger_to_enable  = map_obj_i32(mo, "Trigger_enable");
    ot->trigger_to_disable = map_obj_i32(mo, "Trigger_disable");

    return o;
}

void toggleblock_on_animate(game_s *g, obj_s *o)
{
    o->timer++;
}

void toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
#define TOGGLEBLOCK_TICKS 6

    gfx_ctx_s ctxa = gfx_ctx_display();
    gfx_ctx_s ctxb = ctxa;
    int       i    = min_i(o->timer, TOGGLEBLOCK_TICKS);
    switch (o->state) {
    case TOGGLEBLOCK_OFF:
        ctxb.pat = gfx_pattern_interpolate(TOGGLEBLOCK_TICKS - i, TOGGLEBLOCK_TICKS);
        break;
    case TOGGLEBLOCK_ON:
        ctxb.pat = gfx_pattern_interpolate(i, TOGGLEBLOCK_TICKS);
        break;
    }

    tex_s    tex = asset_tex(TEXID_TOGGLEBLOCK);
    texrec_s tr0 = {tex, {16, 16, 16, 16}};
    texrec_s tr1 = {tex, {48, 16, 16, 16}};
    v2_i32   pos = v2_add(o->pos, cam);

    gfx_spr(ctxa, tr1, pos, 0, 0);
    gfx_spr(ctxb, tr0, pos, 0, 0);
}

void toggleblock_on_trigger(game_s *g, obj_s *o, int trigger)
{
    int tx = o->pos.x >> 4;
    int ty = o->pos.y >> 4;

    obj_toggleblock_s *ot = (obj_toggleblock_s *)o->mem;

    switch (o->state) {
    case TOGGLEBLOCK_OFF:
        if (trigger != ot->trigger_to_enable) return;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_BLOCK;
        game_on_solid_appear(g);
        break;
    case TOGGLEBLOCK_ON:
        if (trigger != ot->trigger_to_disable) return;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_EMPTY;
        break;
    }

    o->state = 1 - o->state;
    o->timer = 0;
}