// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"
#include "obj/behaviour.h"

static inline gfx_pattern_s water_pattern()
{
    return gfx_pattern_4x4(B4(0101),
                           B4(1010),
                           B4(0101),
                           B4(1010));
}

static void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam);

static int cmp_obj_render_priority(obj_s **a, obj_s **b)
{
    obj_s *x = *a;
    obj_s *y = *b;
    if (x->render_priority < y->render_priority) return -1;
    if (x->render_priority > y->render_priority) return +1;
    return 0;
}

SORT_ARRAY_DEF(obj_s *, obj_render, cmp_obj_render_priority)

void game_draw(game_s *g)
{
    if (g->substate == SUBSTATE_MENUSCREEN) {
        menu_screen_draw(g, &g->menu_screen);
        return;
    }

    rec_i32 camrec_raw    = cam_rec_px(g, &g->cam);
    v2_i32  camoffset_raw = {-camrec_raw.x, -camrec_raw.y};
    rec_i32 camrec        = camrec_raw;

#ifdef SYS_DEBUG
    v2_i32 camoffsetmax = cam_offset_max(g, &g->cam);
    assert(0 <= camrec.x && camrec.x <= camoffsetmax.x);
    assert(0 <= camrec.y && camrec.y <= camoffsetmax.y);
#endif
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

    for (int n = 0; n < g->n_wiggle_deco; n++) {
        wiggle_deco_s *wd     = &g->wiggle_deco[n];
        v2_i32         wd_pos = {wd->r.x + wd->offs.x, wd->r.y + wd->offs.y};
        gfx_spr(ctx, wd->tr, v2_add(wd_pos, camoffset), 0, 0);
    }

    if (g->objrender_dirty) {
        g->objrender_dirty = 0;
#if 0
        sort_array(g->obj_render, g->n_objrender, sizeof(obj_s *),
                   cmp_obj_render_priority);
#else
        sort_obj_render(g->obj_render, g->n_objrender);
#endif
    }

    int n_obj_render = 0;
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
    obj_s  *ohook = ohero ? obj_from_obj_handle(ohero->obj_handles[0]) : NULL;
    if (rope && ohook && ohook->substate) {
        v2_i32 ropepts[64];
        int    n_ropepts = 0;

        if (rope->len_max_q4 <= rope_length_q4(g, rope) && 0) {
            for (ropenode_s *a = rope->head; a; a = a->next) {
                ropepts[n_ropepts++] = v2_add(a->p, camoffset);
            }
        } else {
            for (int k = 1; k < ROPE_VERLET_N; k++) {
                v2_i32 p             = v2_shr(g->hero_mem.hookpt[k].p, 8);
                ropepts[n_ropepts++] = v2_add(p, camoffset);
            }
        }

        for (int k = 1; k < n_ropepts; k++) {
            v2_i32 p1 = ropepts[k - 1];
            v2_i32 p2 = ropepts[k];
            gfx_lin_thick(ctx, p1, p2, PRIM_MODE_BLACK, 6);
        }
        for (int k = 1; k < n_ropepts; k++) {
            v2_i32 p1 = ropepts[k - 1];
            v2_i32 p2 = ropepts[k];
            gfx_lin_thick(ctx, p1, p2, PRIM_MODE_WHITE, 1);
        }
    }

    // enemy death animations
    for (int n = 0; n < g->n_enemy_decals; n++) {
        texrec_s tr   = g->enemy_decals[n].t;
        int      i0   = g->enemy_decals[n].tick;
        v2_i32   dpos = v2_add(g->enemy_decals[n].pos, camoffset);

        for (int y = 0; y < tr.r.h; y += 2) {
            if (rngr_i32(0, (100 * i0) / ENEMY_DECAL_TICK) <= 4) continue;
            v2_i32   ppp = {dpos.x, dpos.y + y};
            texrec_s trr = tr;
            trr.r.y      = tr.r.y + y;
            trr.r.h      = 2;
            gfx_spr(ctx, trr, ppp, 0, SPR_MODE_BLACK);
        }
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

    coinparticle_draw(g, camoffset);

    for (; n_obj_render < g->n_objrender; n_obj_render++) {
        obj_s *o = g->obj_render[n_obj_render];
        obj_draw(ctx, g, o, camoffset);
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
            rec_i32 rg = {224 + 8, i + gr->type * 16, 16, 1};
            trgrass.r  = rg;
            gfx_spr(ctx, trgrass, p, 0, 0);
        }
    }

    area_draw_mg(g, &g->area, camoffset_raw, camoffset);
    render_water_and_terrain(g, tilebounds, camoffset);
    render_tilemap(g, TILELAYER_PROP_FG, tilebounds, camoffset);
    area_draw_fg(g, &g->area, camoffset_raw, camoffset);

    for (obj_each(g, o)) {
        if (!((o->flags & OBJ_FLAG_HOVER_TEXT) && o->hover_text_tick)) continue;

        v2_i32    ppos     = v2_add(o->pos, camoffset);
        fnt_s     font     = asset_fnt(FNTID_SMALL);
        gfx_ctx_s ctx_text = ctx;
        ctx_text.pat       = gfx_pattern_interpolate(o->hover_text_tick, OBJ_HOVER_TEXT_TICKS);

        i32 he = 0;
        i32 wi = 0;
        for (i32 n = 0; n < 2; n++) {
            if (o->hover_text[n][0] != '\0') {
                wi = max_i32(wi, fnt_length_px(font, o->hover_text[n]));
                he += 16;
            }
        }

        wi                = max_i32(((wi + 48) & ~15) >> 4, 3);
        he                = max_i32(((he + 16) & ~15) >> 4, 3);
        v2_i32   pos_text = {ppos.x - 30, ppos.y - 70};
        texrec_s tr_tile  = asset_texrec(TEXID_UI, 368, 64, 16, 16);

        for (i32 y = 0; y < he; y++) {
            for (i32 x = 0; x < wi; x++) {
                if (x == 0) tr_tile.r.x = 368;
                else if (x == wi - 1) tr_tile.r.x = 368 + 2 * 16;
                else tr_tile.r.x = 368 + 1 * 16;

                if (y == 0) tr_tile.r.y = 64;
                else if (y == he - 1) tr_tile.r.y = 64 + 2 * 16;
                else tr_tile.r.y = 64 + 1 * 16;
                v2_i32 tilepos = {pos_text.x + x * 16, pos_text.y + y * 16};
                gfx_spr(ctx_text, tr_tile, tilepos, 0, 0);
            }
        }

        for (i32 n = 0; n < 2; n++) {
            v2_i32 post = {pos_text.x + 15, pos_text.y + 10 + n * 16};
            fnt_draw_ascii(ctx_text, font, post, o->hover_text[n], GFX_COL_BLACK);
        }
    }

    render_ui(g, camoffset);

    i32 breath_t = hero_breath_tick(g);
    if (breath_t) {
        spm_push();
        tex_s     drowntex  = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, spm_allocator);
        gfx_ctx_s ctx_drown = gfx_ctx_default(drowntex);
        ctx_drown.pat       = gfx_pattern_interpolate(3, 4);

        v2_i32 herop = v2_add(camoffset, obj_pos_center(ohero));
        gfx_rec_fill(ctx_drown, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
        ctx_drown.pat = gfx_pattern_interpolate(1, 1);
        i32 breath_tm = hero_breath_tick_max(g);
        int cird      = ease_out_quad(700, 0, breath_t, breath_tm);
        gfx_cir_fill(ctx_drown, herop, cird, PRIM_MODE_WHITE);

        int N = SYS_DISPLAY_H * SYS_DISPLAY_WWORDS;
        for (int n = 0; n < N; n++) {
            ctx.dst.px[n] &= drowntex.px[n];
        }
        spm_pop();
    }

    switch (g->substate) {
    case SUBSTATE_GAMEOVER: gameover_draw(g, camoffset); break;
    case SUBSTATE_TEXTBOX: textbox_draw(g, camoffset); break;
    case SUBSTATE_MAPTRANSITION: maptransition_draw(g, camoffset); break;
    case SUBSTATE_HEROUPGRADE: heroupgrade_draw(g, camoffset); break;
    }
    item_select_draw(&g->item_select);
#if LIGHTING_ENABLED
    g->lighting.lights[0].p = ohero->pos;
    g->lighting.lights[0].r = 150;
    lighting_do(g, &g->lighting, camoffset);
#endif
}

static void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32  ppos = v2_add(o->pos, cam);
    rec_i32 aabb = {ppos.x, ppos.y, o->w, o->h};

#ifdef SYS_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
    }
#endif

    if (o->flags & OBJ_FLAG_SPRITE) {
        for (int n = 0; n < o->n_sprites; n++) {
            obj_sprite_s sprite = o->sprites[n];
            if (sprite.trec.t.px == NULL) continue;
            v2_i32 sprpos = v2_add(ppos, sprite.offs);
            int    mode   = sprite.mode;
            if ((o->flags & OBJ_FLAG_ENEMY) &&
                ((o->invincible_tick >> 2) & 1)) {
                mode = SPR_MODE_INV;
            }
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, mode);
        }
    }

    switch (o->ID) {
    case OBJ_ID_TOGGLEBLOCK: toggleblock_on_draw(g, o, cam); break;
    case OBJ_ID_CRUMBLEBLOCK: crumbleblock_on_draw(g, o, cam); break;
    case OBJ_ID_HEROUPGRADE: heroupgrade_on_draw(g, o, cam); break;
    case OBJ_ID_SPIKES: spikes_on_draw(g, o, cam); break;
    }

#ifdef SYS_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_ctx_s ctx_aabb = ctx;
        ctx_aabb.pat       = gfx_pattern_interpolate(1, 4);

        rec_i32 rr2 = aabb;
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

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            rtile_s rt = g->rtiles[layer][x + y * g->tiles_x];
            if (rt.u == 0) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            gfx_spr_tile(ctx, tex, rt.tx, rt.ty, 4, p);
        }
    }
}

int water_render_height(game_s *g, int pixel_x)
{
    int p = pixel_x;
    int t = sys_tick();
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

    for (int x = 1; x < camr.w; x++) {
        int xpos = x + camr.x;
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
    int tick             = sys_tick();

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (rt.U == 0) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            if (rt.type & TILE_WATER_MASK) {
                int j = i - g->tiles_x;
                if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                    gfx_spr_tile(ctxw, twat, 1, water_tile_get(x, y, tick), 4, p);
                } else {
                    gfx_rec_fill(ctxw, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
                }
            }

            if (rt.u == 0) continue;
            v2_i32 tp = {p.x - 8, p.y - 8};
            gfx_spr_tile(ctx, tset, rt.tx, rt.ty, 5, tp);
#if defined(SYS_DEBUG) && 0
            int t1 = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t1 && t1 < NUM_TILE_SHAPES)) continue;
            texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
            gfx_spr(ctx, tr1, p, 0, 0);
#endif
        }
    }

    ocean_s *oc = &g->ocean;
    if (oc->active) {
        gfx_ctx_s ctxf = ctx;
        gfx_ctx_s ctxl = ctx;

        ctxf.pat  = water_pattern();
        ctxl.pat  = gfx_pattern_interpolate(1, 6);
        int y_max = clamp_i(oc->y_max, 0, ctx.dst.h - 1);

        // fill "static" bottom section
        u32 *px = &((u32 *)ctx.dst.px)[y_max * ctx.dst.wword];
        for (int y = y_max; y < ctx.dst.h; y++) {
            u32 pt = ~ctxf.pat.p[y & 7];
            for (int x = 0; x < ctx.dst.wword; x++)
                *px++ &= pt;
        }

        //  dynamic sin section
        for (int k = 0, x = 0; k < oc->n_spans; k++) {
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
    int         tick = sys_tick();
    gfx_ctx_s   ctx  = gfx_ctx_display();
    tex_s       t    = ctx.dst;

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (!(rt.type & TILE_WATER_MASK)) continue;
            v2_i32 p = {(x << 4) + camoff.x, (y << 4) + camoff.y};
            int    j = i - g->tiles_x;
            if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                gfx_spr_tile(ctx, twat, 0, water_tile_get(x, y, tick), 4, p);
            } else {
                gfx_rec_fill(ctx, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
            }
        }
    }

    ocean_s *oc = &g->ocean;
    if (oc->active) {
        int y_max = clamp_i(oc->y_max, 0, t.h);

        for (int k = 0, x = 0; k < oc->n_spans; k++) {
            ocean_span_s sp = oc->spans[k];
            rec_i32      rf = {x, sp.y, sp.w, y_max - sp.y};

            gfx_rec_fill(ctx, rf, PRIM_MODE_BLACK);
            x += sp.w;
        }

        u32 *px = &((u32 *)t.px)[y_max * t.wword];
        int  N  = t.wword * (t.h - y_max);
        for (int n = 0; n < N; n++) {
            *px++ = 0;
        }
    }
}

void render_rec_as_terrain(gfx_ctx_s ctx, rec_i32 r, int terrain)
{
    assert((r.w & 15) == 0 && (r.h & 15) == 0);
    int nx = r.w >> 4;
    int ny = r.h >> 4;

    texrec_s tr = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 16, 16);
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
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
