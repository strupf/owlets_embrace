// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
typedef struct {
    i32     x1;
    i32     y1;
    i32     x2;
    i32     y2;
    i32     saveID;
    i32     trigger_destroy;
    i32     terrainID;
    tile_s *tiles_og;
} shortcutblock_s;

static_assert(sizeof(shortcutblock_s) <= OBJ_MEM_BYTES, "s");
void shortcutblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void shortcutblock_set(g_s *g, obj_s *o, i32 state);

void shortcutblock_load(g_s *g, map_obj_s *mo)
{
    if (!map_obj_check_spawn_saveIDs(g, mo)) return;

    obj_s           *o = obj_create(g);
    shortcutblock_s *b = (shortcutblock_s *)o->mem;
    o->editorUID       = mo->UID;
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
    b->terrainID       = map_obj_i32(mo, "terrainID");
    b->tiles_og        = game_alloc_roomtn(g, tile_s, nx * ny);
    b->x1              = max_i32(px, 0);
    b->y1              = max_i32(py, 0);
    b->x2              = min_i32(px + nx - 1, g->tiles_x - 1);
    b->y2              = min_i32(py + ny - 1, g->tiles_y - 1);

    for (i32 y = 0; y < ny; y++) {
        mcpy(&b->tiles_og[y * nx], &g->tiles[px + (py + y) * g->tiles_x], sizeof(tile_s) * nx);
    }
    shortcutblock_set(g, o, 1);
}

void shortcutblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    shortcutblock_s *b = (shortcutblock_s *)o->mem;
    if (trigger == b->trigger_destroy) {
        shortcutblock_set(g, o, 0);
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

    o->state = state;

    switch (o->state) {
    case 0: {
        // remove this block
        for (i32 y = 0; y < ny; y++) {
            for (i32 x = 0; x < nx; x++) {
                g->tiles[(px + x) + (py + y) * g->tiles_x] = b->tiles_og[x + y * nx];
            }
        }
        obj_delete(g, o);
        break;
    }
    default:
    case 1: {
        tile_s t = {0};
        t.type   = b->terrainID;
        t.shape  = TILE_BLOCK;

        // overwrite map tiles with tiles of this block
        for (i32 y = 0; y < ny; y++) {
            for (i32 x = 0; x < nx; x++) {
                g->tiles[(px + x) + (py + y) * g->tiles_x] = t;
            }
        }
        game_on_solid_appear(g);
        break;
    }
    }
    autotile_terrain_section_game(g, px, py, nx, ny);
}