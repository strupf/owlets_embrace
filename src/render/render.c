// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"

int cmp_obj_render_priority(const void *a, const void *b)
{
    const obj_s *x = *(const obj_s **)a;
    const obj_s *y = *(const obj_s **)b;
    if (x->render_priority < y->render_priority) return -1;
    if (x->render_priority > y->render_priority) return +1;
    return 0;
}

static void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 camoffset)
{
    v2_i32  ppos = v2_add(o->pos, camoffset);
    rec_i32 aabb = {ppos.x, ppos.y, o->w, o->h};

#ifdef SYS_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
    }
#endif

    if (o->flags & OBJ_FLAG_SPRITE) {
        for (int n = 0; n < o->n_sprites; n++) {
            sprite_simple_s sprite = o->sprites[n];
            if (sprite.trec.t.px == NULL) continue;
            v2_i32 sprpos = v2_add(ppos, sprite.offs);
#if 0
            txrec_s txr     = {0};
            txr.t           = tx_from_tex(sprite.trec.t, malloc);
            txr.r           = sprite.trec.r;
            gfx_ctx2_s ctx2 = {0};
            ctx2.dst        = tx_display(ctx.dst);
            memcpy(&ctx2.pat, &ctx.pat, sizeof(ctx.pat));
            ctx2.clip_x1 = ctx.clip_x1;
            ctx2.clip_y1 = ctx.clip_y1;
            ctx2.clip_x2 = ctx.clip_x2;
            ctx2.clip_y2 = ctx.clip_y2;
            gfx_spr2(ctx2, txr, sprpos, sprite.flip, sprite.mode);

            free(txr.t.px);
#else
            gfx_spr_display(ctx, sprite.trec, sprpos, sprite.flip, sprite.mode);
#endif
        }
    }

    switch (o->ID) {
    case OBJ_ID_BLOB:
        blob_on_draw(g, o, camoffset);
        break;
    case OBJ_ID_TOGGLEBLOCK:
        toggleblock_on_draw(g, o, camoffset);
        break;
    case OBJ_ID_HEROUPGRADE:
        heroupgrade_on_draw(g, o, camoffset);
        break;
    case OBJ_ID_HERO: {
        herodata_s *hero = &g->herodata;
#if 0 // render hitboxes
            gfx_ctx_s ctxhb = ctx;
            for (int i = 0; i < hero->n_hitbox; i++) {
                hitbox_s hb = hero->hitbox_def[i];
                hb.r.x += camoffset.x;
                hb.r.y += camoffset.y;
                ctxhb.pat = gfx_pattern_interpolate(1, 2);
                gfx_rec_fill(ctxhb, hb.r, PRIM_MODE_BLACK_WHITE);
            }
#endif

#if 0 // air jump indicators
            herodata_s *hero  = &g->herodata;
            bool32  inair = game_traversable(g, obj_rec_bottom(o));
            if (hero->n_airjumps > 0 && inair) {
                gfx_ctx_s ctx_airjump = gfx_ctx_default(asset_tex(0));

                for (int k = 0; k < hero->n_airjumps; k++) {
                    v2_i32 ajpos = ppos;
                    ajpos.x += -5 + k * 10;
                    ajpos.y -= 20;

                    gfx_cir_fill(ctx_airjump, ajpos, 6, PRIM_MODE_BLACK);
                    gfx_cir_fill(ctx_airjump, ajpos, 5, PRIM_MODE_WHITE);
                    if (o->n_airjumps > k) {
                        gfx_cir_fill(ctx_airjump, ajpos, 4, PRIM_MODE_BLACK);
                    }
                }
            }
#endif
    } break;
    }

#ifdef SYS_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_ctx_s ctx_aabb = ctx;
        ctx_aabb.pat       = gfx_pattern_interpolate(1, 3);
        gfx_rec_fill(ctx_aabb, aabb, PRIM_MODE_BLACK);
    }
#endif
}

void game_draw(game_s *g)
{
    const rec_i32   camrec    = cam_rec_px(g, &g->cam);
    const v2_i32    camoffset = {-camrec.x, -camrec.y};
    const gfx_ctx_s ctx       = gfx_ctx_display();

    render_bg(g, camrec);
    ocean_s *ocean = &g->ocean;

    bounds_2D_s tilebounds = game_tilebounds_rec(g, camrec);
    render_tilemap(g, TILELAYER_BG, tilebounds, camoffset);

#if 0 // roughly cuts FPS in half
    void tex_px_unsafe_display(tex_s tex, int x, int y, int col);
    for (int y = 0, it = 1; y < camrec.h; y++, it = 1 - it) {
        int pposy = y - camoffset.y;
        int tiley = (pposy >> 4) * g->tiles_x;
        for (int x = it; x < camrec.w; x += 2) {
            int pposx = x - camoffset.x;

            if (g->rtiles[TILELAYER_BG][(pposx >> 4) + tiley].u == 0) continue;

            rec_i32 rr = {pposx - 4, pposy - 4, 9, 9};
            if (tiles_solid(g, rr)) {
                tex_px_unsafe_display(ctx.dst, x, y, 0);
            }
        }
    }

#endif

    render_tilemap(g, TILELAYER_PROP_BG, tilebounds, camoffset);
    render_tilemap(g, TILELAYER_TERRAIN, tilebounds, camoffset);

    obj_s  *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *rope  = ohero ? ohero->rope : NULL;
    if (rope) {
#define ROPE_SEG_DISTANCE 120
        const int seg_len = max_i((ROPE_SEG_DISTANCE * rope_length_q4(g, rope)) / rope->len_max_q4, ROPE_SEG_DISTANCE);

        for (int pass = 0; pass < 2; pass++) {
            texrec_s tseg = asset_texrec(TEXID_HOOK, 32, pass * 12, 12, 12);

            int inode       = 0;
            int lensofar_q4 = 0;

            for (ropenode_s *r1 = rope->tail, *r2 = r1->prev;
                 r2; r1 = r2, r2 = r2->prev) {
                v2_i32 p1        = v2_add(r1->p, camoffset);
                v2_i32 p2        = v2_add(r2->p, camoffset);
                v2_i32 dt12_q4   = v2_shl(v2_sub(p2, p1), 4);
                int    lenend_q4 = lensofar_q4 + v2_len(dt12_q4);

                while (inode < lenend_q4) {
                    int dst = inode - lensofar_q4;
                    inode += seg_len;
                    assert(0 <= dst);

                    v2_i32 dd = dt12_q4;
                    dd        = v2_setlen(dd, dst);
                    dd        = v2_shr(dd, 4);
                    dd        = v2_add(dd, p1);
                    gfx_spr_cpy_display(ctx, tseg, dd);
                }
                lensofar_q4 = lenend_q4;
            }
        }
    }

    if (g->objrender_dirty) {
        g->objrender_dirty = 0;
        sort_array(g->obj_render, g->n_objrender, sizeof(obj_s *), cmp_obj_render_priority);
    }

    for (int n = 0; n < g->n_objrender; n++) {
        obj_draw(ctx, g, g->obj_render[n], camoffset);
    }

    for (int i = 0; i < g->particles.n; i++) {
        particle_s *p           = &g->particles.particles[i];
        v2_i32      ppos        = v2_add(v2_shr(p->p_q8, 8), camoffset);
        gfx_ctx_s   ctxparticle = ctx;
        ctxparticle.pat         = gfx_pattern_interpolate(p->ticks, p->ticks_max);

        switch (p->gfx) {
        case PARTICLE_GFX_CIR: {
            gfx_cir_fill(ctxparticle, ppos, p->size, PRIM_MODE_BLACK);
        } break;
        case PARTICLE_GFX_REC: {
            rec_i32 rr = {ppos.x, ppos.y, p->size, p->size};
            gfx_rec_fill(ctxparticle, rr, PRIM_MODE_BLACK);
        } break;
        case PARTICLE_GFX_SPR: {
            gfx_spr(ctxparticle, p->texrec, ppos, 0, 0);
        } break;
        }
    }

    texrec_s trgrass;
    trgrass.t = asset_tex(TEXID_PLANTS);
    for (int n = 0; n < g->n_grass; n++) {
        grass_s *gr     = &g->grass[n];
        rec_i32  rgrass = {gr->pos.x - 8, gr->pos.y - 8, 32, 32};
        if (!overlap_rec(rgrass, camrec)) continue;

        v2_i32 pos = v2_add(gr->pos, camoffset);

        // prerender?
        for (int i = 0; i < 16; i++) {
            v2_i32 p = pos;
            p.y += i;
            p.x += (gr->x_q8 * (15 - i)) >> 8; // shear
            rec_i32 rg = {8, i + gr->type * 16, 16, 1};
            trgrass.r  = rg;
            gfx_spr_display(ctx, trgrass, p, 0, 0);
        }
    }

    ocean_draw(g, asset_tex(0), camoffset);

    if (g->env_effects & ENVEFFECT_WIND)
        enveffect_wind_draw(ctx, &g->env_wind, camoffset);
    if (g->env_effects & ENVEFFECT_HEAT)
        enveffect_heat_draw(ctx, &g->env_heat, camoffset);

    render_ui(g, camoffset);
    transition_draw(&g->transition);
#if 0
    texrec_s   trrr = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 128, 128);
    static int posxx;
    static int posyy;
    if (sys_key(SYS_KEY_UP)) posyy -= 3;
    if (sys_key(SYS_KEY_DOWN)) posyy += 3;
    if (sys_key(SYS_KEY_LEFT)) posxx -= 3;
    if (sys_key(SYS_KEY_RIGHT)) posxx += 3;
    gfx_spr(ctx, trrr, (v2_i32){posxx, posyy}, SPR_FLIP_X, 0);
#endif
}

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 camoffset)
{
    texrec_s tr = {0};
    switch (layer) {
    case TILELAYER_BG:
    case TILELAYER_TERRAIN:
        tr.t = asset_tex(TEXID_TILESET_TERRAIN);
        break;
    case TILELAYER_PROP_BG:
        tr.t = asset_tex(TEXID_TILESET_PROPS_BG);
        break;
    default: return;
    }
    gfx_ctx_s ctx = gfx_ctx_display();
    tr.r.w        = 16;
    tr.r.h        = 16;

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            rtile_s rt = g->rtiles[layer][x + y * g->tiles_x];
            if (rt.u == 0) continue;

            tr.r.x   = rt.tx << 4;
            tr.r.y   = rt.ty << 4;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            gfx_spr_display(ctx, tr, p, 0, 0);
            // gfx_spr2(ctx2, trr, p, 0, 0);
#if defined(SYS_DEBUG) && 0
            int t1 = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t1 && t1 < NUM_TILE_BLOCKS)) continue;
            texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
            gfx_spr(ctx, tr1, p, 0, 0);
#endif
        }
    }
}

void ocean_calc_spans(game_s *g, rec_i32 camr)
{
    ocean_s *oc = &g->ocean;
    if (!oc->active) {
        oc->y_min = camr.h;
        oc->y_max = camr.h;
        return;
    }

    int (*height)(game_s * g, int pixel_x) =
        inp_debug_space() ? ocean_height : ocean_render_height;

    oc->y_min          = I32_MAX;
    oc->y_max          = I32_MIN;
    oc->n_spans        = 1;
    ocean_span_s *span = &oc->spans[0];
    span->y            = height(g, 0 + camr.x) - camr.y;
    span->w            = 1;

    for (int x = 1; x < camr.w; x++) {
        int xpos = x + camr.x;
        i32 h    = height(g, x + camr.x) - camr.y;

        oc->y_min = min_i32(oc->y_min, h);
        oc->y_max = max_i32(oc->y_max, h);
        if (h != span->y) {
            span    = &oc->spans[oc->n_spans++];
            span->y = h;
            span->w = 0;
        }
        span->w++;
    }
}

void ocean_draw(game_s *g, tex_s t, v2_i32 camoff)
{
    ocean_s *oc = &g->ocean;
    if (!oc->active) return;
    const gfx_ctx_s ctx  = gfx_ctx_default(t);
    gfx_ctx_s       ctxf = ctx;

    if ((camoff.x ^ camoff.y) & 1) {
        ctxf.pat = gfx_pattern_4x4(B4(0101),
                                   B4(1010),
                                   B4(0101),
                                   B4(1010));
    } else {
        ctxf.pat = gfx_pattern_4x4(B4(1010),
                                   B4(0101),
                                   B4(1010),
                                   B4(0101));
    }

    gfx_ctx_s ctxl = ctx;
    ctxl.pat       = gfx_pattern_interpolate(1, 6);
    int y_max      = clamp_i(oc->y_max, 0, t.h - 1);

    { // fill "static" bottom section
        u32 *px = &((u32 *)t.px)[y_max * t.wword];
        for (int y = y_max; y < t.h; y++) {
            u32 pt = ~ctxf.pat.p[y & 1];
            for (int x = 0; x < t.wword; x++)
                *px++ &= pt;
        }
    }

    // dynamic sin section
    for (int k = 0, x = 0; k < oc->n_spans; k++) {
        ocean_span_s sp  = oc->spans[k];
        rec_i32      rl1 = {x, sp.y + 2, sp.w, 2};
        rec_i32      rl2 = {x, sp.y + 4, sp.w, 2};
        rec_i32      rfb = {x, sp.y, sp.w, y_max - sp.y};
        gfx_rec_fill_display(ctxf, rfb, PRIM_MODE_BLACK);
        gfx_rec_fill_display(ctxf, rl1, PRIM_MODE_WHITE);
        gfx_rec_fill_display(ctxl, rl2, PRIM_MODE_WHITE);
        x += sp.w;
    }
}