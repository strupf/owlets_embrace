// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"

static inline gfx_pattern_s water_pattern()
{
    return gfx_pattern_4x4(B4(1111),
                           B4(1010),
                           B4(1111),
                           B4(1010));
}

static void obj_draw(gfx_ctx_s ctx, game_s *g, obj_s *o, v2_i32 cam);

int cmp_obj_render_priority(const void *a, const void *b)
{
    const obj_s *x = *(const obj_s **)a;
    const obj_s *y = *(const obj_s **)b;
    if (x->render_priority < y->render_priority) return -1;
    if (x->render_priority > y->render_priority) return +1;
    return 0;
}

void game_draw(game_s *g)
{
    const rec_i32   camrec    = cam_rec_px(g, &g->cam);
    const v2_i32    camoffset = {-camrec.x, -camrec.y};
    const gfx_ctx_s ctx       = gfx_ctx_display();

    ocean_s *ocean = &g->ocean;

    bounds_2D_s tilebounds = game_tilebounds_rec(g, camrec);
    render_bg(g, camrec, tilebounds);
    render_tilemap(g, TILELAYER_BG, tilebounds, camoffset);
    render_tilemap(g, TILELAYER_PROP_BG, tilebounds, camoffset);

    if (g->objrender_dirty) {
        g->objrender_dirty = 0;
        sort_array(g->obj_render, g->n_objrender, sizeof(obj_s *), cmp_obj_render_priority);
    }

    int n_obj_render = 0;
    for (; n_obj_render < g->n_objrender; n_obj_render++) {
        obj_s *o = g->obj_render[n_obj_render];
        if (0 <= o->render_priority) break;
        obj_draw(ctx, g, o, camoffset);
    }

    // texrec_s tr_charge = asset_texrec(TEXID_MISCOBJ, 336, 16, 80, 64);
    // gfx_spr(ctx, tr_charge, v2_add(camoffset, (v2_i32){180, 96 + 32}), 0, 0);

    obj_s  *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *rope  = ohero ? ohero->rope : NULL;
    if (rope) {
        v2_i32 ropepts[64];
        int    n_ropepts = 0;

        if (rope->len_max_q4 <= rope_length_q4(g, rope)) {
            for (ropenode_s *a = rope->head; a; a = a->next) {
                ropepts[n_ropepts++] = v2_add(a->p, camoffset);
            }
        } else {
            for (int k = 1; k < ROPE_VERLET_N; k++) {
                v2_i32 p             = v2_shr(g->herodata.hookpt[k].p, 8);
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

    collectibles_draw(g, camoffset);

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

    if (g->env_effects & ENVEFFECT_RAIN) {
        enveffect_rain_draw(ctx, &g->env_rain, camoffset);
    }

    render_terraintiles(g, tilebounds, camoffset);
    ocean_draw(g, asset_tex(0), camoffset);
    render_tilemap(g, TILELAYER_PROP_FG, tilebounds, camoffset);

    if (g->env_effects & ENVEFFECT_WIND)
        enveffect_wind_draw(ctx, &g->env_wind, camoffset);
    if (g->env_effects & ENVEFFECT_HEAT)
        enveffect_heat_draw(ctx, &g->env_heat, camoffset);

    render_ui(g, camoffset);
    transition_draw(g, &g->transition, camoffset);

    if (g->mainmenu_fade_in) {
        gfx_ctx_s ctxfade = ctx;
        ctxfade.pat       = gfx_pattern_interpolate(g->mainmenu_fade_in, FADETICKS_GAME_IN);
        gfx_rec_fill(ctxfade, (rec_i32){0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H}, PRIM_MODE_BLACK);
    }

    if (upgradehandler_in_progress(&g->heroupgrade)) {
        upgradehandler_draw(g, &g->heroupgrade, camoffset);
    }
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
            sprite_simple_s sprite = o->sprites[n];
            if (sprite.trec.t.px == NULL) continue;
            v2_i32 sprpos = v2_add(ppos, sprite.offs);
            int    mode   = sprite.mode;
            if ((o->flags & OBJ_FLAG_ENEMY) &&
                ((o->enemy.invincible >> 2) & 1)) {
                mode = SPR_MODE_INV;
            }
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, mode);
        }
    }

    switch (o->ID) {
    case OBJ_ID_TOGGLEBLOCK:
        toggleblock_on_draw(g, o, cam);
        break;
    case OBJ_ID_CRUMBLEBLOCK:
        crumbleblock_on_draw(g, o, cam);
        break;
    case OBJ_ID_HEROUPGRADE:
        heroupgrade_on_draw(g, o, cam);
        break;
    }

#ifdef SYS_DEBUG
    if (o->flags & OBJ_FLAG_RENDER_AABB) {
        gfx_ctx_s ctx_aabb = ctx;
        ctx_aabb.pat       = gfx_pattern_interpolate(1, 4);

        rec_i32 rr2 = aabb;
        rr2.x += 3;
        rr2.w -= 6;
        rr2.y += 3;
        rr2.h -= 6;
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
        gfx_rec_fill(ctx_aabb, rr2, PRIM_MODE_BLACK_WHITE);
    }
#endif
}

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 camoffset)
{
    texrec_s tr = {0};
    switch (layer) {
    case TILELAYER_BG:
        tr.t = asset_tex(TEXID_TILESET_TERRAIN);
        break;
    case TILELAYER_PROP_BG:
        tr.t = asset_tex(TEXID_TILESET_PROPS_BG);
        break;
    case TILELAYER_PROP_FG:
        tr.t = asset_tex(TEXID_TILESET_PROPS_FG);
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
            gfx_spr(ctx, tr, p, 0, 0);
        }
    }
}

void render_terraintiles(game_s *g, bounds_2D_s bounds, v2_i32 camoffset)
{
    texrec_s  tr   = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 16, 16);
    texrec_s  tw   = asset_texrec(TEXID_WATER_PRERENDER, 16, 0, 16, 16);
    gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s ctxw = ctx;
    ctxw.pat       = gfx_pattern_2x2(B2(11),
                                     B2(10));
    int tick       = sys_tick();

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (rt.U == 0) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            if (rt.type & TILE_WATER_MASK) {
                int j = i - g->tiles_x;
                if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                    tw.r.y = water_tile_get(x, y, tick) << 4;
                    gfx_spr(ctxw, tw, p, 0, 0);
                } else {
                    gfx_rec_fill(ctxw, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
                }
            }

            if (rt.u == 0) continue;
            tr.r.x = rt.tx << 4;
            tr.r.y = rt.ty << 4;
            gfx_spr(ctx, tr, p, 0, 0);
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

    ctxf.pat = water_pattern();

    gfx_ctx_s ctxl = ctx;
    ctxl.pat       = gfx_pattern_interpolate(1, 6);
    int y_max      = clamp_i(oc->y_max, 0, t.h - 1);

    // fill "static" bottom section
    u32 *px = &((u32 *)t.px)[y_max * t.wword];
    for (int y = y_max; y < t.h; y++) {
        u32 pt = ~ctxf.pat.p[y & 1];
        for (int x = 0; x < t.wword; x++)
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