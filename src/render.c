// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "app.h"
#include "game.h"

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
    foreground_props_draw(g, camoffset);

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

    g->cam.prev_gfx_offs = camoffset;
}

void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam)
{
    if (o->on_draw_pre) {
        o->on_draw_pre(g, o, cam);
    }

    v2_i32 ppos = v2_add(o->pos, cam);
    if (o->enemy.hurt_tick) {
        ppos.x += o->enemy.hurt_shake_offs.x;
        ppos.y += o->enemy.hurt_shake_offs.y;
    }

    if (o->flags & OBJ_FLAG_SPRITE) {
        for (i32 n = 0; n < o->n_sprites; n++) {
            obj_sprite_s sprite = o->sprites[n];
            if (sprite.trec.t.px == NULL) continue;

            v2_i32 sprpos = v2_add(ppos, v2_i32_from_i16(sprite.offs));
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, 0);

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
        tex = asset_tex(TEXID_TILESET_BG_AUTO);
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

typedef struct {
    u16 z;
    u8  tx;
    u8  ty;
    i16 x;
    i16 y;
} tile_spr_s;

static inline i32 cmp_tile_spr(tile_spr_s *a, tile_spr_s *b)
{
    return ((i32)a->z - (i32)b->z);
}

SORT_ARRAY_DEF(tile_spr_s, z_tile_spr, cmp_tile_spr)

void render_water_and_terrain(game_s *g, tile_map_bounds_s bounds, v2_i32 camoffset)
{
    const tex_s     tset = asset_tex(TEXID_TILESET_TERRAIN);
    const tex_s     twat = asset_tex(TEXID_WATER_PRERENDER);
    const gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s       ctxw = ctx;
    ctxw.pat             = water_pattern();
    i32 tick             = g->gameplay_tick;

    spm_push();

    u32         n_tile_spr = 0;
    tile_spr_s *tile_spr   = spm_alloct(tile_spr_s, 512);

    // water tiles
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
                    rec_i32 recw = {p.x, p.y, 16, 16};
                    gfx_rec_fill(ctxw, recw, GFX_COL_BLACK);
                }
            }

            if (rt.u == 0) continue;

            // draw terrain tiles sorted by type, insert in buffer first
            i32        tt          = rt.type & 31;
            tile_spr_s sp          = {tt, rt.tx, rt.ty, p.x, p.y};
            tile_spr[n_tile_spr++] = sp;
#if defined(PLTF_DEBUG) && 0
            i32 t1 = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t1 && t1 < NUM_TILE_SHAPES)) continue;
            texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
            gfx_spr(ctx, tr1, p, 0, 0);
#endif
        }
    }

    sort_z_tile_spr(tile_spr, n_tile_spr);
    for (u32 n = 0; n < n_tile_spr; n++) {
        tile_spr_s sp   = tile_spr[n];
        texrec_s   trec = {tset, {sp.tx << 5, sp.ty << 5, 32, 32}};
        v2_i32     pos  = {sp.x - 8, sp.y - 8};
        gfx_spr_tile_32x32(ctx, trec, pos);
    }
    spm_pop();

    ocean_s *oc = &g->ocean;
    if (oc->active) {
        gfx_ctx_s ctxf = ctx;
        gfx_ctx_s ctxl = ctx;

        ctxf.pat  = water_pattern();
        ctxl.pat  = gfx_pattern_interpolate(1, 6);
        i32 y_max = clamp_i32(oc->y_max, 0, ctx.dst.h - 1);

        // fill "static" bottom section
        u32 *px = &ctx.dst.px[y_max * ctx.dst.wword];
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
        i32 y_max = clamp_i32(oc->y_max, 0, t.h);

        for (i32 k = 0, x = 0; k < oc->n_spans; k++) {
            ocean_span_s sp = oc->spans[k];
            rec_i32      rf = {x, sp.y, sp.w, y_max - sp.y};

            gfx_rec_fill(ctx, rf, GFX_COL_BLACK);
            x += sp.w;
        }

        u32 *px = &t.px[y_max * t.wword];
        i32  N  = t.wword * (t.h - y_max);
        for (i32 n = 0; n < N; n++) {
            *px++ = 0;
        }
    }
}

// renders a block of tiles dressed up as tile_type
void render_tile_terrain_block(gfx_ctx_s ctx, v2_i32 pos, i32 tx, i32 ty, i32 tile_type)
{
    if (tile_type < TILE_TYPE_DEBUG) return;

    // check if visible at all
    rec_i32 visible_geometry = {pos.x - 8,
                                pos.y - 8,
                                (tx << 5) + 8,
                                (ty << 5) + 8};
    rec_i32 rtarget          = {ctx.clip_x1,
                                ctx.clip_y1,
                                ctx.clip_x2 - ctx.clip_x1 + 1,
                                ctx.clip_y2 - ctx.clip_y1 + 1};
    if (!overlap_rec(visible_geometry, rtarget)) return;

    texrec_s tr = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 32, 32);

    for (i32 y = 0; y < ty; y++) {
        for (i32 x = 0; x < tx; x++) {
            u32 tilex = 0;
            u32 tiley = 0;

            if (0) {
            } else if (tx == 1 && ty == 1) { // 1-tile block
                tilex = 7, tiley = 1;
            } else if (tx == 1 && 1 < ty) { // vertical strip
                if (0) {
                } else if (y == 0) { // left
                    tilex = 0, tiley = 1;
                } else if (y == ty - 1) { // right
                    tilex = 0, tiley = 7;
                } else { // middle
                    tilex = 7, tiley = 3;
                }
            } else if (ty == 1 && 1 < tx) { // horizontal strip
                if (0) {
                } else if (x == 0) { // top
                    tilex = 1, tiley = 0;
                } else if (x == tx - 1) { // bottom
                    tilex = 7, tiley = 0;
                } else { // middle
                    tilex = 4, tiley = 7;
                }
            } else { // big block
                if (0) {
                } else if (x == 0 && y == 0) { // corner top left
                    tilex = 5, tiley = 5;
                } else if (x == 0 && y == ty - 1) { // corner bottom left
                    tilex = 3, tiley = 4;
                } else if (x == tx - 1 && y == 0) { // corner top right
                    tilex = 4, tiley = 3;
                } else if (x == tx - 1 && y == ty - 1) { // corner bottom right
                    tilex = 6, tiley = 6;
                } else if (x == 0) { // left
                    tilex = 0, tiley = 4;
                } else if (y == 0) { // top
                    tilex = 4, tiley = 0;
                } else if (x == tx - 1) { // right
                    tilex = 2, tiley = 4;
                } else if (y == ty - 1) { // bottom
                    tilex = 1, tiley = 6;
                } else { // middle
                    tilex = 4, tiley = 1;
                }
            }

            tr.r.x = (tilex << 5);
            tr.r.y = (tiley << 5) + ((tile_type - 2) << 8);

            v2_i32 p = {pos.x + (x << 4) - 8, pos.y + (y << 4) - 8};
            gfx_spr(ctx, tr, p, 0, 0);
        }
    }
}

// cam is the top left corner but the center of the
// view area is needed
v2_i32 parallax_offs(v2_i32 cam, v2_i32 pos, i32 x_q8, i32 y_q8)
{
    i32    cx = pos.x - (-cam.x + (PLTF_DISPLAY_W >> 1));
    i32    cy = pos.y - (-cam.y + (PLTF_DISPLAY_H >> 1));
    v2_i32 p  = {pos.x + ((cx * x_q8) >> 8),
                 pos.y + ((cy * y_q8) >> 8)};
    return p;
}