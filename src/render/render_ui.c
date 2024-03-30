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
    const gfx_ctx_s ctx    = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (g->areaname.fadeticks) {
        gfx_ctx_s ctx_af = ctx;
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

#define COIN_MONO_SPACING 8
#define COIN_POS_END_X    60
    fnt_s font_c      = asset_fnt(FNTID_SMALL);
    char  coins_l[16] = {0};
    strs_from_u32(g->save.coins, coins_l);
    int    coinstrl = fnt_length_px_mono(font_c, coins_l, COIN_MONO_SPACING);
    v2_i32 coinpos  = {COIN_POS_END_X - coinstrl, 15};
    fnt_draw_ascii_mono(ctx, font_c, coinpos, coins_l, SPR_MODE_BLACK, COIN_MONO_SPACING);
    if (g->coins_added) {
        char coins_add_l[16] = {0};
        strs_from_u32(abs_i32(g->coins_added), coins_add_l);
        int    coinastrl = fnt_length_px_mono(font_c, coins_add_l, COIN_MONO_SPACING);
        v2_i32 coinposa  = {COIN_POS_END_X - coinastrl, 30};
        fnt_draw_ascii_mono(ctx, font_c, coinposa, coins_add_l, SPR_MODE_BLACK, COIN_MONO_SPACING);
        v2_i32      coinposplus = {10, coinposa.y};
        const char *strsig      = 0 < g->coins_added ? "+" : "-";
        fnt_draw_ascii_mono(ctx, font_c, coinposplus, strsig, SPR_MODE_BLACK, COIN_MONO_SPACING);
    }

    obj_s *interactable = obj_from_obj_handle(g->hero_mem.interactable);
    if (interactable) {
        v2_i32 posi = obj_pos_center(interactable);
        posi        = v2_add(posi, camoff);
        posi.y -= 64 + 16;
        posi.x -= 32;
        int      btn_frame = (sys_tick() >> 5) & 1;
        texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
        gfx_spr(ctx, tui, posi, 0, 0);
    }

    if (g->save_ticks) {
        i32       saveframe = ((g->save_ticks >> 1) % 14) * 32;
        gfx_ctx_s ctx_save  = ctx;
        if (56 <= g->save_ticks) { // (14 * 2) << 1, two turns
            saveframe = 0;
            i32 t0    = g->save_ticks - SAVE_TICKS_FADE_OUT;
            if (0 <= t0) {
                i32 t1       = SAVE_TICKS - SAVE_TICKS_FADE_OUT;
                ctx_save.pat = gfx_pattern_interpolate(t1 - t0, t1);
            }
        }

        texrec_s trsave = asset_texrec(TEXID_MISCOBJ, 512 + saveframe, 0, 32, 32);
        gfx_spr(ctx_save, trsave, (v2_i32){16, 200}, 0, 0);
    }

    if (shop_active(g)) {
        shop_draw(g);
    }

    if (ohero) {
        v2_i32 hctr = v2_add(camoff, obj_pos_center(ohero));

        if (ohero->health < ohero->health_max) {
            gfx_ctx_s health_ctx = ctx;
            health_ctx.pat       = gfx_pattern_75();
            i32 health_r         = 150 + (ohero->health * 100) / ohero->health_max;
            i32 health_flicker   = 12 - (ohero->health * 12) / ohero->health_max;
            for (int y = 0; y < SYS_DISPLAY_H; y++) {
                i32 yy    = (hctr.y - y);
                i32 xx_sq = pow2_i32(health_r) - pow2_i32(yy);
                i32 offs  = rngr_sym_i32(health_flicker);
                if (0 < xx_sq) {
                    i32     xx  = sqrt_i32(xx_sq);
                    rec_i32 rr1 = {0, y, hctr.x - xx + offs, 1};
                    rec_i32 rr2 = {hctr.x + xx + offs, y, 400, 1};
                    gfx_rec_fill(health_ctx, rr1, GFX_COL_BLACK);
                    gfx_rec_fill(health_ctx, rr2, GFX_COL_BLACK);
                } else {
                    rec_i32 rr = {0, y, SYS_DISPLAY_H, 1};
                    gfx_rec_fill(health_ctx, rr, GFX_COL_BLACK);
                }
            }
        }

        texrec_s titem = asset_texrec(TEXID_UI, 240, 80 + g->item.selected * 32, 32, 32);
        gfx_spr(ctx, titem, (v2_i32){400 - 32, 0}, 0, 0);
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

void render_map(game_s *g, gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h, i32 s_q8, v2_i32 c_q8)
{

    rec_i32 rfill = {0, 0, ctx.dst.w, ctx.dst.h};
    gfx_rec_fill(ctx, rfill, PRIM_MODE_BLACK);
    v2_i32 offs = {x, y};

    enum {
        MAPVIEW_PASS_BACKGROUND = 0,
        MAPVIEW_PASS_WALLS      = 1,
        //
        NUM_MAPVIEW_PASSES
    };

    for (int pass = 0; pass < NUM_MAPVIEW_PASSES; pass++) {
        i32 wallcol  = (pass == MAPVIEW_PASS_BACKGROUND ? GFX_COL_BLACK : GFX_COL_WHITE);
        i32 wallhalf = (pass == MAPVIEW_PASS_BACKGROUND ? 4 : 2);
        i32 wallsize = wallhalf * 2;

        for (int i = 0; i < g->map_world.n_rooms; i++) {
            map_worldroom_s *room = &g->map_world.rooms[i];
            int              xx   = room->x / WORLD_GRID_W;
            int              yy   = room->y / WORLD_GRID_H;
            int              ww   = room->w / WORLD_GRID_W;
            int              hh   = room->h / WORLD_GRID_H;

            gfx_ctx_s ctxfill = ctx;
            ctxfill.pat       = g->map_worldroom == room ? gfx_pattern_50() : gfx_pattern_75();

            for (int ty = 0; ty < hh; ty++) {
                for (int tx = 0; tx < ww; tx++) {
                    if (!hero_visited_tile(g, room, tx, ty)) continue;

                    v2_i32 p1 = {(xx + tx + 0) << 8, (yy + ty + 0) << 8};
                    v2_i32 p2 = {(xx + tx + 1) << 8, (yy + ty + 1) << 8};
                    v2_i32 d1 = v2_add(offs, mapview_screen_from_world_q8(p1, c_q8, w, h, s_q8));
                    v2_i32 d2 = v2_add(offs, mapview_screen_from_world_q8(p2, c_q8, w, h, s_q8));
                    i32    dw = d2.x - d1.x;
                    i32    dh = d2.y - d1.y;

                    if (pass == MAPVIEW_PASS_BACKGROUND) { // fill room background
                        i32     rx1   = d1.x + 2 * (tx == 0);
                        i32     ry1   = d1.y + 2 * (ty == 0);
                        i32     rx2   = d2.x - 2 * (tx == ww - 1);
                        i32     ry2   = d2.y - 2 * (ty == hh - 1);
                        rec_i32 rfill = {rx1, ry1, rx2 - rx1, ry2 - ry1};
                        gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK_WHITE);
                    }

                    if (!(tx == 0 || ty == 0 || tx == ww - 1 || ty == hh - 1)) continue; // no walls

                    // draw walls
                    i32 walls = room->room_walls[tx + ty * ww];
                    if (walls & 1) { // left wall
                        rec_i32 rwall = {d1.x - wallhalf, d1.y - wallhalf, wallsize, dh + wallsize};
                        gfx_rec_fill(ctx, rwall, wallcol);
                    }
                    if (walls & 2) { // right wall
                        rec_i32 rwall = {d2.x - wallhalf, d1.y - wallhalf, wallsize, dh + wallsize};
                        gfx_rec_fill(ctx, rwall, wallcol);
                    }
                    if (walls & 4) { // up wall
                        rec_i32 rwall = {d1.x - wallhalf, d1.y - wallhalf, dw + wallsize, wallsize};
                        gfx_rec_fill(ctx, rwall, wallcol);
                    }
                    if (walls & 8) { // bot wall
                        rec_i32 rwall = {d1.x - wallhalf, d2.y - wallhalf, dw + wallsize, wallsize};
                        gfx_rec_fill(ctx, rwall, wallcol);
                    }
                }
            }
        }
    }

    for (int n = 0; n < g->save.n_map_pins; n++) {
        map_pin_s p      = g->save.map_pins[n];
        texrec_s  tpin   = asset_texrec(TEXID_UI, 256 + p.type * 32, 192, 32, 32);
        v2_i32    pinpos = v2_add(offs, mapview_screen_from_world_q8(p.pos, c_q8, w, h, s_q8));
        pinpos.x -= 16;
        pinpos.y -= 16;
        gfx_spr(ctx, tpin, pinpos, 0, 0);
    }
}

void render_pause(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    if (g->substate == SUBSTATE_MENUSCREEN || !ohero) {
        sys_del_menu_image();
    } else {
        tex_s            tex      = asset_tex(TEXID_PAUSE_TEX);
        gfx_ctx_s        ctx      = gfx_ctx_default(tex);
        i32              scl_q8   = MAPVIEW_SCL_DEFAULT << 8;
        map_worldroom_s *room     = g->map_worldroom;
        v2_i32           hero_pos = mapview_hero_world_q8(g);
        render_map(g, ctx, 0, 0, SYS_DISPLAY_W / 2, SYS_DISPLAY_H, scl_q8, hero_pos);

        v2_i32   pheropin = {SYS_DISPLAY_W / 4 - 16, SYS_DISPLAY_H / 2 - 16};
        texrec_s theropin = asset_texrec(TEXID_UI, 320, 160, 32, 32);
        gfx_spr(ctx, theropin, pheropin, 0, 0);

        tex_s tdisplay = asset_tex(0);
        for (int n = 0; n < tex.wword * tex.h; n++) {
            tdisplay.px[n] = tex.px[n];
        }
        sys_set_menu_image(tex.px, tex.h, tex.wword << 2);
    }
}