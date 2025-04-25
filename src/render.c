// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "app.h"
#include "game.h"

void draw_light_circle(gfx_ctx_s ctx, v2_i32 p, i32 radius, i32 strength);
i32  objs_draw(gfx_ctx_s ctx, g_s *g, v2_i32 cam, i32 ifrom, i32 prio);

static inline i32 cmp_obj_render_priority(obj_s **a, obj_s **b)
{
    return (i32)(*a)->render_priority - (i32)(*b)->render_priority;
}

SORT_ARRAY_DEF(obj_s *, obj_render, cmp_obj_render_priority)

static void game_draw_door_or_pit(gfx_ctx_s ctx, g_s *g, rec_i32 r, v2_i32 cam, bool32 pit);

void game_draw(g_s *g)
{
    i32     i_obj                = 0;
    cam_s  *cam                  = &g->cam;
    hero_s *hero                 = &g->hero;
    rec_i32 camrec_raw           = cam_rec_px(g, cam);
    rec_i32 camrec               = {camrec_raw.x & ~1, camrec_raw.y & ~1,
                                    camrec_raw.w, camrec_raw.h};
    v2_i32  camoffset_raw        = {-camrec_raw.x, -camrec_raw.y};
    v2_i32  camoff               = {-camrec.x, -camrec.y};
    g->cam_prev                  = camoff;
    obj_s            *ohero      = obj_get_hero(g);
    tex_s             texdisplay = asset_tex(0);
    gfx_ctx_s         ctx        = gfx_ctx_default(texdisplay);
    tile_map_bounds_s tilebounds = tile_map_bounds_rec(g, camrec);
    area_s           *area       = &g->area;

    if (g->darken_bg_q12 == 4096) {
        tex_clr(ctx.dst, GFX_COL_BLACK);
    } else {
        area_draw_bg(g, &g->area, camoffset_raw, camoff);

        if (g->darken_bg_q12) {
            gfx_ctx_s ctxdarken = ctx;
            ctxdarken.pat       = gfx_pattern_interpolate(g->darken_bg_q12, 4096);
            gfx_rec_fill(ctxdarken, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
        }
    }

    if (g->cuts.on_draw_background) {
        g->cuts.on_draw_background(g, &g->cuts, camoff);
    }

    switch (area->fx_type) {
    case AFX_RAIN: {
        areafx_rain_draw(g, &area->fx.rain, camoff);
        break;
    }
    case AFX_SNOW: {
        areafx_snow_draw(g, &area->fx.snow, camoff);
        break;
    }
    }
    particle_sys_draw(g, camoff);

    if (g->objrender_dirty) {
        g->objrender_dirty = 0;
        sort_obj_render(g->obj_render, g->n_objrender);
    }

    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_BACKGROUND);

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_draw(ctx, &g->fluid_areas[n], camoff, 0);
    }
    render_tilemap(g, TILELAYER_BG, tilebounds, camoff);
    render_tilemap(g, TILELAYER_PROP_BG, tilebounds, camoff);
    render_tilemap(g, TILELAYER_BG_TILE, tilebounds, camoff);
    deco_verlet_draw(g, camoff);

    if (ohero && ohero->rope) {
        grapplinghook_draw(g, &g->ghook, camoff);
    }
    if (ohero) {
        if (g->hero.hook_aim_mode) {
            hero_hook_preview_throw(g, ohero, camoff);
        }
    }

    render_fluids(g, camoff, tilebounds);
    grass_draw(g, camrec, camoff);
    render_tilemap(g, TILELAYER_PROP_FG, tilebounds, camoff);

    if (g->render_map_doors || 1) {
        for (i32 n = 0; n < g->n_map_doors; n++) {
            map_door_s *md    = &g->map_doors[n];
            rec_i32     rdoor = {md->x, md->y, md->w, md->h};
            game_draw_door_or_pit(ctx, g, rdoor, camoff, 0);
        }
    }
    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_INFRONT_FLUID_AREA);
    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_draw(ctx, &g->fluid_areas[n], camoff, 1);
    }

    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_INFRONT_TERRAIN_LAYER);

    for (i32 n = 0; n < g->n_map_pits; n++) {
        map_pit_s *mp   = &g->map_pits[n];
        rec_i32    rpit = {mp->x, g->tiles_y - 1, mp->w, 1};
        game_draw_door_or_pit(ctx, g, rpit, camoff, 1);
    }
    boss_draw(g, camoff);
    render_terrain(g, tilebounds, camoff);

    i_obj = objs_draw(ctx, g, camoff, i_obj, I32_MAX);

    if (g->dark) {
        spm_push();

        tex_s tlight = tex_create(PLTF_DISPLAY_W, PLTF_DISPLAY_H, 0, spm_allocator(), 0);
        tex_clr(tlight, GFX_COL_BLACK);
        gfx_ctx_s lctx = gfx_ctx_default(tlight);

        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_LIGHT)) continue;

            i32    r = it->light_radius;
            v2_i32 p = v2_i32_add(obj_pos_center(it), camoff);
            draw_light_circle(lctx, p, r, it->light_strength);
        }

        u32 *p1 = ctx.dst.px;
        u32 *p2 = tlight.px;
        for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
            *p1++ &= *p2++;
        }

        spm_pop();
    }

#if 0 // godray overlay test
    gfx_ctx_s ctx_godray  = ctx;
    // ctx_godray.pat       = gfx_pattern_2x2(B2(10), B2(01));
    i32       raystrength = 7 + ((g->tick >> 6) & 3);
    for (i32 n = 0; n < 35; n++) {
        ctx_godray.pat = gfx_pattern_interpolate((35 - n) * raystrength / 10, 35 * 2);
        rec_i32 rray1  = {200 - n - 5,
                          0 + n,
                          30 + 10,
                          1};
        gfx_rec_fill(ctx_godray, rray1, PRIM_MODE_WHITE);
        rray1.x += 60;
        rray1.w -= 5;
        gfx_rec_fill(ctx_godray, rray1, PRIM_MODE_WHITE);

        ctx_godray.pat = gfx_pattern_interpolate((35 - n) * raystrength / 10, 35);
        rec_i32 rray2  = {200 - n,
                          0 + n,
                          30,
                          1};

        gfx_rec_fill(ctx_godray, rray2, PRIM_MODE_WHITE);
        rray2.x += 60;
        rray2.w -= 5;

        gfx_rec_fill(ctx_godray, rray2, PRIM_MODE_WHITE);
    }
#endif

    if (area->fx_type == AFX_HEAT) {
        areafx_heat_draw(g, &area->fx.heat, camoff);
    }

    // fill out of bounds with black
    if (0 < camoff.x) {
        rec_i32 rfilloff = {0, 0, camoff.x, PLTF_DISPLAY_H};
        gfx_rec_fill(ctx, rfilloff, GFX_COL_BLACK);
    } else if (g->pixel_x + camoff.x < PLTF_DISPLAY_W) {
        rec_i32 rfilloff = {PLTF_DISPLAY_W - (g->pixel_x + camoff.x), 0,
                            PLTF_DISPLAY_W, PLTF_DISPLAY_H};
        gfx_rec_fill(ctx, rfilloff, GFX_COL_BLACK);
    }
    if (0 < camoff.y) {
        rec_i32 rfill = {0, 0, PLTF_DISPLAY_W, camoff.y};
        gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);
    } else if (g->pixel_y + camoff.y < PLTF_DISPLAY_H) {
        rec_i32 rfill = {0, g->pixel_y + camoff.y, PLTF_DISPLAY_W, PLTF_DISPLAY_H};
        gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);
    }

    render_ui(g, camoff);

    i32 breath_t = hero_breath_tick(ohero);
    if (breath_t) {
        spm_push();
        tex_s drowntex = tex_create(PLTF_DISPLAY_W, PLTF_DISPLAY_H, 0, spm_allocator(), 0);
        tex_clr(drowntex, GFX_COL_WHITE);
        gfx_ctx_s ctx_drown = gfx_ctx_default(drowntex);
        ctx_drown.pat       = gfx_pattern_4x4(B4(1101),
                                              B4(1111),
                                              B4(0111),
                                              B4(1111));

        v2_i32 herop = v2_i32_add(camoff, obj_pos_center(ohero));
        gfx_rec_fill(ctx_drown, CINIT(rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
        ctx_drown.pat = gfx_pattern_100();
        i32 breath_tm = hero_breath_tick_max(g);
        i32 cird      = ease_out_quad(700, 0, breath_t, breath_tm);

        gfx_cir_fill(ctx_drown, herop, cird, PRIM_MODE_WHITE);

        u32 *p1 = ctx.dst.px;
        u32 *p2 = drowntex.px;
        for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
            *p1++ &= *p2++;
        }
        spm_pop();
    }

    boss_draw_post(g, camoff);
    if (g->cuts.on_draw) {
        g->cuts.on_draw(g, &g->cuts, camoff);
    }
    cam->prev_gfx_offs = camoff;

    if (g->dialog.state) {
        dialog_draw(g);
    }
}

static void game_draw_door_or_pit(gfx_ctx_s ctx, g_s *g, rec_i32 r, v2_i32 cam, bool32 pit)
{
    gfx_ctx_s ctxdoor = ctx;
    rec_i32   rfil    = {(r.x << 4) + cam.x,
                         (r.y << 4) + cam.y,
                         r.w << 4,
                         r.h << 4};

    i32 p = GFX_PATTERN_MAX - 1 - 4 * (sin_q15(g->tick << 10) + 32768) / 65537;
    i32 d = 0;

    // vertical glow - room to the side
    if (r.w == 1) {
        rfil.w = 4;

        if (r.x == 0) {
            d = +1;
        } else if (r.x == g->tiles_x - 1) {
            d = -1;
            rfil.x += 16 - rfil.w;
        }

        for (i32 k = 0; k < 8; k++) {
            i32 gp = p - ((k * k) >> 2);
            if (gp <= 0) break;

            ctxdoor.pat = gfx_pattern_bayer_4x4(gp);
            ctxdoor.pat = gfx_pattern_shift(ctxdoor.pat, d, 0);
            gfx_rec_fill(ctxdoor, rfil, PRIM_MODE_WHITE);
            rfil.x += d * 3;
        }
    }

    // horizontal glow - room above or below, or pit below
    if (r.h == 1) {
        i32 mode = PRIM_MODE_WHITE;
        rfil.h   = 4;

        if (r.y == 0) {
            d = +1;
        } else if (r.y == g->tiles_y - 1) {
            d = -1;
            rfil.y += 16 - rfil.h;
            if (pit) {
                mode = PRIM_MODE_BLACK;
                p -= 3;
            }
        }

        for (i32 k = 0; k < 8; k++) {
            i32 gp = p - ((k * k) >> 2);
            if (gp <= 0) break;

            ctxdoor.pat = gfx_pattern_bayer_4x4(gp);
            ctxdoor.pat = gfx_pattern_shift(ctxdoor.pat, 0, d);
            gfx_rec_fill(ctxdoor, rfil, mode);
            rfil.y += d * 3;
        }
    }
}

i32 objs_draw(gfx_ctx_s ctx, g_s *g, v2_i32 cam, i32 ifrom, i32 prio)
{
    i32 i = ifrom;
    for (; i < g->n_objrender; i++) {
        obj_s *o = g->obj_render[i];
        if (prio <= o->render_priority) break;

        if (o->blinking && ((g->tick_animation >> 1) & 1)) continue;

        v2_i32 ppos = v2_i32_add(o->pos, cam);
        if (o->ID == OBJID_HERO) {
            // slightly adjust player sprite position
            // in certain situations to align player sprite to camera
            if (g->cam.can_align_x && (ppos.x - 1) == g->cam.hero_off.x) {
                ppos.x--;
            }
            if (g->cam.can_align_y && (ppos.y - 1) == g->cam.hero_off.y) {
                ppos.y--;
            }
            g->cam.hero_off = ppos;
        }

        if (o->enemy.hurt_tick) {
            ppos.x += rngr_sym_i32(3);
            ppos.y += rngr_sym_i32(3);
        }

        for (i32 n = 0; n < o->n_sprites; n++) {
            obj_sprite_s sprite = o->sprites[n];
            if (!sprite.trec.t.px) continue;

            v2_i32 sprpos = v2_i32_add(ppos, v2_i32_from_i16(sprite.offs));
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, 0);
            if (o->enemy.flash_tick) {
                gfx_ctx_s ctxf = ctx;
                ctxf.pat       = gfx_pattern_2x2(B2(00), B2(01));
                gfx_spr(ctxf, sprite.trec, sprpos, sprite.flip, SPR_MODE_WHITE);
            }
        }
        if (o->on_draw) {
            o->on_draw(g, o, cam);
        }
#ifdef PLTF_DEBUG
        pltf_debugr(ppos.x, ppos.y, o->w, o->h, 0xFF, 0, 0, 1);
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
    return i;
}

void render_tilemap(g_s *g, int layer, tile_map_bounds_s bounds, v2_i32 camoffset)
{
    i32 texID = 0;
    switch (layer) {
    default: return;
    case TILELAYER_BG:
        texID = TEXID_TILESET_BG_AUTO;
        break;
    case TILELAYER_BG_TILE:
        texID = TEXID_TILESET_DECO;

        break;
    case TILELAYER_PROP_FG:
    case TILELAYER_PROP_BG:
        texID = TEXID_TILESET_PROPS;
        break;
    }
    tex_s     tex = asset_tex(texID);
    gfx_ctx_s ctx = gfx_ctx_display();

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 tID = g->rtiles[layer][x + y * g->tiles_x];
            if (tID == 0) continue;
            v2_i32   p   = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            i32      tx  = tID & 1;
            i32      ty  = tID >> 1;
            texrec_s trr = {tex, tx << 4, ty << 4, 16, 16};
            gfx_spr_tile_32x32(ctx, trr, p);
        }
    }
}

typedef struct {
    ALIGNAS(4)
    u16 z;
    u16 ty;
    i16 x;
    i16 y;
} tile_spr_s;

static inline i32 cmp_tile_spr(tile_spr_s *a, tile_spr_s *b)
{
    return ((i32)a->z - (i32)b->z);
}

SORT_ARRAY_DEF(tile_spr_s, z_tile_spr, cmp_tile_spr)

void render_terrain(g_s *g, tile_map_bounds_s bounds, v2_i32 camoffset)
{
    tex_s      tset       = asset_tex(TEXID_TILESET_TERRAIN);
    gfx_ctx_s  ctx        = gfx_ctx_display();
    i32        tick       = g->tick_animation;
    i32        n_tile_spr = 0;
    tile_spr_s tile_spr[480]; // max 27 * 17 tiles in theory

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (rt.u == 0) continue;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};

            if (rt.ty == 0) continue;
#if defined(PLTF_DEBUG) && 0
            i32 t1 = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t1 && t1 < NUM_TILE_SHAPES)) continue;
            texrec_s tr1 = asset_texrec(TEXID_COLLISION_TILES, 0, t1 * 16, 16, 16);
            gfx_spr(ctx, tr1, p, 0, 0);
#else
            // draw terrain tiles sorted by type, insert in buffer first
            tile_spr_s sp = {rt.type & 31, rt.ty, p.x, p.y};
            assert(n_tile_spr < ARRLEN(tile_spr));
            tile_spr[n_tile_spr++] = sp;
#endif
        }
    }

    sort_z_tile_spr(tile_spr, n_tile_spr);
    spm_push();
    tex_s     tglare   = tex_create(32, 16, 1, spm_allocator(), 0);
    gfx_ctx_s ctxglare = gfx_ctx_default(tglare);
    texrec_s  trglare  = {tglare, 0, 0, 32, 16};

    for (i32 n = 0; n < n_tile_spr; n++) {
        tile_spr_s sp   = tile_spr[n];
        texrec_s   trec = {tset, 0, (i32)sp.ty << 5, 32, 32};
        v2_i32     pos  = {sp.x - 8, sp.y - 8};
        gfx_spr_tile_32x32(ctx, trec, pos);

        switch (sp.z) {
        case TILE_TYPE_DARK_OBSIDIAN: {
            tex_clr(tglare, GFX_COL_CLEAR);
            ctxglare.pat = gfx_pattern_2x2(B2(00), B2(10));

            for (i32 h = 0; h < 16; h++) {
                i32 gl_x = sp.y - sp.x + h - 400 + ((g->tick * 14) & 2047);
                gfx_rec_strip(ctxglare, gl_x, h, 30, GFX_COL_WHITE);
            }

            u32 *ptilem = tset.px + ((trec.y + 256 + 8) * tset.wword) + 1;
            u32 *pglare = tglare.px + 1;
            for (i32 h = 0; h < 16; h++) {
                *pglare &= *ptilem; // mask
                ptilem += tset.wword;
                pglare += 2;
            }
            v2_i32 glarepos = {pos.x, pos.y + 8};
            gfx_spr(ctx, trglare, glarepos, 0, SPR_MODE_WHITE_ONLY);
            break;
        }
        case TILE_TYPE_THORNS: {
            tex_clr(tglare, GFX_COL_CLEAR);
            ctxglare.pat = gfx_pattern_bayer_4x4(3);

            for (i32 h = 0; h < 16; h++) {
                i32 gl_x = sp.y - sp.x + h - 400 + ((g->tick * 20) & 1023);
                gfx_rec_strip(ctxglare, gl_x, h, 90, GFX_COL_BLACK);
            }

            u32 *ptilem = tset.px + ((trec.y + 256 * 4 + 8) * tset.wword) + 1;
            u32 *pglare = tglare.px + 1;
            for (i32 h = 0; h < 16; h++) {
                *pglare &= *ptilem; // mask
                ptilem += tset.wword;
                pglare += 2;
            }
            v2_i32 glarepos = {pos.x, pos.y + 8};
            gfx_spr(ctx, trglare, glarepos, 0, SPR_MODE_BLACK_ONLY);
            break;
        }
        }
    }
    spm_pop();
}

#define FLUID_SRC_TILE(X, Y) (X) + (Y) * 16

void render_fluids(g_s *g, v2_i32 camoff, tile_map_bounds_s bounds)
{
    const tex_s twat = asset_tex(TEXID_FLUIDS);
    u32         tick = pltf_cur_tick();
    gfx_ctx_s   ctx  = gfx_ctx_display();
    tex_s       t    = ctx.dst;

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 fl = g->fluid_streams[x + y * g->tiles_x];
            if (fl == 0) continue;

            i32    tile_index = fl - 1;
            i32    frx        = 0;
            i32    fry        = 0;
            i32    frw        = 16;
            i32    frh        = 16;
            i32    framex     = 12 * 16 * (((tick) / 4) & 3);
            v2_i32 p          = {camoff.x + (x << 4), camoff.y + (y << 4)};
            u32    nb         = 0; // neighbours

            switch (tile_index) {
                // WATERFALL BIG
            case FLUID_SRC_TILE(1, 0):
            case FLUID_SRC_TILE(1, 1):
            case FLUID_SRC_TILE(1, 2): {
                if (x < g->tiles_x - 1)
                    nb |= (!!g->fluid_streams[x + 1 + y * g->tiles_x]);
                if (0 < x)
                    nb |= (!!g->fluid_streams[x - 1 + y * g->tiles_x]) << 1;

                switch (nb) {
                case 0: frx = 16 * 5; break;             // none
                case 1: frx = 16 * 1; break;             // r
                case 2: frx = 16 * 4; break;             // l
                case 3: frx = 16 * (2 + (x & 1)); break; // l & r
                }

                switch (tile_index) {
                case FLUID_SRC_TILE(1, 0): fry = 16 * 1; break;
                case FLUID_SRC_TILE(1, 1):
                case FLUID_SRC_TILE(1, 2): fry = 16 * (2 + (y & 1)); break;
                }
                break;
            }
            case FLUID_SRC_TILE(3, 0):
            case FLUID_SRC_TILE(3, 1):
                frx = 6 * 16;
                fry = (2 + (y & 1)) * 16;
                break;
            case FLUID_SRC_TILE(7, 0):
            case FLUID_SRC_TILE(7, 1):
                frx = 6 * 16;
                fry = (6 + (y & 1)) * 16;
                break;
                // WATER STREAM FACING RIGHT
            case FLUID_SRC_TILE(6, 0):
                frx = 11 * 16;
                fry = 7 * 16;
                break;
            case FLUID_SRC_TILE(6, 2):
                frx = 8 * 16;
                fry = 7 * 16;
                break;
            case FLUID_SRC_TILE(6, 1):
                frx = (8) * 16;
                fry = (5 + (y & 1)) * 16;
                break;
            case FLUID_SRC_TILE(7, 2): {
                frx = (9 + (x & 1)) * 16;
                fry = 7 * 16;
                break;
            }
                // WATER STREAM FACING LEFT
            case FLUID_SRC_TILE(4, 0):
                frx = 8 * 16;
                fry = 3 * 16;
                break;
            case FLUID_SRC_TILE(4, 2):
                frx = 11 * 16;
                fry = 3 * 16;
                break;
            case FLUID_SRC_TILE(4, 1):
                frx = 11 * 16;
                fry = (1 + (y & 1)) * 16;
                break;
            case FLUID_SRC_TILE(3, 2):
                frx = (9 + (x & 1)) * 16;
                fry = 3 * 16;
                break;
                //
            }

            texrec_s trw = {twat, frx + framex, fry, frw, frh};
            gfx_spr_tile_32x32(ctx, trw, p);

            // FOAM
            bool32 render_foam = 1;
            switch (tile_index) {
            default: render_foam = 0; break;
            case FLUID_SRC_TILE(1, 2): {
                fry = 4 * 16;
                switch (nb) {
                case 0:
                    frx = 16 * 5;
                    break; // none
                case 1:
                    p.x -= 16;
                    frx = 0;
                    frw = 32;
                    break; // r
                case 2:
                    frx = 16 * 4;
                    frw = 32;
                    break; // l
                case 3:
                    frx = 16 * (2 + (x & 1));
                    break; // l & r
                }
                break;
            }
            case FLUID_SRC_TILE(7, 1): p.x -= 8;
            case FLUID_SRC_TILE(3, 1): {
                fry = 16 * 4;
                frx = 16 * 6;
                frw = 32;
                break;
            }
            }

            if (render_foam) {
                texrec_s tr_foam = {twat, frx + framex, fry, frw, frh};
                p.y += 4;
                gfx_spr_tile_32x32(ctx, tr_foam, p);
            }
        }
    }
}

// gets the tile index of a terrain block
// tx = width of block in tiles
// ty = height of block in tiles
// x = tile position within tx
// y = tile position within ty
// returns index into "transformed" tile coordinates
i32 tileindex_terrain_block(i32 tx, i32 ty, i32 tile_type, i32 x, i32 y)
{
    i32 tilex = 0;
    i32 tiley = 0;

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
            tilex = 5, tiley = 3;
        } else if (y == 0) { // top
            tilex = 4, tiley = 0;
        } else if (x == tx - 1) { // right
            tilex = 6, tiley = 1;
        } else if (y == ty - 1) { // bottom
            tilex = 4, tiley = 2;
        } else { // middle
            tilex = 4, tiley = 1;
        }
    }

    // transform coordinates into "transformed" tile index
    return (tilex + (tiley + ((tile_type - 2) * 8)) * 12);
}

// renders a block of tiles dressed up as tile_type
void render_tile_terrain_block(gfx_ctx_s ctx, v2_i32 pos, i32 tx, i32 ty, i32 tile_type)
{
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
            tr.y = tileindex_terrain_block(tx, ty, tile_type, x, y) << 5;

            v2_i32 p = {pos.x + (x << 4) - 8, pos.y + (y << 4) - 8};
            gfx_spr(ctx, tr, p, 0, 0);
        }
    }
}

void render_map_doors(g_s *g, v2_i32 camoff)
{
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

void draw_light_circle(gfx_ctx_s ctx, v2_i32 p, i32 radius, i32 strength)
{
    i32       str  = min_i32(strength + 1, 4);
    u32       seed = pltf_cur_tick() >> 1;
    i32       r    = radius;
    i32       y1   = max_i32(p.y - r, ctx.clip_y1);
    i32       y2   = min_i32(p.y + r, ctx.clip_y2);
    gfx_ctx_s ctxw = ctx;

    gfx_pattern_s pts[4] = {
        gfx_pattern_2x2(B2(00), B2(10)),
        gfx_pattern_2x2(B2(01), B2(10)),
        gfx_pattern_2x2(B2(11), B2(10)),
        gfx_pattern_2x2(B2(11), B2(11))};
    i32 r2[4] = {
        pow2_i32((256 * r) >> 8),
        pow2_i32((220 * r) >> 8),
        pow2_i32((190 * r) >> 8),
        pow2_i32((150 * r) >> 8)};

    for (i32 y = y1; y <= y2; y++) {
        i32 px  = p.x + rngsr_i32(&seed, -1, +1);
        i32 dy2 = pow2_i32(p.y - y);
        for (i32 i = 0; i < str; i++) {
            i32 dx   = sqrt_u32(max_i32(r2[i] - dy2, 0));
            i32 x1   = px - dx;
            ctxw.pat = pts[i];
            gfx_rec_strip(ctxw, x1, y, dx << 1, GFX_COL_WHITE);
        }
    }
}

void render_map_transition_in(g_s *g, v2_i32 cam, i32 t, i32 t2)
{
    tex_s     display = asset_tex(0);
    tex_s     tmp     = asset_tex(TEXID_DISPLAY_TMP);
    obj_s    *ohero   = obj_get_tagged(g, OBJ_TAG_HERO);
    gfx_ctx_s ctxfill = gfx_ctx_default(tmp);
    ctxfill.pat       = gfx_pattern_interpolate(t2 - t, t2);

    rec_i32 rfill = {0, 0, tmp.w, tmp.h};
    gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK_WHITE);

    if (ohero) {
        gfx_ctx_s ctxc = gfx_ctx_default(tmp);
        v2_i32    cpos = v2_i32_add(obj_pos_center(ohero), cam);
        i32       cird = ease_out_quad(0, 200, min_i32(t, t2 / 2), t2 / 2);
        gfx_cir_fill(ctxc, cpos, cird, GFX_COL_WHITE);
    }

    u32 *p1 = display.px;
    u32 *p2 = tmp.px;
    for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
        *p1++ &= *p2++;
    }
}

void render_map_transition_out(g_s *g, v2_i32 cam, i32 t, i32 t2)
{
    tex_s     display = asset_tex(0);
    tex_s     tmp     = asset_tex(TEXID_DISPLAY_TMP);
    obj_s    *ohero   = obj_get_tagged(g, OBJ_TAG_HERO);
    gfx_ctx_s ctxfill = gfx_ctx_default(tmp);
    ctxfill.pat       = gfx_pattern_interpolate(t, t2);

    rec_i32 rfill = {0, 0, tmp.w, tmp.h};
    gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK_WHITE);

    u32 *p1 = display.px;
    u32 *p2 = tmp.px;
    for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
        *p1++ &= *p2++;
    }
}