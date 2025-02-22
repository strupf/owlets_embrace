// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define ITEM_FRAME_SIZE  64
#define ITEM_BARREL_R    16
#define ITEM_SIZE        32
#define ITEM_X_OFFS      325
#define ITEM_Y_OFFS      180
#define FNTID_AREA_LABEL FNTID_LARGE

void render_hero_ui(g_s *g, obj_s *ohero, v2_i32 camoff);

void render_ui(g_s *g, v2_i32 camoff)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (hero_present_and_alive(g, &ohero)) {
        render_hero_ui(g, ohero, camoff);
    }

    spm_push();
    {
        fnt_s font_sm       = asset_fnt(FNTID_SMALL);
        tex_s tex_dev_build = tex_create(128, 32, 1, spm_allocator2(), 0);
        tex_clr(tex_dev_build, GFX_COL_CLEAR);
        fnt_draw_ascii(gfx_ctx_default(tex_dev_build),
                       font_sm, CINIT(v2_i32){2, 2}, "Dev Build", SPR_MODE_BLACK);
        tex_outline_white(tex_dev_build);
        gfx_spr(ctx, texrec_from_tex(tex_dev_build), CINIT(v2_i32){340, 222}, 0, 0);
    }
    spm_pop();

#if 0
    if (g->areaname.fadeticks) {
        gfx_ctx_s ctx_af = ctx;
        texrec_s  tlabel = asset_texrec(TEXID_AREALABEL, 0, 0, 256, 64);

        i32 p1 = (FADETICKS_AREALABEL * 1) / 8;
        i32 p2 = (FADETICKS_AREALABEL * 6) / 8;
        i32 ft = g->areaname.fadeticks;
        if (ft <= p1) {
            ctx_af.pat = gfx_pattern_interpolate(ft, p1);
        } else if (p2 <= ft) {
            ctx_af.pat = gfx_pattern_interpolate(FADETICKS_AREALABEL - ft, FADETICKS_AREALABEL - p2);
        }
        i32    strl = fnt_length_px(font_1, g->areaname.label);
        v2_i32 loc  = {(ctx_af.dst.w - strl) >> 1, 10};

        gfx_spr(ctx_af, tlabel, (v2_i32){loc.x, loc.y}, 0, 0);
    }
#endif

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
        gfx_spr(ctx_save, trsave, CINIT(v2_i32){16, 200}, 0, 0);
    }
}

void render_hero_ui(g_s *g, obj_s *ohero, v2_i32 camoff)
{
    fnt_s              font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s          ctx    = gfx_ctx_display();
    hero_s            *h      = (hero_s *)ohero->heap;
    v2_i32             posh   = v2_i32_add(obj_pos_center(ohero), camoff);
    hero_interaction_s hi     = hero_get_interaction(g, ohero);
    switch (hi.action) {
    case HERO_INTERACTION_INTERACT: {
        obj_s *oi = obj_from_obj_handle(hi.interact);
        if (!oi) break;

        v2_i32 pbt = v2_i32_add(camoff, obj_pos_center(oi));
        pbt.x -= 48 / 2;
        pbt.y -= 48 / 2 + oi->h;
        texrec_s trb = asset_texrec(TEXID_BUTTONS,
                                    64 * ((g->tick >> 5) & 1),
                                    32,
                                    48,
                                    48);
        gfx_spr(ctx, trb, pbt, 0, 0);
        break;
    }
    }

    render_stamina_ui(g, ohero, camoff);

    coins_draw(g);

    void render_itemswap_ui(ui_itemswap_s * u, obj_s * ohero, v2_i32 camoff);
    render_itemswap_ui(&g->ui_itemswap, ohero, camoff);
}

void render_itemswap_ui(ui_itemswap_s *u, obj_s *ohero, v2_i32 camoff)
{
    if (!u->progress || u->ticks_in < UI_ITEMSWAP_TICKS_POP_UP) return;

    i32 ticks_progress = UI_ITEMSWAP_TICKS;

    spm_push();
    tex_s thold = tex_create(96, 32, 1, spm_allocator2(), 0);
    tex_clr(thold, GFX_COL_CLEAR);

    bool32    swapped = u->progress == ticks_progress;
    gfx_ctx_s ctx     = gfx_ctx_display();
    gfx_ctx_s ctxhold = gfx_ctx_default(thold);
    v2_i32    pswapl  = {12, 0};
    v2_i32    pbbut   = {0, -5};
    texrec_s  trswap  = asset_texrec(TEXID_BUTTONS, 32, 160, 64, 32);
    texrec_s  trbbut  = asset_texrec(TEXID_BUTTONS, 160 + 32, 64, 32, 32);

    if (!inp_btn(INP_B)) {
        trbbut.x -= 32;
    }

    // "swap" label
    gfx_spr(ctxhold, trswap, pswapl, 0, 0);

    i32 t1 = u->progress - (UI_ITEMSWAP_TICKS_POP_UP + 5);
    i32 t2 = ticks_progress - (UI_ITEMSWAP_TICKS_POP_UP + 5);
    if (0 < t1) {
        // "swap" label inverted -> progress bar
        trswap.y += 32;
        trswap.w = (trswap.w * t1) / t2;
        i32 mode = SPR_MODE_XOR;

        if (swapped && u->ticks_out < 10 && ((u->ticks_out >> 1) & 1)) {
            // brief blinking of "swap" label after swapping
            mode = SPR_MODE_WHITE;
        }
        gfx_spr(ctxhold, trswap, pswapl, 0, mode);
    }

    // B button image
    gfx_spr(ctxhold, trbbut, pbbut, 0, 0);

    // final image
    gfx_ctx_s ctxhold_res = ctx;
    v2_i32    posswap     = v2_i32_add(obj_pos_center(ohero), camoff);
    posswap.x += 20 * ohero->facing - 40;
    posswap.y -= 55;

    if (!swapped && u->ticks_out && u->ticks_out < 15) {
        posswap.x += (u->ticks_out % 5) - 2; // shake left/right in denial
    }

    // fade out after swapping successfully
    if (20 <= u->ticks_out) {
        i32 h2          = UI_ITEMSWAP_TICKS_FADE - 20;
        ctxhold_res.pat = gfx_pattern_interpolate(h2 - (u->ticks_out - 20), h2);
        posswap.y += (u->ticks_out - 20) >> 1;
    }

    i32 f1 = u->ticks_in - UI_ITEMSWAP_TICKS_POP_UP;
    if (f1 < 5) {
        ctxhold_res.pat = gfx_pattern_interpolate(f1, 5);
    }

    tex_outline_white(thold);
    gfx_spr(ctxhold_res, texrec_from_tex(thold), posswap, 0, 0);
    spm_pop();
}

void render_stamina_ui(g_s *g, obj_s *o, v2_i32 camoff)
{
    hero_s *h = (hero_s *)o->heap;
    if (!(h->stamina_upgrades && h->stamina_ui_fade_out)) return;

    gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s ctxb = ctx;
    gfx_ctx_s ctxc = ctx;
    v2_i32    p    = v2_i32_add(o->pos, camoff);

    i32     wi_total = 28 + (h->stamina_upgrades - 1) * 3;
    i32     wi       = (wi_total * h->stamina_ui_fade_out) / STAMINA_UI_TICKS_HIDE;
    i32     ft0      = hero_stamina_max(o);
    i32     ftx      = hero_stamina_ui_full(o);
    i32     fty      = hero_stamina_ui_added(o);
    rec_i32 rsta     = {p.x + 5 - (wi) / 2, p.y - 32, wi, 10};
    rec_i32 rsta_1   = {rsta.x - 2, rsta.y - 2, rsta.w + 4, rsta.h + 4};
    rec_i32 rsta_2   = {rsta.x + 2, rsta.y + 2, (ftx * (rsta.w - 4)) / ft0, rsta.h - 4};
    rec_i32 rsta_3   = {rsta.x + 2, rsta.y + 2, ((ftx + fty) * (rsta.w - 4)) / ft0, rsta.h - 4};

    ctxc.pat = gfx_pattern_2x2(B4(0001),
                               B4(0010));
    if (ftx == 0) {
        i32 i    = max_i32(40000, sin_q16(pltf_cur_tick() << 14));
        ctxb.pat = gfx_pattern_interpolate(i, 65536);
    }

    gfx_rec_rounded_fill(ctx, rsta_1, -1, GFX_COL_WHITE);
    gfx_rec_rounded_fill(ctxb, rsta, -1, GFX_COL_BLACK);
    gfx_rec_rounded_fill(ctxc, rsta_3, -1, GFX_COL_WHITE);
    gfx_rec_rounded_fill(ctxb, rsta_2, -1, GFX_COL_WHITE);

    for (i32 n = 0; n < h->stamina_upgrades - 1; n++) {
        i32     pp = (rsta.w * (n + 1)) / h->stamina_upgrades;
        rec_i32 rr = {rsta.x + pp, rsta.y + 3, 1, rsta.h - 6};
        gfx_rec_fill(ctx, rr, GFX_COL_BLACK);
    }
}

void prerender_area_label(g_s *g)
{

#if 0
    fnt_s     font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s ctx    = gfx_ctx_default(asset_tex(TEXID_AREALABEL));
    tex_clr(ctx.dst, GFX_COL_CLEAR);
    v2_i32 loc = {2, 2};
    fnt_draw_ascii(ctx, font_1, loc, g->areaname.label, SPR_MODE_BLACK);
    tex_outline_white(ctx.dst);
    tex_outline_white(ctx.dst);
#endif
}

void render_pause(g_s *g)
{
}