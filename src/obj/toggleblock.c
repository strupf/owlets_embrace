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
    o->trigger = o->trigger_off;
    return o;
}

void toggleblock_on_animate(game_s *g, obj_s *o)
{
    o->timer++;
}

void toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
#define TOGGLEBLOCK_TICKS 6

    gfx_ctx_s ctx   = gfx_ctx_default(asset_tex(0));
    gfx_ctx_s ctx_0 = ctx;
    int       i     = min_i(o->timer, TOGGLEBLOCK_TICKS);
    switch (o->state) {
    case TOGGLEBLOCK_OFF:
        ctx.pat = gfx_pattern_interpolate(TOGGLEBLOCK_TICKS - i, TOGGLEBLOCK_TICKS);
        break;
    case TOGGLEBLOCK_ON:
        ctx.pat = gfx_pattern_interpolate(i, TOGGLEBLOCK_TICKS);
        break;
    }

    tex_s tex;
    asset_tex_load("toggleblock.tex", &tex);
    texrec_s tr0;
    tr0.t = tex;
    tr0.r = (rec_i32){16, 16, 16, 16};
    texrec_s tr1;
    tr1.t = tex;
    tr1.r = (rec_i32){48, 16, 16, 16};

    v2_i32 pos = v2_add(o->pos, cam);

    gfx_spr(ctx_0, tr1, pos, 0, 0);
    gfx_spr(ctx, tr0, pos, 0, 0);
}

void toggleblock_on_trigger(game_s *g, obj_s *o)
{
    o->timer = 0;
    int tx   = o->pos.x >> 4;
    int ty   = o->pos.y >> 4;

    switch (o->state) {
    case TOGGLEBLOCK_OFF: {
        o->trigger                               = o->trigger_on;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_BLOCK;
        game_on_solid_appear(g);
    } break;
    case TOGGLEBLOCK_ON: {
        o->trigger                               = o->trigger_off;
        g->tiles[tx + ty * g->tiles_x].collision = TILE_EMPTY;
    } break;
    }
    o->state = 1 - o->state;
}