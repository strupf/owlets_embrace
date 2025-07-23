// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32     saveID;
    tile_s *tiles_og;
    i32     trigger_destroy;
} shortcutblock_s;

void shortcutblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void shortcutblock_set(g_s *g, obj_s *o, i32 state);

void shortcutblock_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    obj_s           *o = obj_create(g);
    shortcutblock_s *b = (shortcutblock_s *)o->mem;
    o->ID              = OBJID_SHORTCUTBLOCK;
    o->w               = mo->w;
    o->h               = mo->h;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->on_trigger      = shortcutblock_on_trigger;
    i32 px             = o->pos.x >> 4;
    i32 py             = o->pos.y >> 4;
    i32 nx             = o->w >> 4;
    i32 ny             = o->h >> 4;
    b->trigger_destroy = map_obj_i32(mo, "trigger_destroy");
    b->tiles_og        = game_alloctn(g, tile_s, nx * ny);
    for (i32 y = 0; y < ny; y++) {
        mcpy(&b->tiles_og[y * nx],
             &g->tiles[px + (py + y) * g->tiles_x],
             sizeof(tile_s) * nx);
    }
    b->saveID = saveID;
    shortcutblock_set(g, o, 1);
}

void shortcutblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    shortcutblock_s *b = (shortcutblock_s *)o->mem;
    if (trigger == b->trigger_destroy) {
        shortcutblock_set(g, o, 0);
        save_event_register(g, b->saveID);
        obj_delete(g, o);
    }
}

void shortcutblock_set(g_s *g, obj_s *o, i32 state)
{
    if (state == o->state) return;

    shortcutblock_s *b  = (shortcutblock_s *)o->mem;
    i32              px = o->pos.x >> 4;
    i32              py = o->pos.y >> 4;
    i32              nx = o->w >> 4;
    i32              ny = o->h >> 4;

    switch (state) {
    case 0:
        for (i32 y = 0; y < ny; y++) {
            mcpy(&g->tiles[px + (py + y) * g->tiles_x],
                 &b->tiles_og[y * nx],
                 sizeof(tile_s) * nx);
        }
        break;
    case 1:
        for (i32 y = 0; y < ny; y++) {
            for (i32 x = 0; x < nx; x++) {
                tile_s *t = &g->tiles[(px + x) + (py + y) * g->tiles_x];
                t->type   = TILE_TYPE_BRIGHT_STONE;
                t->shape  = TILE_BLOCK;
            }
        }
        break;
    }

    o->state = state;
    autotile_terrain_section(g->tiles, g->tiles_x, g->tiles_y, 0, 0,
                             px - 2, py - 2, nx + 4, ny + 4);
    if (o->state) {
        game_on_solid_appear(g);
    }
}