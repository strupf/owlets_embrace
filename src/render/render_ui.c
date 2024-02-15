// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define ITEM_FRAME_SIZE  64
#define ITEM_BARREL_R    16
#define ITEM_SIZE        32
#define ITEM_X_OFFS      16
#define ITEM_Y_OFFS      16
#define FNTID_AREA_LABEL FNTID_LARGE

static void render_item_selector(item_selector_s *is);

void render_ui(game_s *g, v2_i32 camoff)
{
    const gfx_ctx_s ctx_ui = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (g->areaname.fadeticks) {
        gfx_ctx_s ctx_af = ctx_ui;
        texrec_s  tlabel = asset_texrec(TEXID_AREALABEL, 0, 0, 256, 64);
        int       p1     = (FADETICKS_AREALABEL * 1) / 8;
        int       p2     = (FADETICKS_AREALABEL * 6) / 8;
        int       ft     = g->areaname.fadeticks;
        if (ft <= p1) {
            ctx_af.pat = gfx_pattern_interpolate(ft, p1);
        } else if (p2 <= ft) {
            ctx_af.pat = gfx_pattern_interpolate(FADETICKS_AREALABEL - ft, FADETICKS_AREALABEL - p2);
        }
        int    strl = fnt_length_px(font_1, g->areaname.label);
        v2_i32 loc  = {(ctx_af.dst.w - strl) >> 1, 10};

        gfx_spr(ctx_af, tlabel, (v2_i32){loc.x, loc.y}, 0, 0);
    }

    obj_s *interactable = obj_from_obj_handle(g->herodata.interactable);
    if (interactable) {
        v2_i32 posi = obj_pos_center(interactable);
        posi        = v2_add(posi, camoff);
        posi.y -= 64 + 16;
        posi.x -= 32;
        int      btn_frame = (sys_tick() >> 5) & 1;
        texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
        gfx_spr(ctx_ui, tui, posi, 0, 0);
    }

    if (textbox_active(&g->textbox)) {
        textbox_draw(&g->textbox, camoff);
    }

    if (shop_active(g)) {
        shop_draw(g);
    }

#if !GAME_JUMP_ATTACK
    if (g->herodata.upgrades[HERO_UPGRADE_HOOK] &&
        g->herodata.upgrades[HERO_UPGRADE_WHIP]) {
        render_item_selector(&g->item_selector);
        texrec_s tr_items = asset_texrec(TEXID_UI_ITEM_CACHE,
                                         0, 0, 128, ITEM_FRAME_SIZE);
        gfx_spr(ctx_ui, tr_items, (v2_i32){400 - 80, 240 - 64 + 16}, 0, 0);
    }

#endif

    if (ohero) {
        texrec_s healthbar = asset_texrec(TEXID_UI, 0, 64, 8, 16);
        gfx_rec_fill(ctx_ui, (rec_i32){0, 0, ohero->health_max * 6 + 2, 16},
                     PRIM_MODE_BLACK);
        int bars = ohero->health;
        for (int i = 0; i < bars; i++) {
            v2_i32 barpos = {i * 6, 0};
            gfx_spr(ctx_ui, healthbar, barpos, 0, 0);
        }
    }

    if (g->die_ticks) {
        gfx_ctx_s ctx_black = gfx_ctx_display();
        ctx_black.pat       = gfx_pattern_interpolate(g->die_ticks, 50);
        gfx_rec_fill(ctx_black, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
    }

    if (g->respawn_ticks) {
        gfx_ctx_s ctx_black = gfx_ctx_display();
        ctx_black.pat       = gfx_pattern_interpolate(50 - g->respawn_ticks, 50);
        gfx_rec_fill(ctx_black, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
    }
}

void prerender_area_label(game_s *g)
{
    fnt_s     font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s ctx    = gfx_ctx_default(asset_tex(TEXID_AREALABEL));
    tex_clr(ctx.dst, TEX_CLR_TRANSPARENT);
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

void render_pause(game_s *g)
{
    spm_push();
    fnt_s font = asset_fnt(FNTID_LARGE);

    tex_s tex = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, spm_allocator);
    tex_clr(tex, TEX_CLR_WHITE);
    gfx_ctx_s ctx = gfx_ctx_default(tex);

    for (int i = 0; i < SYS_DISPLAY_H * SYS_DISPLAY_WWORDS; i++) {
        tex.px[i] = rng_u32();
    }

    v2_i32 pos = {80, 140};
    for (int y = -2; y <= +2; y++) {
        for (int x = -2; x <= +2; x++) {
            v2_i32 p = pos;
            p.x += x;
            p.y += y;
            fnt_draw_ascii(ctx, font, p, "Pause", SPR_MODE_WHITE);
        }
    }

    fnt_draw_ascii(ctx, font, pos, "Pause", SPR_MODE_BLACK);

    sys_set_menu_image(tex.px, tex.h, tex.wword * 4);
    spm_pop();
}

static void render_item_selector(item_selector_s *is)
{
    tex_s     texcache = asset_tex(TEXID_UI_ITEM_CACHE);
    gfx_ctx_s ctx      = gfx_ctx_default(texcache);
    texrec_s  tr       = {0};
    tr.t               = asset_tex(TEXID_UI_ITEMS);
    tex_clr(texcache, TEX_CLR_TRANSPARENT);

    // barrel background

    int offs = 15;
    if (!is->decoupled) {
        int dtang = abs_i(inp_crank_calc_dt_q16(is->angle, inp_crank_q16()));
        offs      = min_i(pow2_i32(dtang) / 4000000, offs);
    }

#define ITEM_OVAL_Y 12

    int turn1 = (inp_crank_q16() >> 6) + 0x400;
    int turn2 = (is->angle >> 6) + 0x400;
    int sy1   = (sin_q16(turn1) * ITEM_OVAL_Y) >> 16;
    int sy2   = (sin_q16(turn2) * ITEM_OVAL_Y) >> 16;
    int sx1   = (cos_q16(turn1));
    int sx2   = (cos_q16(turn2));

    // wraps the item image around a rotating barrel
    // map coordinate to angle to image coordinate
    //   |
    //   v_________
    //   /  \      \*
    //  /    \      \*
    //  |     \     |
    tr.r.x = 32;
    tr.r.w = ITEM_SIZE;
    tr.r.h = 1;
    for (int y = 0; y < 2 * ITEM_BARREL_R; y++) {
        int yy   = y - ITEM_BARREL_R;
        int abar = asin_q16((yy << 16) / ITEM_BARREL_R) >> 2; // asin returns TURN in Q18, one turn = 0x40000, shr by 2 for Q16
        int aimg = (abar + is->angle + 0x4000) & 0xFFFF;
        if (0 <= aimg && aimg < 0x8000) { // maps to [0, 180) deg (visible barrel)
            tr.r.y = is->item * ITEM_SIZE + (aimg * ITEM_SIZE) / 0x8000;
            gfx_spr(ctx, tr, (v2_i32){16, ITEM_Y_OFFS + y}, 0, 0);
        } else {
            gfx_rec_fill(ctx, (rec_i32){16, ITEM_Y_OFFS + y, ITEM_SIZE, 1}, PRIM_MODE_WHITE);
        }
    }

    tr.r = (rec_i32){64, 0, 64, 64}; // frame
    gfx_spr(ctx, tr, (v2_i32){0, 0}, 0, 0);
    tr.r = (rec_i32){144, 0, 32, 64}; // disc
    gfx_spr(ctx, tr, (v2_i32){48 + 17 + offs, 0}, 0, 0);
    tr.r = (rec_i32){112, 64, 16, 16}; // hole
    gfx_spr(ctx, tr, (v2_i32){48, 32 - 8 - sy2}, 0, 0);
    tr.r = (rec_i32){112, 80, 16, 16}; // bolt
    gfx_spr(ctx, tr, (v2_i32){49 + offs, 32 - 8 - sy1}, 0, 0);
}