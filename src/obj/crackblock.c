// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// gets triggered by touching it and falls down

#include "game.h"

enum {
    CRACKBLOCK_ST_IDLE,
    CRACKBLOCK_ST_SHAKE,
    CRACKBLOCK_ST_FALL,
    CRACKBLOCK_ST_IDLE_FELL,
};

typedef struct {
    // 1 tile bigger on each side
    tile_s *tiles;
    i32     tile_type;
    i32     on_trigger;
    i32     trigger_on_destroy;
} crackblock_s;

#define CRACKBLOCK_SHAKE_T 16

void crackblock_on_update(g_s *g, obj_s *o);
void crackblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void crackblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void crackblock_break(g_s *g, obj_s *o);

void crackblock_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o    = obj_create(g);
    crackblock_s *b    = (crackblock_s *)o->mem;
    o->ID              = OBJID_CRACKBLOCK;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->render_priority = RENDER_PRIO_INFRONT_FLUID_AREA - 1;

    // need 1 tile of space around the actual bounding box for autotiling
    i32 px                = o->pos.x >> 4;
    i32 py                = o->pos.y >> 4;
    i32 nx                = o->w >> 4;
    i32 ny                = o->h >> 4;
    b->on_trigger         = map_obj_i32(mo, "crack_on_trigger");
    b->trigger_on_destroy = map_obj_i32(mo, "triggers");
    b->tile_type          = map_terrain_enum_to_type(map_obj_i32(mo, "terrainID"));
    b->tiles              = game_alloctn(g, tile_s, (nx + 2) * (ny + 2));

    // autotiling this stone
    for (i32 y = 0; y < ny; y++) {
        for (i32 x = 0; x < nx; x++) {
            tile_s *tb = &b->tiles[(1 + x) + (1 + y) * (nx + 2)];
            tb->type   = b->tile_type;
            tb->shape  = TILE_BLOCK;
        }
    }
    autotile_terrain(b->tiles, nx + 2, ny + 2, 0, 0);

    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) {
        o->state   = CRACKBLOCK_ST_IDLE_FELL;
        o->on_draw = crackblock_on_draw;
        o->flags   = OBJ_FLAG_SOLID | OBJ_FLAG_CLIMBABLE;
        rec_i32 rb = obj_rec_bottom(o);
        for (i32 steps = 1024; steps && !map_blocked(g, rb); steps--) {
            o->pos.y++;
            rb.y++;
        }
    } else {
        o->substate = saveID;

        if (b->on_trigger) {
            o->on_trigger = crackblock_on_trigger;
        } else {
            o->on_update = crackblock_on_update;
        }

        if (map_obj_bool(mo, "seamless")) { // place static terrain tiles
            for (i32 y = 0; y < ny; y++) {
                for (i32 x = 0; x < nx; x++) {
                    tile_s *t = &g->tiles[(px + x) + (py + y) * g->tiles_x];
                    t->type   = b->tile_type;
                    t->shape  = TILE_BLOCK;
                }
            }
            autotile_terrain_section(g->tiles, g->tiles_x, g->tiles_y, 0, 0,
                                     px - 2, py - 2, nx + 4, ny + 4);
        } else {
            o->on_draw = crackblock_on_draw;
        }
    }
}

void crackblock_on_update(g_s *g, obj_s *o)
{
    crackblock_s *b = (crackblock_s *)o->mem;
    o->timer++;

    switch (o->state) {
    case CRACKBLOCK_ST_IDLE: {
        if (b->on_trigger) break;

        obj_s *ohero = obj_get_owl(g);
        if (!ohero) break;

        rec_i32 rh = obj_aabb(ohero);
        if (overlap_rec_touch(obj_aabb(o), rh)) {
            crackblock_break(g, o);
        }
        break;
    }
    case CRACKBLOCK_ST_SHAKE: {
        if (CRACKBLOCK_SHAKE_T <= o->timer) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case CRACKBLOCK_ST_FALL: {
        o->v_q12.y += Q_VOBJ(0.3);
        o->subpos_q12.y += o->v_q12.y;
        i32 dy = (o->subpos_q12.y >> 12) & ~1;
        o->subpos_q12.y &= 0x1FFF;

        for (i32 k = 0; k < dy; k++) {
            if (!map_blocked(g, obj_rec_bottom(o))) {
                obj_move(g, o, 0, 1);
                continue;
            }

            // landed on the floor
            // spawn dust explosions
            save_event_register(g, o->substate);
            o->on_update = 0;
            o->v_q12.y   = 0;
            cam_screenshake_xy(&g->cam, 15, 0, 4);
            snd_play(SNDID_EXPLO1, 0.3f, 1.5f);

            i32 w = o->w >> 4;
            for (i32 n = 0; n < w; n++) {
                v2_i32 panim = {o->pos.x + n * 16 + 8, o->pos.y + o->h};
                obj_s *oanim = objanim_create(g, panim, OBJANIM_BOULDER_POOF);

                oanim->render_priority = RENDER_PRIO_INFRONT_TERRAIN_LAYER;
            }
            break;
        }
        break;
    }
    }
}

void crackblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    crackblock_s *b   = (crackblock_s *)o->mem;
    gfx_ctx_s     ctx = gfx_ctx_display();
    v2_i32        p   = v2_i32_add(o->pos, cam);
    texrec_s      tr  = asset_texrec(0, 0, 0, 64, 64);

    if (o->state == CRACKBLOCK_ST_SHAKE) {
        p.x += rngr_i32(0, 2) * 2 - 2;
    }

    p.x &= ~1;
    p.y &= ~1;

    i32 nx = o->w >> 4;
    i32 ny = o->h >> 4;
    for (i32 y = 0; y < ny; y++) {
        for (i32 x = 0; x < nx; x++) {
            tile_s  *t    = &b->tiles[(1 + x) + (1 + y) * (nx + 2)];
            texrec_s trec = {asset_tex(TEXID_TILESET_TERRAIN), 0, (i32)t->ty << 5, 32, 32};
            v2_i32   pos  = {p.x + x * 16 - 8, p.y + y * 16 - 8};
            gfx_spr_tile_32x32(ctx, trec, pos);
        }
    }
}

void crackblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    crackblock_s *b = (crackblock_s *)o->mem;
    if (b->on_trigger == trigger) {
        crackblock_break(g, o);
        o->on_update = crackblock_on_update;
    }
}

void crackblock_break(g_s *g, obj_s *o)
{
    crackblock_s *b = (crackblock_s *)o->mem;
    o->timer        = 0;
    o->state        = CRACKBLOCK_ST_SHAKE;
    o->on_draw      = crackblock_on_draw;
    o->flags        = OBJ_FLAG_SOLID | OBJ_FLAG_CLIMBABLE;

    // clear static terrain tiles
    i32 px = o->pos.x >> 4;
    i32 py = o->pos.y >> 4;
    i32 nx = o->w >> 4;
    i32 ny = o->h >> 4;
    for (i32 y = 0; y < ny; y++) {
        mclr(&g->tiles[(px) + (py + y) * g->tiles_x], sizeof(tile_s) * nx);
    }
    autotile_terrain_section(g->tiles, g->tiles_x, g->tiles_y, 0, 0,
                             px - 2, py - 2, nx + 4, ny + 4);
    game_on_trigger(g, b->trigger_on_destroy);
}