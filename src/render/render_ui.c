// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define ITEM_FRAME_SIZE  64
#define ITEM_BARREL_R    16
#define ITEM_SIZE        32
#define ITEM_X_OFFS      325
#define ITEM_Y_OFFS      180
#define FNTID_AREA_LABEL FNTID_LARGE

void render_ui(game_s *g, v2_i32 camoff)
{
    const gfx_ctx_s ctx_ui = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (g->areaname.fadeticks) {
        gfx_ctx_s ctx_af = ctx_ui;
        texrec_s  tlabel = asset_texrec(TEXID_AREALABEL, 0, 0, 256, 64);

        int p1 = (FADETICKS_AREALABEL * 1) / 8;
        int p2 = (FADETICKS_AREALABEL * 6) / 8;
        int ft = g->areaname.fadeticks;
        if (ft <= p1) {
            ctx_af.pat = gfx_pattern_interpolate(ft, p1);
        } else if (p2 <= ft) {
            ctx_af.pat = gfx_pattern_interpolate(FADETICKS_AREALABEL - ft, FADETICKS_AREALABEL - p2);
        }
        int    strl = fnt_length_px(font_1, g->areaname.label);
        v2_i32 loc  = {(ctx_af.dst.w - strl) >> 1, 10};

        gfx_spr(ctx_af, tlabel, (v2_i32){loc.x, loc.y}, 0, 0);
    }

    obj_s *interactable = obj_from_obj_handle(g->hero_mem.interactable);
    if (interactable) {
        v2_i32 posi = obj_pos_center(interactable);
        posi        = v2_add(posi, camoff);
        posi.y -= 64 + 16;
        posi.x -= 32;
        int      btn_frame = (sys_tick() >> 5) & 1;
        texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
        gfx_spr(ctx_ui, tui, posi, 0, 0);
    }

    if (shop_active(g)) {
        shop_draw(g);
    }

    if (ohero) {
        texrec_s healthbar = asset_texrec(TEXID_UI, 0, 64, 8, 16);
        gfx_rec_fill(ctx_ui, (rec_i32){0, 0, ohero->health_max * 6 + 2, 16},
                     PRIM_MODE_BLACK);
        int bars = ohero->health;
        for (int i = 0; i < bars; i++) {
            v2_i32 barpos = {i * 6, 0};
            gfx_spr(ctx_ui, healthbar, barpos, 0, 0);
        }

        texrec_s titem = asset_texrec(TEXID_UI, 240, 80 + g->item.selected * 32, 32, 32);
        gfx_spr(ctx_ui, titem, (v2_i32){400 - 32, 0}, 0, 0);
    }
}

void prerender_area_label(game_s *g)
{
    fnt_s     font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s ctx    = gfx_ctx_default(asset_tex(TEXID_AREALABEL));
    tex_clr(ctx.dst, GFX_COL_CLEAR);
    char  *label = g->areaname.label;
    v2_i32 loc   = {2, 2};

    for (int yy = -2; yy <= +2; yy++) {
        for (int xx = -2; xx <= +2; xx++) {
            v2_i32 locbg = {loc.x + xx, loc.y + yy};
            fnt_draw_ascii(ctx, font_1, locbg, label, SPR_MODE_WHITE);
        }
    }
    fnt_draw_ascii(ctx, font_1, loc, label, SPR_MODE_BLACK);
}

#define WORLD_GRID_W SYS_DISPLAY_W
#define WORLD_GRID_H SYS_DISPLAY_H

v2_i32 mapview_world_q8_from_screen(v2_i32 pxpos, v2_i32 ctr_wpos_q8, i32 w, i32 h, i32 scl_q8)
{
    v2_i32 p = {(((pxpos.x - (w >> 1)) << 16) / scl_q8 + ctr_wpos_q8.x),
                (((pxpos.y - (h >> 1)) << 16) / scl_q8 + ctr_wpos_q8.y)};
    return p;
}

v2_i32 mapview_screen_from_world_q8(v2_i32 wpos_q8, v2_i32 ctr_wpos_q8, i32 w, i32 h, i32 scl_q8)
{
    v2_i32 p = {(((wpos_q8.x - ctr_wpos_q8.x) * scl_q8) >> 16) + (w >> 1),
                (((wpos_q8.y - ctr_wpos_q8.y) * scl_q8) >> 16) + (h >> 1)};
    return p;
}

void render_map(game_s *g, gfx_ctx_s ctx, i32 w, i32 h, i32 scl_q8, v2_i32 wctr_q8)
{
    gfx_ctx_s ctx_stripes = ctx;
    ctx_stripes.pat       = gfx_pattern_4x4(B4(0001),
                                            B4(0010),
                                            B4(0100),
                                            B4(1000));

    rec_i32 rfill = {0, 0, ctx.dst.w, ctx.dst.h};
    gfx_rec_fill(ctx, rfill, 0);

    // room walls
    for (int i = 0; i < g->map_world.n_rooms; i++) {
        map_worldroom_s *room = &g->map_world.rooms[i];
        int              xx   = room->x / WORLD_GRID_W;
        int              yy   = room->y / WORLD_GRID_H;
        int              ww   = room->w / WORLD_GRID_W;
        int              hh   = room->h / WORLD_GRID_H;

        // walls
        for (int y = 0; y < hh; y++) {
            for (int x = 0; x < ww; x++) {
                // if (!hero_visited_tile(g, room, x, y)) continue;
                v2_i32 p1_q8 = {(xx + x + 0) << 8, (yy + y + 0) << 8};
                v2_i32 p2_q8 = {(xx + x + 1) << 8, (yy + y + 1) << 8};
                v2_i32 d1    = mapview_screen_from_world_q8(p1_q8, wctr_q8, w, h, scl_q8);
                v2_i32 d2    = mapview_screen_from_world_q8(p2_q8, wctr_q8, w, h, scl_q8);
                i32    dw    = d2.x - d1.x;
                i32    dh    = d2.y - d1.y;

                rec_i32 rfill = {d1.x, d1.y, dw, dh};
                gfx_rec_fill(ctx_stripes, rfill, PRIM_MODE_WHITE_BLACK);

                if (!(x == 0 || y == 0 || x == ww - 1 || y == hh - 1)) continue; // no walls
                int walls = room->room_walls[x + y * ww];

                if (walls & 1) { // left wall
                    rec_i32 rwall = {d1.x - 2, d1.y - 2, 4, dh + 4};
                    gfx_rec_fill(ctx, rwall, PRIM_MODE_WHITE);
                }
                if (walls & 2) { // right wall
                    rec_i32 rwall = {d2.x - 2, d1.y - 2, 4, dh + 4};
                    gfx_rec_fill(ctx, rwall, PRIM_MODE_WHITE);
                }
                if (walls & 4) { // up wall
                    rec_i32 rwall = {d1.x - 2, d1.y - 2, dw + 4, 4};
                    gfx_rec_fill(ctx, rwall, PRIM_MODE_WHITE);
                }
                if (walls & 8) { // bot wall
                    rec_i32 rwall = {d1.x - 2, d2.y - 2, dw + 4, 4};
                    gfx_rec_fill(ctx, rwall, PRIM_MODE_WHITE);
                }
            }
        }
    }

#if 1
    v2_i32 origin = {0, 0};
    origin        = mapview_screen_from_world_q8(origin, wctr_q8, w, h, scl_q8);
    gfx_cir_fill(ctx, origin, 15, PRIM_MODE_WHITE);

    for (int n = 0; n < g->save.n_map_pins; n++) {
        map_pin_s p      = g->save.map_pins[n];
        texrec_s  tpin   = asset_texrec(TEXID_UI, 256 + p.type * 32, 192, 32, 32);
        v2_i32    pinpos = mapview_screen_from_world_q8(p.pos, wctr_q8, w, h, scl_q8);
        pinpos.x -= 16;
        pinpos.y -= 16;
        gfx_spr(ctx, tpin, pinpos, 0, 0);
    }
#endif
}

void render_pause(game_s *g)
{
    if (menu_screen_active(&g->menu_screen)) {
        sys_set_menu_image(NULL, 0, 0);
    } else {
        tex_s     tex = asset_tex(TEXID_PAUSE_TEX);
        gfx_ctx_s ctx = gfx_ctx_default(tex);
        tex_clr(tex, GFX_COL_BLACK);

        i32    scl_q8   = 32 << 8;
        v2_i32 hero_pos = {
            ((g->map_worldroom->x + g->map_worldroom->w / 2) << 8) / SYS_DISPLAY_W,
            ((g->map_worldroom->y + g->map_worldroom->h / 2) << 8) / SYS_DISPLAY_H};
        render_map(g, ctx, SYS_DISPLAY_W / 2, SYS_DISPLAY_H, scl_q8, hero_pos);

        v2_i32 cirpos = mapview_screen_from_world_q8(hero_pos, hero_pos,
                                                     SYS_DISPLAY_W / 2, SYS_DISPLAY_H, scl_q8);
        gfx_cir_fill(ctx, cirpos, scl_q8 >> 9, PRIM_MODE_WHITE);
        tex_s tdisplay = asset_tex(0);
        for (int n = 0; n < tex.wword * tex.h; n++) {
            tdisplay.px[n] = tex.px[n];
        }
        sys_set_menu_image(tex.px, tex.h, tex.wword << 2);
    }
}