// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "app.h"
#include "game.h"

void draw_gameplay(g_s *g);
void draw_light_circle(gfx_ctx_s ctx, v2_i32 p, i32 radius, i32 strength);
i32  objs_draw(gfx_ctx_s ctx, g_s *g, v2_i32 cam, i32 ifrom, i32 prio);

static inline i32 cmp_obj_render_priority(obj_s **a, obj_s **b)
{
    return (i32)(*a)->render_priority - (i32)(*b)->render_priority;
}

SORT_ARRAY_DEF(obj_s *, obj_render, cmp_obj_render_priority)

static v2_i32 fg_parallax(v2_i32 cam, i32 x_q8, i32 y_q8, i32 ax, i32 ay)
{
    v2_i32 p = {((cam.x * x_q8) >> 8) & ~ax, ((cam.y * y_q8) >> 8) & ~ay};
    return p;
}

void game_draw(g_s *g)
{
    tex_s     texdisplay = asset_tex(0);
    gfx_ctx_s ctx        = gfx_ctx_from_tex(texdisplay);

    bool32 draw_gp = 1;
    if (g->minimap.state &&
        (g->minimap.state != MINIMAP_ST_FADE_IN &&
         g->minimap.state != MINIMAP_ST_FADE_IN_MENU &&
         g->minimap.state != MINIMAP_ST_FADE_OUT)) {
        draw_gp = 0;
    }
    if (draw_gp) {
        draw_gameplay(g);
    }

    render_ui(g);
    if (g->minimap.state) {
        minimap_draw(g);
    } else if (g->dia.state) {
        dia_draw(g);
    }
}

void draw_gameplay(g_s *g)
{
    i32               i_obj             = 0;
    cam_s            *cam               = &g->cam;
    obj_s            *ohero             = obj_get_owl(g);
    owl_s            *owl               = &g->owl;
    v2_i32            cam_top_left      = cam_pos_px_top_left(cam);
    v2_i32            cam_mid           = {cam_top_left.x + CAM_WH, cam_top_left.y + CAM_HH};
    rec_i32           camrec_raw        = {cam_top_left.x, cam_top_left.y, CAM_W, CAM_H};
    rec_i32           camrec            = {cam_top_left.x & ~1, cam_top_left.y & ~1, CAM_W, CAM_H};
    v2_i32            camoffset_raw     = {-cam_top_left.x, -cam_top_left.y};
    v2_i32            camoff            = {-camrec.x, -camrec.y};
    tex_s             texdisplay        = asset_tex(0);
    gfx_ctx_s         ctx               = gfx_ctx_from_tex(texdisplay);
    tile_map_bounds_s tilebounds        = tile_map_bounds_rec(g, camrec);
    tex_s             tex_outline_layer = asset_tex(TEXID_DISPLAY_WHITE_OUTLINED);
    tex_clr(tex_outline_layer, GFX_COL_CLEAR);

    background_draw(g, camoffset_raw, camoff);

    switch (g->vfx_ID) {
    default: break;
    case VFX_ID_SNOW:
        vfx_area_snow_draw(g, camoff);
        break;
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

    if (g->cs.on_draw_background) {
        g->cs.on_draw_background(g, &g->cs, camoff);
    }

    render_fluids(g, camoff, tilebounds);
    grapplinghook_draw(g, &g->ghook, camoff);

    if (owl->aim_ticks) {
        v2_i32 vaim  = owl_hook_aim_vec_from_crank_angle(inp_crank_q16(), 100);
        v2_i32 phero = obj_pos_center(ohero);
        phero        = v2_i32_add(phero, camoff);
        vaim         = v2_i32_add(phero, vaim);
        gfx_lin_thick(ctx, phero, vaim, PRIM_MODE_WHITE, 5);
    }

    grass_draw(g, camrec, camoff);

    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_INFRONT_FLUID_AREA);
    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_draw(ctx, &g->fluid_areas[n], camoff, 1);
    }

    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_INFRONT_TERRAIN_LAYER);
    boss_draw(g, camoff);
    tex_merge_to_opaque_outlined_white(texdisplay, tex_outline_layer);
    render_terrain(g, tilebounds, camoff);

    i32     parallaxox = ((cam_mid.x - (g->pixel_x >> 1)) * 410) >> 12;
    i32     parallaxoy = ((cam_mid.y - (g->pixel_y >> 1)) * 410) >> 12;
    rec_i32 camrec_fg  = {(cam_top_left.x + parallaxox) & ~1,
                          (cam_top_left.y + parallaxoy) & ~1,
                          CAM_W, CAM_H};
    v2_i32  camoff_fg  = {-camrec_fg.x, -camrec_fg.y};

    tile_map_bounds_s tilebounds2 = tile_map_bounds_rec(g, camrec_fg);
    render_tilemap(g, TILELAYER_FG, tilebounds2, camoff_fg);
    foreground_draw(g, camoffset_raw, camoff);

    i_obj = objs_draw(ctx, g, camoff, i_obj, RENDER_PRIO_UI_LEVEL);
    if (g->dark) {
        spm_push();

        tex_s tlight = tex_create(PLTF_DISPLAY_W, PLTF_DISPLAY_H, 0, spm_allocator(), 0);
        tex_clr(tlight, GFX_COL_BLACK);
        gfx_ctx_s lctx = gfx_ctx_from_tex(tlight);

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

    switch (g->vfx_ID) {
    case VFX_ID_HEAT:
        vfx_area_heat_draw(g, camoff);
        break;
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

    boss_draw_post(g, camoff);
    if (g->cs.on_draw) {
        g->cs.on_draw(g, &g->cs, camoff);
    }

    i_obj = objs_draw(ctx, g, camoff, i_obj, I32_MAX);
    render_hero_ui(g, ohero, camoff);
}

i32 objs_draw(gfx_ctx_s ctx, g_s *g, v2_i32 cam, i32 ifrom, i32 prio)
{
    owl_s *h = &g->owl;
    i32    i = ifrom;

    for (; i < g->n_objrender; i++) {
        obj_s *o = g->obj_render[i];
        if (prio <= o->render_priority) break;
        if (o->flags & OBJ_FLAG_DONT_SHOW) continue;
        if (o->blinking && ((g->tick_gameplay >> 1) & 1)) continue;

        v2_i32 ppos = v2_i32_add(o->pos, cam);
        if (o->ID == OBJID_OWL) {
            if (g->cam.cowl.do_align_x) {
                ppos.x &= ~1;
            }
            if (g->cam.cowl.do_align_y) {
                ppos.y &= ~1;
            }
        }

        if (o->enemy.hurt_tick) {
            ppos.x += rngr_sym_i32(3);
            ppos.y += rngr_sym_i32(3);
        }

        for (i32 n = 0; n < o->n_sprites; n++) {
            obj_sprite_s sprite = o->sprites[n];
            if (!sprite.trec.t.px) continue;

            v2_i32 sprpos = v2_i32_add(ppos, v2_i32_from_i16(sprite.offs));
            gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, o->enemy.flash_tick ? SPR_MODE_WHITE : 0);

            if (o->ID == OBJID_OWL && h->stamina_blink_tick) {
                gfx_ctx_s ctx2 = ctx;

                i32 k1   = lerp_i32(0, 65536, h->stamina_blink_tick, h->stamina_blink_tick_max);
                i32 num  = (256 * (sin_q15(k1))) >> 15;
                // num      = (160 * ((num * num))) >> 16;
                num      = (192 * ((num))) >> 8;
                ctx2.pat = gfx_pattern_interpolate(num, 256);
                gfx_spr(ctx2, sprite.trec, sprpos, sprite.flip, SPR_MODE_BLACK);
            }
        }
        if (o->on_draw) {
            o->on_draw(g, o, cam);
        }
    }
    return i;
}

void render_tilemap(g_s *g, i32 layer, tile_map_bounds_s bounds, v2_i32 cam)
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
    case TILELAYER_PROP_BG:
        texID = TEXID_TILESET_PROPS;
        break;
    case TILELAYER_FG:
        texID = TEXID_TILESET_FRONT;
        break;
    }
    tex_s     tex    = asset_tex(texID);
    gfx_ctx_s ctx    = gfx_ctx_display();
    u16      *rtiles = g->rtiles[layer];

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 tID = rtiles[x + y * g->tiles_x];
            if (tID == 0) continue;

            v2_i32   p   = {(x << 4) + cam.x, (y << 4) + cam.y};
            i32      tx  = tID & 1;
            i32      ty  = tID >> 1;
            texrec_s trr = {tex, tx << 4, ty << 4, 16, 16};
            gfx_spr_tile_32x32(ctx, trr, p);
        }
    }
}

typedef struct {
    ALIGNAS(8)
    u8  z;
    u8  type;
    u16 ty;
    i16 x;
    i16 y;
} tile_spr_s;

static inline i32 cmp_tile_spr(tile_spr_s *a, tile_spr_s *b)
{
    return ((i32)a->z - (i32)b->z);
}

SORT_ARRAY_DEF(tile_spr_s, z_tile_spr, cmp_tile_spr)

void render_terrain(g_s *g, tile_map_bounds_s bounds, v2_i32 cam)
{
    i32                    n_tile_spr = 0;
    ALIGNAS(32) tile_spr_s tile_spr[460]; // max 27 * 17 tiles in theory

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            tile_s rt = g->tiles[x + y * g->tiles_x];
            if (rt.ty == 0) continue;

            // draw terrain tiles sorted by type, insert in buffer first
            tile_spr_s sp = {tile_type_render_priority(rt.type & 31),
                             rt.type & 31,
                             rt.ty,
                             (x << 4) + cam.x,
                             (y << 4) + cam.y};
            assert(n_tile_spr < ARRLEN(tile_spr));
            tile_spr[n_tile_spr++] = sp;
        }
    }

    sort_z_tile_spr(tile_spr, n_tile_spr);

    TEX_STACK_CTX(tglare, 32, 32, 1);
    texrec_s  trglare     = {tglare, 0, 0, 32, 32};
    tex_s     tset        = asset_tex(TEXID_TILESET_TERRAIN);
    gfx_ctx_s ctx         = gfx_ctx_display();
    i32       tick        = g->tick_gameplay;
    i32       glare_cache = ((g->tick * 14) & 2047) - 400;

    for (i32 n = 0; n < n_tile_spr; n++) {
        tile_spr_s sp   = tile_spr[n];
        texrec_s   trec = {tset, 0, (i32)sp.ty << 5, 32, 32};
        v2_i32     pos  = {sp.x - 8, sp.y - 8};
        gfx_spr_tile_32x32(ctx, trec, pos);

        switch (sp.type) {
        case TILE_TYPE_DARK_OBSIDIAN: {
            tex_clr(tglare, GFX_COL_CLEAR);
            tglare_ctx.pat = gfx_pattern_2x2(B2(00), B2(10));

            // diagonal light glare
            for (i32 h = 0, gl_x = sp.y - sp.x + glare_cache; h < 32; h++, gl_x++) {
                gfx_rec_strip(tglare_ctx, gl_x, h, 30, GFX_COL_WHITE);
            }

            u32 *ptilex = tset.px + ((trec.y + 8 * 12 * 32 * 3) * tset.wword) + 1;
            u32 *ptilem = tset.px + ((trec.y) * tset.wword) + 1;
            u32 *pglare = tglare.px + 1; // mask
            for (i32 h = 0; h < 32; h++) {
                *pglare &= *ptilem & *ptilex; // cut out mask pixels
                ptilem += tset.wword;
                ptilex += tset.wword;
                pglare += 2;
            }
            v2_i32 glarepos = {pos.x, pos.y};
            gfx_spr_tile_32x32(ctx, trglare, glarepos);
            break;
        }
        case TILE_TYPE_THORNS: {
            tex_clr(tglare, GFX_COL_CLEAR);
            tglare_ctx.pat = gfx_pattern_bayer_4x4(3);

            for (i32 h = 0; h < 16; h++) {
                i32 gl_x = sp.y - sp.x + h - 400 + ((g->tick * 20) & 1023);
                gfx_rec_strip(tglare_ctx, gl_x, h, 90, GFX_COL_BLACK);
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
}

#define FLUID_SRC_TILE(X, Y) (X) + (Y) * 16

void render_fluids(g_s *g, v2_i32 camoff, tile_map_bounds_s bounds)
{
    tex_s     twat = asset_tex(TEXID_FLUIDS);
    u32       tick = pltf_cur_tick();
    gfx_ctx_s ctx  = gfx_ctx_display();
    tex_s     t    = ctx.dst;

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
            tilex = 3, tiley = 7;
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
    return (tilex + (tiley + (tile_type << 3)) * 12);
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
    obj_s    *ohero   = obj_get_tagged(g, OBJ_TAG_OWL);
    gfx_ctx_s ctxfill = gfx_ctx_from_tex(tmp);
    ctxfill.pat       = gfx_pattern_interpolate(t2 - t, t2);

    rec_i32 rfill = {0, 0, tmp.w, tmp.h};
    gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK_WHITE);

    if (ohero) {
        gfx_ctx_s ctxc = gfx_ctx_from_tex(tmp);
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
    obj_s    *ohero   = obj_get_tagged(g, OBJ_TAG_OWL);
    gfx_ctx_s ctxfill = gfx_ctx_from_tex(tmp);
    ctxfill.pat       = gfx_pattern_interpolate(t, t2);

    rec_i32 rfill = {0, 0, tmp.w, tmp.h};
    gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK_WHITE);

    u32 *p1 = display.px;
    u32 *p2 = tmp.px;
    for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
        *p1++ &= *p2++;
    }
}

void foreground_draw(g_s *g, v2_i32 cam_al, v2_i32 cam)
{
    texrec_s  tr  = asset_texrec(TEXID_FOREGROUND, 0, 0, 0, 0);
    gfx_ctx_s ctx = gfx_ctx_display();
    // ctx.pat       = gfx_pattern_75();

    for (i32 n = 0; n < g->n_fg; n++) {
        foreground_el_s *fg = &g->fg_el[n];

        i32    wx = -cam_al.x + CAM_WH;
        i32    wy = -cam_al.y + CAM_HH;
        i32    ox = ((wx - fg->x) * fg->k_q8) >> 8;
        i32    oy = ((wy - fg->y) * fg->k_q8) >> 8;
        v2_i32 p  = {fg->x + cam_al.x - fg->tw / 2 - ox,
                     fg->y + cam_al.y - fg->th / 2 - oy};
        tr.x      = fg->tx;
        tr.y      = fg->ty;
        tr.w      = fg->tw;
        tr.h      = fg->th;
        gfx_spr(ctx, tr, p, 0, SPR_MODE_BLACK_ONLY_WHITE_PT_OPAQUE);
    }
}