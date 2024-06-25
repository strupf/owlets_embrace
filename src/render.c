// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "app.h"
#include "game.h"
#include "obj/behaviour.h"

static inline i32 cmp_obj_render_priority(obj_s **a, obj_s **b)
{
    obj_s *x = *a;
    obj_s *y = *b;
    if (x->render_priority < y->render_priority) return -1;
    if (x->render_priority > y->render_priority) return +1;
    return 0;
}

SORT_ARRAY_DEF(obj_s *, obj_render, cmp_obj_render_priority)

static inline gfx_pattern_s water_pattern()
{
    return gfx_pattern_4x4(B4(0101),
                           B4(1010),
                           B4(0101),
                           B4(1010));
}

void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam);

void game_draw(game_s *g)
{
    if (g->substate == SUBSTATE_MENUSCREEN) {
        menu_screen_draw(g, &g->menu_screen);
        return;
    }

    rec_i32 camrec_raw    = cam_rec_px(g, &g->cam);
    v2_i32  camoffset_raw = {-camrec_raw.x, -camrec_raw.y};
    rec_i32 camrec        = camrec_raw;
    v2_i32  camoffsetmax  = cam_offset_max(g, &g->cam);
    assert(0 <= camrec.x && camrec.x <= camoffsetmax.x);
    assert(0 <= camrec.y && camrec.y <= camoffsetmax.y);

    camrec.x &= ~1;
    camrec.y &= ~1;

    v2_i32 camoffset = {-camrec.x, -camrec.y};

    tex_s             texdisplay = asset_tex(0);
    gfx_ctx_s         ctx        = gfx_ctx_default(texdisplay);
    ocean_s          *ocean      = &g->ocean;
    tile_map_bounds_s tilebounds = tile_map_bounds_rec(g, camrec);
    ocean_calc_spans(g, camrec);
    area_draw_bg(g, &g->area, camoffset_raw, camoffset);
    area_draw_mg(g, &g->area, camoffset_raw, camoffset);
    render_water_background(g, camoffset, tilebounds);
    render_tilemap(g, TILELAYER_BG, tilebounds, camoffset);
    render_tilemap(g, TILELAYER_PROP_BG, tilebounds, camoffset);

    deco_verlet_draw(g, camoffset);

    if (g->objrender_dirty) {
        g->objrender_dirty = 0;
        sort_obj_render(g->obj_render, g->n_objrender);
    }

    u32 n_obj_render = 0;
    for (; n_obj_render < g->n_objrender; n_obj_render++) {
        obj_s *o = g->obj_render[n_obj_render];
        if (0 <= o->render_priority) break;
        obj_draw(ctx, g, o, camoffset);
        if (o->on_draw) {
            o->on_draw(g, o, camoffset);
        }
    }

    obj_s  *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *rope  = ohero ? ohero->rope : NULL;
    obj_s  *ohook = ohero ? obj_from_obj_handle(g->hero_mem.hook) : NULL;
    if (rope && ohook) {
        v2_i32 ropepts[64];
        u32    n_ropepts = 0;

        for (u32 k = 1; k < ROPE_VERLET_N; k++) {
            v2_i32 p             = v2_shr(g->rope.ropept[k].p, 8);
            ropepts[n_ropepts++] = v2_add(p, camoffset);
        }

        gfx_ctx_s ctxwr = ctx;
        ctxwr.pat       = gfx_pattern_4x4(B4(0111),
                                          B4(1010),
                                          B4(1101),
                                          B4(1010));
        for (u32 k = 1; k < n_ropepts; k++) {
            v2_i32 p1 = ropepts[k - 1];
            v2_i32 p2 = ropepts[k];
            gfx_lin_thick(ctx, p1, p2, GFX_COL_WHITE, 8);
        }
        for (u32 k = 1; k < n_ropepts; k++) {
            v2_i32 p1 = ropepts[k - 1];
            v2_i32 p2 = ropepts[k];
            gfx_lin_thick(ctx, p1, p2, GFX_COL_BLACK, 7);
        }
        for (u32 k = 1; k < n_ropepts; k++) {
            v2_i32 p1 = ropepts[k - 1];
            v2_i32 p2 = ropepts[k];
            gfx_lin_thick(ctxwr, p1, p2, GFX_COL_WHITE, 3);
        }
    }

    particles_draw(g, &g->particles, camoffset);
    coinparticle_draw(g, camoffset);

    for (; n_obj_render < g->n_objrender; n_obj_render++) {
        obj_draw(ctx, g, g->obj_render[n_obj_render], camoffset);
    }

    grass_draw(g, camrec, camoffset);
    area_draw_mg(g, &g->area, camoffset_raw, camoffset);
    render_water_and_terrain(g, tilebounds, camoffset);
    render_tilemap(g, TILELAYER_PROP_FG, tilebounds, camoffset);
    area_draw_fg(g, &g->area, camoffset_raw, camoffset);

#if LIGHTING_ENABLED
#if 0
    {
        obj_s *ohero              = obj_get_tagged(g, OBJ_TAG_HERO);
        g->lighting.lights[0].p.x = obj_pos_center(ohero).x;
        g->lighting.lights[0].p.y = ohero->pos.y;
        g->lighting.lights[0].r   = 150;

        g->lighting.lights[1].p.x = 400;
        g->lighting.lights[1].p.y = 550;
        g->lighting.lights[1].r   = 150;

        g->lighting.lights[2].p.x = 800;
        g->lighting.lights[2].p.y = 400;
        g->lighting.lights[2].r   = 200;
        g->lighting.n_lights      = 3;
        lighting_shadowcast(g, &g->lighting, camoffset);
    }

#else

    static i32 ltick = 0;

    ltick++;
    if (ltick & 1) {
        lighting_refresh(g, &g->lighting);
        lighting_render(&g->lighting, camoffset);
    }
    lighting_apply_tex(&g->lighting);

#endif
#endif

    render_ui(g, camoffset);

    i32 breath_t = hero_breath_tick(g);
    if (breath_t) {
        spm_push();
        tex_s     drowntex  = tex_create_opaque(PLTF_DISPLAY_W, PLTF_DISPLAY_H, spm_allocator);
        gfx_ctx_s ctx_drown = gfx_ctx_default(drowntex);
        ctx_drown.pat       = gfx_pattern_interpolate(3, 4);

        v2_i32 herop = v2_add(camoffset, obj_pos_center(ohero));
        gfx_rec_fill(ctx_drown, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
        ctx_drown.pat = gfx_pattern_interpolate(1, 1);
        i32 breath_tm = hero_breath_tick_max(g);
        i32 cird      = ease_out_quad(700, 0, breath_t, breath_tm);
        gfx_cir_fill(ctx_drown, herop, cird, PRIM_MODE_WHITE);

        u32 N = PLTF_DISPLAY_H * PLTF_DISPLAY_WWORDS;
        for (u32 n = 0; n < N; n++) {
            ctx.dst.px[n] &= drowntex.px[n];
        }
        spm_pop();
    }

    switch (g->substate) {
    case 0: break;
    case SUBSTATE_TEXTBOX:
        textbox_draw(g, (v2_i32){0});
        break;
    case SUBSTATE_GAMEOVER:
        gameover_draw(g, camoffset);
        break;
    case SUBSTATE_MAPTRANSITION:
        maptransition_draw(g, camoffset);
        break;
    }
}

void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32 ppos = v2_add(o->pos, cam);

    if (o->flags & OBJ_FLAG_SPRITE) {
        for (i32 n = 0; n < o->n_sprites; n++) {
            obj_sprite_s sprite = o->sprites[n];
            if (sprite.trec.t.px == NULL) continue;

            v2_i32 sprpos = v2_add(ppos, v2_i32_from_i16(sprite.offs));
            i32    mode   = 0;
            if ((o->flags & OBJ_FLAG_ENEMY) && ((o->enemy.hurt_tick >> 2) & 1)) {
                mode = SPR_MODE_INV;
            }
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, mode);

            // player low health blinking
            if (o->ID == OBJ_ID_HERO && o->health == 1) {
                gfx_ctx_s cs = ctx;
                i32       s  = (sin_q16(g->gameplay_tick << 13) + 65536) >> 1;
                cs.pat       = gfx_pattern_interpolate(s, 65536 * 3);
                gfx_spr(cs, sprite.trec, sprpos, sprite.flip, SPR_MODE_BLACK);
            }
        }
    }

    if (o->on_draw) {
        o->on_draw(g, o, cam);
    }

#ifdef PLTF_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_ctx_s ctx_aabb = ctx;
        ctx_aabb.pat       = gfx_pattern_interpolate(1, 4);

        rec_i32 aabb = {ppos.x, ppos.y, o->w, o->h};
        rec_i32 rr2  = aabb;
        rr2.x += 3;
        rr2.y += 3;
        rr2.w -= 6;
        rr2.h -= 6;
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
        gfx_rec_fill(ctx_aabb, rr2, PRIM_MODE_BLACK_WHITE);
    }
#endif
}

void render_tilemap(game_s *g, int layer, tile_map_bounds_s bounds, v2_i32 camoffset)
{
    tex_s tex = {0};
    switch (layer) {
    case TILELAYER_BG:
        tex = asset_tex(TEXID_TILESET_TERRAIN);
        break;
    case TILELAYER_PROP_BG:
        tex = asset_tex(TEXID_TILESET_PROPS_BG);
        break;
    case TILELAYER_PROP_FG:
        tex = asset_tex(TEXID_TILESET_PROPS_FG);
        break;
    default: return;
    }
    gfx_ctx_s ctx = gfx_ctx_display();

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            rtile_s rt = g->rtiles[layer][x + y * g->tiles_x];
            if (rt.u == 0) continue;
            v2_i32   p   = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            texrec_s trr = {tex, {rt.tx << 4, rt.ty << 4, 16, 16}};
            gfx_spr_tile_32x32(ctx, trr, p);
        }
    }
}

int water_render_height(game_s *g, int pixel_x)
{
    i32 p = pixel_x;
    i32 t = g->gameplay_tick;
    i32 y = (sin_q6((p << 2) + (t << 4) + 0x20) << 1) +
            (sin_q6((p << 4) - (t << 5) + 0x04) << 0) +
            (sin_q6((p << 5) + (t << 6) + 0x10) >> 2);
    return (y >> 6);
}

void ocean_calc_spans(game_s *g, rec_i32 camr)
{
    ocean_s *oc = &g->ocean;
    if (!oc->active) {
        oc->y_min = camr.h;
        oc->y_max = camr.h;
        return;
    }

    oc->y_min          = I32_MAX;
    oc->y_max          = I32_MIN;
    oc->n_spans        = 1;
    ocean_span_s *span = &oc->spans[0];
    span->y            = ocean_render_height(g, 0 + camr.x) - camr.y;
    span->w            = 1;

    for (i32 x = 1; x < camr.w; x++) {
        i32 xpos = x + camr.x;
        i32 h    = ocean_render_height(g, x + camr.x) - camr.y;

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

void render_water_and_terrain(game_s *g, tile_map_bounds_s bounds, v2_i32 camoffset)
{
    const tex_s     tset = asset_tex(TEXID_TILESET_TERRAIN);
    const tex_s     twat = asset_tex(TEXID_WATER_PRERENDER);
    const gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s       ctxw = ctx;
    ctxw.pat             = water_pattern();
    i32 tick             = g->gameplay_tick;

#if 0
    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (rt.U == 0) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            if (rt.type & TILE_WATER_MASK) {
                i32 j = i - g->tiles_x;
                if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                    i32      watert = water_tile_get(x, y, tick);
                    texrec_s trw    = {twat, {16, watert << 4, 16, 16}};
                    gfx_spr_tile_32x32(ctxw, trw, p);
                } else {
                    gfx_rec_fill(ctxw, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
                }
            }

            if (rt.u == 0) continue;
            v2_i32   tp  = {p.x - 8, p.y - 8};
            texrec_s trt = {tset, {rt.tx << 5, rt.ty << 5, 32, 32}};
            gfx_spr_tile_32x32(ctx, trt, tp);
#if defined(SYS_DEBUG) && 0
            int t1 = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t1 && t1 < NUM_TILE_SHAPES)) continue;
            texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
            gfx_spr(ctx, tr1, p, 0, 0);
#endif
        }
    }
#else
    // water tiles
    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (rt.U == 0) continue;
            if (!(rt.type & TILE_WATER_MASK)) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            i32    j = i - g->tiles_x;
            if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                i32      watert = water_tile_get(x, y, tick);
                texrec_s trw    = {twat, {16, watert << 4, 16, 16}};
                gfx_spr_tile_32x32(ctxw, trw, p);
            } else {
                gfx_rec_fill(ctxw, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
            }
        }
    }

    for (i32 pass = 0; pass < 15; pass++) {
        for (i32 y = bounds.y1; y <= bounds.y2; y++) {
            for (i32 x = bounds.x1; x <= bounds.x2; x++) {
                tile_s rt = g->tiles[x + y * g->tiles_x];
                i32    tt = rt.type & 15;
                if (tt + 1 != pass) continue;
                if (rt.u == 0) continue;
                v2_i32   p   = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
                v2_i32   tp  = {p.x - 8, p.y - 8};
                texrec_s trt = {tset, {rt.tx << 5, rt.ty << 5, 32, 32}};
                gfx_spr_tile_32x32(ctx, trt, tp);
#if defined(SYS_DEBUG) && 0
                int t1 = g->tiles[x + y * g->tiles_x].collision;
                if (!(0 < t1 && t1 < NUM_TILE_SHAPES)) continue;
                texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
                gfx_spr(ctx, tr1, p, 0, 0);
#endif
            }
        }
    }
#endif

    ocean_s *oc = &g->ocean;
    if (oc->active) {
        gfx_ctx_s ctxf = ctx;
        gfx_ctx_s ctxl = ctx;

        ctxf.pat  = water_pattern();
        ctxl.pat  = gfx_pattern_interpolate(1, 6);
        i32 y_max = clamp_i(oc->y_max, 0, ctx.dst.h - 1);

        // fill "static" bottom section
        u32 *px = &((u32 *)ctx.dst.px)[y_max * ctx.dst.wword];
        for (i32 y = y_max; y < ctx.dst.h; y++) {
            u32 pt = ~ctxf.pat.p[y & 7];
            for (i32 x = 0; x < ctx.dst.wword; x++)
                *px++ &= pt;
        }

        //  dynamic sin section
        for (i32 k = 0, x = 0; k < oc->n_spans; k++) {
            ocean_span_s sp  = oc->spans[k];
            rec_i32      rl1 = {x, sp.y + 2, sp.w, 2};
            rec_i32      rl2 = {x, sp.y + 4, sp.w, 2};
            rec_i32      rfb = {x, sp.y, sp.w, y_max - sp.y};
            gfx_rec_fill(ctxf, rfb, PRIM_MODE_BLACK);
            gfx_rec_fill(ctxf, rl1, PRIM_MODE_WHITE);
            gfx_rec_fill(ctxl, rl2, PRIM_MODE_WHITE);
            x += sp.w;
        }
    }
}

void render_water_background(game_s *g, v2_i32 camoff, tile_map_bounds_s bounds)
{
    const tex_s twat = asset_tex(TEXID_WATER_PRERENDER);
    i32         tick = g->gameplay_tick;
    gfx_ctx_s   ctx  = gfx_ctx_display();
    tex_s       t    = ctx.dst;

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (!(rt.type & TILE_WATER_MASK)) continue;
            v2_i32 p = {(x << 4) + camoff.x, (y << 4) + camoff.y};
            i32    j = i - g->tiles_x;
            if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                i32      watert = water_tile_get(x, y, tick);
                texrec_s trw    = {twat, {0, watert << 4, 16, 16}};
                gfx_spr_tile_32x32(ctx, trw, p);
            } else {
                gfx_rec_fill(ctx, (rec_i32){p.x, p.y, 16, 16}, GFX_COL_BLACK);
            }
        }
    }

    ocean_s *oc = &g->ocean;
    if (oc->active) {
        i32 y_max = clamp_i(oc->y_max, 0, t.h);

        for (i32 k = 0, x = 0; k < oc->n_spans; k++) {
            ocean_span_s sp = oc->spans[k];
            rec_i32      rf = {x, sp.y, sp.w, y_max - sp.y};

            gfx_rec_fill(ctx, rf, GFX_COL_BLACK);
            x += sp.w;
        }

        u32 *px = &((u32 *)t.px)[y_max * t.wword];
        i32  N  = t.wword * (t.h - y_max);
        for (i32 n = 0; n < N; n++) {
            *px++ = 0;
        }
    }
}

void render_rec_as_terrain(gfx_ctx_s ctx, rec_i32 r, int terrain)
{
    assert((r.w & 15) == 0 && (r.h & 15) == 0);
    i32 nx = r.w >> 4;
    i32 ny = r.h >> 4;

    texrec_s tr = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 16, 16);
    for (i32 y = 0; y < ny; y++) {
        for (i32 x = 0; x < nx; x++) {
            if (ny == 1 && nx == 1) { // single block
                tr.r.x = 7 * 16;
                tr.r.y = 1 * 16;
            } else if (ny == 1) { // horizontal strip
                if (x == 0) {

                } else if (x == nx - 1) {
                } else {
                }
            } else if (nx == 1) { // vertical strip
                if (y == 0) {

                } else if (y == ny - 1) {
                } else {
                }
            } else if (y == 0) { // top row
                if (x == 0) {

                } else if (x == nx - 1) {

                } else {
                }
            } else if (y == ny - 1) { // bottom row
                if (x == 0) {

                } else if (x == nx - 1) {

                } else {
                }
            } else { // middle row
                if (x == 0) {

                } else if (x == nx - 1) {

                } else {
                }
            }

            v2_i32 pos = {0};
            gfx_spr(ctx, tr, pos, 0, 0);
        }
    }
}
