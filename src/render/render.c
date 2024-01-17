// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"

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
            gfx_spr_display(ctx, sprite.trec, sprpos, sprite.flip, sprite.mode);
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
    // render_tilemap(g, TILELAYER_BG, tilebounds, camoffset);

    for (int n = 0; n < g->n_decal_bg; n++) {
        gfx_ctx_s ctx_decal = ctx;
        decal_s   decal     = g->decal_bg[n];
        texrec_s  decalrec  = {decal.tex,
                               {0, 0, decal.tex.w, decal.tex.h}};
        v2_i32    decalpos  = {decal.x, decal.y};
        gfx_spr_display(ctx_decal, decalrec, v2_add(decalpos, camoffset), 0, 0);
    }

    render_tilemap(g, TILELAYER_TERRAIN, tilebounds, camoffset);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && ohero->rope) {
        rope_s   *rope     = ohero->rope;
        gfx_ctx_s ctx_rope = ctx;

        int         inode       = 0;
        int         lensofar_q4 = 0;
        ropenode_s *r1          = rope->tail;
        ropenode_s *r2          = r1->prev;

        int seg_len = (80 * rope_length_q4(g, rope)) / rope->len_max_q4;
        seg_len     = max_i(seg_len, 80);

        while (r1 && r2) {
            v2_i32 p1        = v2_add(r1->p, camoffset);
            v2_i32 p2        = v2_add(r2->p, camoffset);
            v2_i32 dt12_q4   = v2_shl(v2_sub(p2, p1), 4);
            int    lenend_q4 = lensofar_q4 + v2_len(dt12_q4);

            while (inode * seg_len < lenend_q4) {
                int dst = inode * seg_len - lensofar_q4;
                assert(0 <= dst);
                inode++;

                v2_i32 dd = dt12_q4;
                dd        = v2_setlen(dd, dst);
                dd        = v2_shr(dd, 4);
                dd        = v2_add(dd, p1);
                gfx_cir_fill(ctx_rope, dd, 6, PRIM_MODE_BLACK);
                gfx_cir_fill(ctx_rope, dd, 2, PRIM_MODE_WHITE);
            }
            lensofar_q4 = lenend_q4;
            r1          = r2;
            r2          = r2->prev;
        }
    }

    for (obj_each(g, o)) {
        obj_draw(ctx, g, o, camoffset);
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

    for (int n = 0; n < g->n_decal_fg; n++) {
        gfx_ctx_s ctx_decal = ctx;
        decal_s   decal     = g->decal_fg[n];
        texrec_s  decalrec  = {decal.tex,
                               {0, 0, decal.tex.w, decal.tex.h}};
        v2_i32    decalpos  = {decal.x, decal.y};
        gfx_spr_display(ctx_decal, decalrec, v2_add(decalpos, camoffset), 0, 0);
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
}

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 camoffset)
{
    texrec_s tr = {0};
    switch (layer) {
    case TILELAYER_TERRAIN:
        tr.t = asset_tex(TEXID_TILESET_TERRAIN);
        break;
    case TILELAYER_BG:
        tr.t = asset_tex(TEXID_TILESET_BG);
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

#if defined(SYS_DEBUG) && 0
            {
                int t = g->tiles[x + y * g->tiles_x].collision;
                if (!(0 < t && t < NUM_TILE_BLOCKS)) continue;
                texrec_s tr;
                tr.t   = asset_tex(TEXID_COLLISION_TILES);
                tr.r.x = 0;
                tr.r.y = t * 16;
                tr.r.w = 16;
                tr.r.h = 16;
                gfx_spr(ctx, tr, p, 0, 0);
            }
#endif
        }
    }
}

static int ocean_render_amplitude_at(game_s *g, i32 px_x)
{
    assert(0 <= px_x && (px_x >> 3) < g->ocean.surf.nparticles);
    int i1 = (px_x) >> 3;
    int i2 = (px_x + 7) >> 3;
    int h1 = water_amplitude(&g->ocean.surf, i1);
    int h2 = water_amplitude(&g->ocean.surf, i2);
    int u1 = px_x & 7;
    int u2 = 8 - u1;
    int oh = ocean_height(g, px_x);
    oh     = g->ocean.y;
    return oh + ((h1 * u2 + h2 * u1) >> 3);
}

void ocean_calc_spans(game_s *g, rec_i32 camr)
{
    ocean_s *oc = &g->ocean;
    if (!oc->active) {
        oc->y_min = camr.h;
        oc->y_max = camr.h;
        return;
    }

    if (inp_debug_space()) {
        water_impact(&oc->surf, 30, 6, 30000);
    }

    oc->y_min          = I32_MAX;
    oc->y_max          = I32_MIN;
    oc->n_spans        = 1;
    ocean_span_s *span = &oc->spans[0];
    span->y            = ocean_render_amplitude_at(g, 0 + camr.x) - camr.y;
    span->w            = 1;

    for (int x = 1; x < camr.w; x++) {
        i32 h     = ocean_render_amplitude_at(g, x + camr.x) - camr.y;
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