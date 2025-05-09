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
#define FNTID_AREA_LABEL FNTID_MEDIUM

void render_hero_ui(g_s *g, obj_s *ohero, v2_i32 camoff);
void render_itemswap(obj_s *ohero, v2_i32 camoff);
void render_hurt_ui(g_s *g, obj_s *ohero);

void render_ui(g_s *g, v2_i32 camoff)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (hero_present_and_alive(g, &ohero)) {
        render_hero_ui(g, ohero, camoff);
    }

    if (g->minimap.state) {
        minimap_draw(g);
    }

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
}

void render_hurt_ui(g_s *g, obj_s *ohero)
{
#define HURT_VFX_FRIZZLE 8

    gfx_ctx_s ctx = gfx_ctx_display();
    if (ohero->health == 0 || ohero->health == ohero->health_max) return;

    hero_s *h           = (hero_s *)ohero->heap;
    i32     frizzle_str = 0;
    i32     pulse_off   = 0;
    i32     rad_off_add = 0;

    switch (ohero->health) {
    case 1:
        rad_off_add = 35;
        pulse_off   = 4;
        frizzle_str = 3;
        break;
    case 2:
        rad_off_add = 0;
        pulse_off   = 2;
        frizzle_str = 1;
        break;
    }

    i32 rad_off = (pulse_off * sin_q15(g->tick_animation << 10)) >> 15;
    rad_off -= ease_in_quad(0, 60, h->ticks_health, HERO_LOW_HP_ANIM_TICKS_HIT);
    rad_off -= rad_off_add;

    u32 sfrizzle = (g->tick_animation);
    i32 off_x1   = 0;
    i32 off_x2   = rngsr_sym_i32(&sfrizzle, frizzle_str);

    for (i32 y = 0; y < PLTF_DISPLAY_H; y++) {
        i32 ym = y & (HURT_VFX_FRIZZLE - 1);
        if (ym == 0) {
            off_x1 = off_x2;
            off_x2 = rngsr_sym_i32(&sfrizzle, frizzle_str);
        }

        i32 off_x = lerp_i32(off_x1, off_x2, ym, HURT_VFX_FRIZZLE);

        for (i32 rh = 0; rh < 2; rh++) {
            i32 ptbase = ohero->health == 2 ? 14 : 16;
            ctx.pat    = gfx_pattern_bayer_4x4(ptbase - rh * 4); // pat no. 12, 16
            ctx.pat    = gfx_pattern_shift(ctx.pat, 1, 0);
            i32 r      = (216 - rh * 11) + rad_off;
            i32 dt_sq  = pow2_i32(r) - pow2_i32(PLTF_DISPLAY_H / 2 - y);
            i32 x1     = PLTF_DISPLAY_W;

            if (0 < dt_sq) {
                i32 dt = sqrt_i32(dt_sq);
                i32 x2 = PLTF_DISPLAY_W / 2 + dt + off_x;
                x1     = PLTF_DISPLAY_W / 2 - dt + off_x;
                gfx_rec_strip(ctx, x2, y, PLTF_DISPLAY_W, PRIM_MODE_BLACK);
            }
            gfx_rec_strip(ctx, 0, y, x1, PRIM_MODE_BLACK);
        }
    }
}

void render_hero_ui(g_s *g, obj_s *ohero, v2_i32 camoff)
{
    fnt_s              font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s          ctx    = gfx_ctx_display();
    hero_s            *h      = (hero_s *)ohero->heap;
    v2_i32             posh   = v2_i32_add(obj_pos_center(ohero), camoff);
    hero_interaction_s hi     = hero_get_interaction(g, ohero);

    render_hurt_ui(g, ohero);

    switch (hi.action) {
    case HERO_INTERACTION_INTERACT: {
        obj_s *oi = obj_from_obj_handle(hi.interact);
        if (!oi) break;

        v2_i32 pbt = v2_i32_add(camoff, obj_pos_center(oi));
        pbt.x -= 48 / 2 + oi->interact_offs.x;
        pbt.y -= 48 / 2 + oi->interact_offs.y;
        texrec_s trb = asset_texrec(TEXID_BUTTONS,
                                    48,
                                    48 * (1 - ani_frame(ANIID_BUTTON, g->tick)),
                                    48,
                                    48);
        gfx_spr(ctx, trb, pbt, 0, 0);
        break;
    }
    }

    render_stamina_ui(g, ohero, camoff);
    coins_draw(g);
    render_itemswap(ohero, camoff);
}

void render_itemswap(obj_s *ohero, v2_i32 camoff)
{
    hero_s *h = (hero_s *)ohero->heap;
    if (!h->item_swap_tick && !h->item_swap_fade) return;

    spm_push();
    i32   ticks_progress = max_i32(h->item_swap_tick, h->item_swap_tick_saved);
    tex_s thold          = tex_create(96, 32, 1, spm_allocator(), 0);
    tex_clr(thold, GFX_COL_CLEAR);

    bool32    swapped = 0 < h->item_swap_fade;
    gfx_ctx_s ctx     = gfx_ctx_display();
    gfx_ctx_s ctxhold = gfx_ctx_default(thold);
    v2_i32    pswapl  = {12, 0};
    v2_i32    pbbut   = {0, -5};
    texrec_s  trswap  = asset_texrec(TEXID_BUTTONS, 32, 160, 64, 32);
    texrec_s  trbbut  = asset_texrec(TEXID_BUTTONS, 160 + 32, 64, 32, 32);

    // "swap" label
    gfx_spr(ctxhold, trswap, pswapl, 0, 0);

    if (0 < ticks_progress) {
        // "swap" label inverted -> progress bar
        trswap.y += 32;
        trswap.w = ease_out_quad(0, trswap.w, ticks_progress, HERO_ITEMSWAP_TICKS);
        i32 mode = SPR_MODE_XOR;

        // brief blinking of "swap" label after swapping
        if (10 < h->item_swap_fade &&
            h->item_swap_fade <= HERO_ITEMSWAP_TICKS_FADE &&
            ((h->item_swap_fade / 3) & 1) == 0) {
            mode = SPR_MODE_WHITE;
        }
        gfx_spr(ctxhold, trswap, pswapl, 0, mode);
    }

    // B button image
    if (!inp_btn(INP_B)) {
        trbbut.x -= 32;
    }
    gfx_spr(ctxhold, trbbut, pbbut, 0, 0);

    // final image
    gfx_ctx_s ctxhold_res = ctx;
    v2_i32    posswap     = v2_i32_add(obj_pos_center(ohero), camoff);
    posswap.x -= 40;
    posswap.y += 20;

    // item swap  canceled - shake left/right in denial
    if (h->item_swap_fade < 0) {
        i32 t_denial = h->item_swap_fade & 7;
        i32 of       = (t_denial & 3) == 3 ? 1 : (t_denial & 3);
        if (4 <= t_denial) {
            posswap.x += of;
        } else {
            posswap.x -= of;
        }
    }

    // fade in/out
    i32 f1 = abs_i32(h->item_swap_fade);
    i32 f2 = HERO_ITEMSWAP_TICKS_FADE >> 2;
    if (f1 && f1 < f2) {
        ctxhold_res.pat = gfx_pattern_interpolate(f1, f2);
        posswap.y += (f2 - f1) >> 1;
    } else if (h->item_swap_tick && h->item_swap_tick < 5) {
        ctxhold_res.pat = gfx_pattern_interpolate(h->item_swap_tick, 5);
        posswap.y += 4 - h->item_swap_tick;
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
    fnt_draw_str(ctx, font_1, loc, g->areaname.label, SPR_MODE_BLACK);
    tex_outline_white(ctx.dst);
    tex_outline_white(ctx.dst);
#endif
}

void render_pause(g_s *g)
{
}