// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

enum {
    STOMPABLEBLOCK_IDLE,
    STOMPABLEBLOCK_BREAKING,
};

typedef struct {
    i32    saveID;
    bool32 standingon;
} stompable_block_s;

#define DITHERAREA_TICKS      50
#define STOMPABLE_TICKS_BREAK 80

void ditherarea_on_update(g_s *g, obj_s *o);
void ditherarea_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void ditherarea_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID_readonly");
    if (save_event_exists(g, saveID)) return;

    obj_s *o           = obj_create(g);
    o->ID              = OBJID_DITHERAREA;
    o->render_priority = RENDER_PRIO_INFRONT_TERRAIN_LAYER - 2;
    o->on_update       = ditherarea_on_update;
    o->on_draw         = ditherarea_on_draw;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->state           = saveID;
}

void ditherarea_on_update(g_s *g, obj_s *o)
{
    if (o->timer) {
        o->timer--;
        if (o->timer == 0) {
            obj_delete(g, o);
        }
    }
}

void ditherarea_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    rec_i32   rf  = translate_rec(obj_aabb(o), cam.x, cam.y);
    if (o->timer) {
        ctx.pat = gfx_pattern_interpolate(o->timer, DITHERAREA_TICKS + 4);
    } else {
        ctx.pat = gfx_pattern_4x4(B4(1111),
                                  B4(1111),
                                  B4(1111),
                                  B4(0111));
    }

    gfx_rec_fill(ctx, rf, GFX_COL_BLACK);
}

void stompable_block_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void stompable_block_on_update(g_s *g, obj_s *o);

void stompable_block_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    obj_s *o             = obj_create(g);
    o->ID                = OBJID_STOMPABLE_BLOCK;
    o->flags             = OBJ_FLAG_SOLID;
    o->on_draw           = stompable_block_on_draw;
    o->on_update         = stompable_block_on_update;
    o->render_priority   = RENDER_PRIO_INFRONT_TERRAIN_LAYER - 1;
    stompable_block_s *b = (stompable_block_s *)o->mem;
    b->saveID            = saveID;
    o->w                 = mo->w;
    o->h                 = mo->h;
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
}

void stompable_block_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    i32       nx  = o->w >> 4;
    i32       ny  = o->h >> 4;
    v2_i32    p   = v2_i32_add(o->pos, cam);

    switch (o->state) {
    case STOMPABLEBLOCK_IDLE: {
        i32 tID = TILE_TYPE_BRIGHT_STOMP;
        if (o->timer && ((o->timer >> 2) & 1))
            tID++;
        render_tile_terrain_block(ctx, p, nx, ny, tID);
        break;
    }
    case STOMPABLEBLOCK_BREAKING: {
        // falling particles using integration: x = att/2 + vt
        tex_s     tterrain = asset_tex(TEXID_TILESET_TERRAIN);
        i32       nx       = o->w >> 4;
        i32       ny       = o->h >> 4;
        i32       t        = o->timer - 1;
        u32       s        = 213;
        i32       py_acc   = 30 * t * t;
        gfx_ctx_s ctxp     = ctx;

        if (STOMPABLE_TICKS_BREAK - 20 <= o->timer) {
            i32 t1   = o->timer - (STOMPABLE_TICKS_BREAK - 20);
            ctxp.pat = gfx_pattern_interpolate(20 - t1, 20);
        }

        for (i32 ty = 0; ty < ny; ty++) {
            for (i32 tx = 0; tx < nx; tx++) {
                i32 k = tileindex_terrain_block(nx, ny,
                                                TILE_TYPE_BRIGHT_STOMP,
                                                tx, ty);

                // subdivide each tile in 4 subtile particles
                for (i32 i = 0; i < 4; i++) {
                    i32 u = i & 1;
                    i32 v = i >> 1;

                    texrec_s trec = {tterrain, (u << 4), (v << 4) + (k << 5),
                                     16, 16};
                    v2_i32   v_q8 = {rngsr_i32(&s, -100, +100),
                                     rngsr_i32(&s, -150, +150) - 350};
                    v2_i32   p_q8 = {v_q8.x * t,
                                     v_q8.y * t + py_acc};
                    v2_i32   p_px = {p.x + (tx << 4) - 8 + (u << 4),
                                     p.y + (ty << 4) - 8 + (v << 4)};
                    p_px          = v2_i32_add(p_px, v2_i32_shr(p_q8, 8));
                    gfx_spr(ctxp, trec, p_px, 0, 0);
                }
            }
        }
        break;
    }
    }
}

void stompable_block_on_update(g_s *g, obj_s *o)
{
    stompable_block_s *b = (stompable_block_s *)o->mem;
    switch (o->state) {
    case STOMPABLEBLOCK_IDLE: {
        obj_s *ohero;

        if (o->timer) {
            o->timer--;
        }

        if (!hero_present_and_alive(g, &ohero)) {
            b->standingon = 0;
            break;
        }

        rec_i32 herofeet   = obj_rec_bottom(ohero);
        bool32  standingon = overlap_rec(herofeet, obj_aabb(o)) &&
                            obj_grounded(g, ohero);

        if (!b->standingon && standingon) {
            o->timer = 16;
            i32 nx   = o->w >> 4;
            for (i32 n = 0; n < nx; n++) {
                v2_i32 ptop = {o->pos.x + n * 16 + 8, o->pos.y};
                v2_i32 pbot = {o->pos.x + n * 16 + 8, o->pos.y + o->h};
                particle_emit_ID(g, PARTICLE_EMIT_ID_STOMPBLOCK_HINT, ptop);
                particle_emit_ID(g, PARTICLE_EMIT_ID_STOMPBLOCK_HINT, pbot);
            }
        }
        b->standingon = standingon;
        break;
    }
    case STOMPABLEBLOCK_BREAKING: {
        o->timer++;
        if (STOMPABLE_TICKS_BREAK <= o->timer) {
            obj_delete(g, o);
        }
        break;
    }
    }
}

void stompable_block_on_destroy(g_s *g, obj_s *o)
{
    if (o->state != STOMPABLEBLOCK_IDLE) return;
    stompable_block_s *b = (stompable_block_s *)o->mem;
    save_event_register(g, b->saveID);
    o->state = STOMPABLEBLOCK_BREAKING;
    o->timer = 0;
    o->flags &= ~OBJ_FLAG_SOLID;

    for (obj_each(g, i)) {
        if (i->ID != OBJID_DITHERAREA) continue;
        if (i->state == b->saveID) {
            i->timer = DITHERAREA_TICKS;
        }
    }
}