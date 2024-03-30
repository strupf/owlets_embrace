// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define TOGGLEBLOCK_TICKS 6

typedef struct {
    i32 trigger_to_enable;
    i32 trigger_to_disable;
} obj_toggleblock_s;

void toggleblock_on_animate(game_s *g, obj_s *o)
{
    o->timer++;
}

void toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    int sx = 0;

    switch (o->state) {
    case 0:
        sx = o->timer < TOGGLEBLOCK_TICKS ? 64 : 128;
        break;
    case 1:
        sx = o->timer < TOGGLEBLOCK_TICKS ? 64 : 0;
        break;
    }

    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = asset_texrec(TEXID_TOGGLE, 0, 0, 16, 16);
    v2_i32    pos = v2_add(o->pos, cam);
    int       nx  = o->w >> 4;
    int       ny  = o->h >> 4;

    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            v2_i32 p = {pos.x + (x << 4), pos.y + (y << 4)};
            if (ny == 1) {
                tr.r.y = 48;
            } else if (y == 0) {
                tr.r.y = 0;
            } else if (y == ny - 1) {
                tr.r.y = 32;
            } else {
                tr.r.y = 16;
            }
            if (nx == 1) {
                tr.r.x = 48 + sx;
            } else if (x == 0) {
                tr.r.x = 0 + sx;
            } else if (x == nx - 1) {
                tr.r.x = 32 + sx;
            } else {
                tr.r.x = 16 + sx;
            }

            gfx_spr(ctx, tr, p, 0, 0);
        }
    }
}

static void toggleblock_set_state(game_s *g, obj_s *o, int state)
{
    o->state = state;
    o->timer = 0;
    int b    = state == 1 ? TILE_BLOCK : TILE_EMPTY;

    game_set_collision_tiles(g, obj_aabb(o), b, b == TILE_BLOCK ? TILE_TYPE_DIRT : 0);
}

void toggleblock_on_trigger(game_s *g, obj_s *o, i32 trigger)
{
    obj_toggleblock_s *ot = (obj_toggleblock_s *)o->mem;

    switch (o->state) {
    case 0:
        if (trigger == ot->trigger_to_enable) {
            toggleblock_set_state(g, o, 1);
        }
        break;
    case 1:
        if (trigger == ot->trigger_to_disable) {
            toggleblock_set_state(g, o, 0);
        }
        break;
    }
}

void toggleblock_load(game_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_TOGGLEBLOCK;
    o->on_animate      = toggleblock_on_animate;
    o->on_draw         = toggleblock_on_draw;
    o->on_trigger      = toggleblock_on_trigger;
    o->render_priority = -10;
    o->w               = 16;
    o->h               = 16;
    o->state           = 0;
    o->timer           = TOGGLEBLOCK_TICKS;

    obj_toggleblock_s *ot  = (obj_toggleblock_s *)o->mem;
    o->pos.x               = mo->x;
    o->pos.y               = mo->y;
    o->w                   = mo->w;
    o->h                   = mo->h;
    ot->trigger_to_enable  = map_obj_i32(mo, "Trigger_enable");
    ot->trigger_to_disable = map_obj_i32(mo, "Trigger_disable");
    toggleblock_set_state(g, o, map_obj_bool(mo, "Enabled"));
    o->timer = TOGGLEBLOCK_TICKS;
}
