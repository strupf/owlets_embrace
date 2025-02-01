// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

enum {
    CRUMBLE_STATE_IDLE,
    CRUMBLE_STATE_BREAKING,
    CRUMBLE_STATE_RESPAWNING,
};

#define CRUMBLE_TICKS_BREAK   25
#define CRUMBLE_TICKS_RESPAWN 100

void crumbleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void crumbleblock_on_update(g_s *g, obj_s *o);
void crumbleblock_on_animate(g_s *g, obj_s *o);

void crumbleblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJID_CRUMBLEBLOCK;
    o->on_update       = crumbleblock_on_update;
    o->on_draw         = crumbleblock_on_draw;
    o->render_priority = 0;
    o->state           = CRUMBLE_STATE_IDLE;
    o->substate        = TILE_BLOCK;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;

    o->substate = map_obj_bool(mo, "Platform") ? TILE_ONE_WAY : TILE_BLOCK;
    // i32 tt             = o->substate == TILE_BLOCK ? TILE_TYPE_DIRT : 0;
    i32 tt      = 0;
    tile_map_set_collision(g, obj_aabb(o), o->substate, tt);
}

void crumbleblock_break(g_s *g, obj_s *o);

static void crumbleblock_start_breaking(obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE) {
        o->state = CRUMBLE_STATE_BREAKING;
        o->timer = CRUMBLE_TICKS_BREAK;
    }
}

void crumbleblock_on_hooked(obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE && o->substate == TILE_BLOCK) {
        crumbleblock_start_breaking(o);
    }
}

void crumbleblock_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case CRUMBLE_STATE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;

        rec_i32 r = {o->pos.x, o->pos.y, o->w, 1};
        if (0 <= ohero->v_q8.y && overlap_rec(obj_rec_bottom(ohero), r)) {
            crumbleblock_start_breaking(o);
        }
        break;
    }
    case CRUMBLE_STATE_BREAKING: {
        o->timer--;
        if (0 < o->timer) break;

        tile_map_set_collision(g, obj_aabb(o), 0, 0);
        crumbleblock_break(g, o);
        o->state = CRUMBLE_STATE_RESPAWNING;
        o->timer = CRUMBLE_TICKS_RESPAWN;
        break;
    }
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (0 < o->timer) break;

        o->state = CRUMBLE_STATE_IDLE;
        tile_map_set_collision(g, obj_aabb(o), o->substate, 0);
        game_on_solid_appear(g);
        break;
    }
    }
}

void crumbleblock_break(g_s *g, obj_s *o)
{
    return;
    i32 tx = o->w >> 4;
    i32 ty = o->h >> 4;

    // spawn subtile block particles
    for (i32 y = 0; y < ty; y++) {
        for (i32 x = 0; x < tx; x++) {
            i32 t = tileindex_terrain_block(tx, ty, TILE_TYPE_BRIGHT_BREAKING, x, y);

            for (i32 yi = 0; yi <= 1; yi++) {
                for (i32 xi = 0; xi <= 1; xi++) {
                    v2_i32  pos = {o->pos.x + x * 16 + xi * 16 - 8,
                                   o->pos.y + y * 16 + yi * 16 - 8};
                    rec_i32 rec = {xi * 16,
                                   yi * 16 + (t << 5),
                                   16, 16};
                    obj_s  *d   = spritedecal_create(g, 0, 0, pos,
                                                     TEXID_TILESET_TERRAIN,
                                                     rec, 30, 1, 0);
                    d->v_q8.y   = rngr_i32(-500, -100);
                    d->v_q8.x   = rngr_sym_i32(100);
                }
            }
        }
    }
}

void crumbleblock_on_animate(g_s *g, obj_s *o)
{
}

void crumbleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    // shake offset table (have to align 50/50 dither pattern)
    static const v2_i8 shakeoff[9] = {{+0, +0},
                                      {+1, +1},
                                      {+1, -1},
                                      {-1, +1},
                                      {-1, -1},
                                      {+0, +2},
                                      {+0, -2},
                                      {+2, +0},
                                      {-2, +0}};

    if (o->state == CRUMBLE_STATE_RESPAWNING) return;

    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);

    if (o->state == CRUMBLE_STATE_BREAKING) {
        u32 seed   = pltf_cur_tick() >> 1;
        i32 sindex = rngsr_i32(&seed, 0, 9);
        p.x += shakeoff[sindex].x;
        p.y += shakeoff[sindex].y;
    }

    render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4,
                              TILE_TYPE_BRIGHT_BREAKING);
}
