// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32     saveID;
    i32     trigger_destroy;
    i32     terrainID;
    tile_s *tiles_og;
} shortcutblock_s;

void shortcutblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void shortcutblock_set(g_s *g, obj_s *o, i32 state);

void shortcutblock_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    obj_s           *o = obj_create(g);
    shortcutblock_s *b = (shortcutblock_s *)o->mem;
    o->UUID            = mo->UUID;
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
    b->saveID          = saveID;
    b->tiles_og        = game_per_room_alloctn(g, tile_s, nx * ny);

    for (i32 y = 0; y < ny; y++) {
        mcpy(&b->tiles_og[y * nx],
             &g->tiles[px + (py + y) * g->tiles_x],
             sizeof(tile_s) * nx);
    }

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
    case 0: {
        // disappear: smokes and mirrors
        i32 nx_expl = 1 + nx / 4;
        i32 ny_expl = 1 + ny / 4;
        i32 space_x = o->w / (1 + nx_expl);
        i32 space_y = o->h / (1 + ny_expl);

        for (i32 y = 0; y < ny_expl; y++) {
            for (i32 x = 0; x < nx_expl; x++) {
                v2_i32 p = {o->pos.x + (1 + x) * space_x + rngr_sym_i32(8),
                            o->pos.y + (1 + y) * space_y + rngr_sym_i32(8)};
                animobj_create(g, p, ANIMOBJ_EXPLOSION_5);
            }
        }

        // set original tiles again
        for (i32 y = 0; y < ny; y++) {
            for (i32 x = 0; x < nx; x++) {
                g->tiles[(px + x) + (py + y) * g->tiles_x] = b->tiles_og[x + y * nx];
            }
        }
        break;
    }
    case 1:
        // overwrite map tiles with tiles of this block
        for (i32 y = 0; y < ny; y++) {
            for (i32 x = 0; x < nx; x++) {
                tile_s *t = &g->tiles[(px + x) + (py + y) * g->tiles_x];
                if (t->shape != TILE_BLOCK) {
                    t->type  = b->terrainID;
                    t->shape = TILE_BLOCK;
                }
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