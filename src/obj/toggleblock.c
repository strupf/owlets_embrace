// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    TOGGLEBLOCK_OFF,
    TOGGLEBLOCK_ON,
};

obj_s *toggleblock_create(game_s *g)
{
    obj_s *o   = obj_create(g);
    o->ID      = OBJ_ID_TOGGLEBLOCK;
    o->w       = 16;
    o->h       = 16;
    o->state   = TOGGLEBLOCK_OFF;
    o->trigger = o->trigger_on_0;
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

void toggleblock_on_trigger(game_s *g, obj_s *o)
{
    o->timer = 0;
    int tx   = o->pos.x >> 4;
    int ty   = o->pos.y >> 4;

    switch (o->state) {
    case TOGGLEBLOCK_OFF:
        o->trigger                               = o->trigger_on_1;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_BLOCK;
        game_on_solid_appear(g);
        break;
    case TOGGLEBLOCK_ON:
        o->trigger                               = o->trigger_on_0;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_EMPTY;
        break;
    }
    o->state = 1 - o->state;
}