// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/gfx.h"
#include "game.h"
#include "render.h"

#define ITEM_FRAME_SIZE  64
#define ITEM_BARREL_R    16
#define ITEM_SIZE        32
#define ITEM_X_OFFS      325
#define ITEM_Y_OFFS      180
#define FNTID_AREA_LABEL FNTID_MEDIUM

void render_ui(g_s *g)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    obj_s          *ohero  = obj_get_tagged(g, OBJ_TAG_OWL);
    fnt_s           font_1 = asset_fnt(FNTID_AREA_LABEL);

    if (AREANAME_ST_DELAY < g->area_anim_st) {
        gfx_ctx_s ctx_area = ctx;

        switch (g->area_anim_st) {
        case AREANAME_ST_FADE_IN:
            ctx_area.pat = gfx_pattern_interpolate(g->area_anim_tick, AREANAME_TICKS_IN);
            break;
        case AREANAME_ST_FADE_OUT:
            ctx_area.pat = gfx_pattern_interpolate(AREANAME_TICKS_OUT - g->area_anim_tick, AREANAME_TICKS_OUT);
            break;
        }
        fnt_draw_outline_style(ctx_area, asset_fnt(FNTID_VVLARGE), (v2_i32){200, 10}, g->areaname, 2, 1);
    }
}

void render_hero_ui(g_s *g, obj_s *ohero, v2_i32 camoff)
{
    fnt_s     font_1 = asset_fnt(FNTID_AREA_LABEL);
    gfx_ctx_s ctx    = gfx_ctx_display();
    owl_s    *h      = (owl_s *)ohero->heap;
    v2_i32    posh   = v2_i32_add(obj_pos_center(ohero), camoff);
    //  hero_interaction_s hi     = hero_get_interaction(g, ohero);

    i32      hp            = ohero->health;
    i32      hp_max        = ohero->health_max;
    i32      hp_containers = hp_max >> 1;
    texrec_s trheart       = asset_texrec(TEXID_BUTTONS, 0, 0, 32, 28);
    i32      t_fade_1      = g->health_ui_fade;
    i32      t_fade_2      = min_i32(g->health_ui_fade + 20, HEALTH_UI_TICKS);

    for (i32 n = 0; n < hp_containers; n++) {
        i32 fr_x      = 0;
        i32 fr_y      = 0;
        i32 hp_n_full = 2 + (n * 2);
        i32 hp_n_half = 1 + (n * 2);

        if (hp < hp_n_half) {
            // lower full heart
            fr_x = 3;
            fr_y = 0;
        } else if (hp < hp_n_full) {
            // half
            fr_y = 1;
            fr_x = 3 + ((g->tick_gameplay >> (hp == 1 ? 4 : 5)) & 1);
        } else {
            fr_y = 0;
            if (hp == hp_n_full) {
                // full
                // current highest active heart
                i32 t = (g->tick_gameplay >> (hp == 2 && 4 <= hp_max ? 4 : 5)) & 3;
                switch (t) {
                case 0: fr_x = 0; break;
                case 1: fr_x = 1; break;
                case 2: fr_x = 0; break;
                case 3: fr_x = 2; break;
                }
            } else {
                // empty
                fr_x = -1;
            }
        }

        v2_i32 heartp = {0 + (n) * 22, 0}; // todo
        trheart.x     = (7 + fr_x) << 5;
        trheart.y     = (11 + fr_y) << 5;
        i32 t_fade    = lerp_i32(t_fade_1, t_fade_2, (hp_containers - n - 1), hp_containers - 1);
        heartp.y      = ease_out_quad(-70, heartp.y, t_fade, HEALTH_UI_TICKS);
        gfx_spr_tile_32x32(ctx, trheart, heartp);
    }

    // render_stamina_ui(g, ohero, camoff);
    coins_draw(g);
}

void render_stamina_ui(g_s *g, obj_s *o, v2_i32 camoff)
{
#if 1
    owl_s *h = (owl_s *)o->heap;
    if (!(h->stamina_upgrades && h->stamina_ui_fade_ticks)) return;

    gfx_ctx_s ctx   = gfx_ctx_display();
    gfx_ctx_s ctxfh = ctx;
    gfx_ctx_s ctxb  = ctx;
    ctxfh.pat       = gfx_pattern_50();
    i32 ft0         = h->stamina_max;
    i32 ftx         = h->stamina - h->stamina_added;
    i32 fty         = h->stamina_added;

    v2_i32 p = v2_i32_add(obj_pos_center(o), camoff);
    p.y += -42;

    i32 wi_inner  = ease_out_quad(-2, 12 + h->stamina_upgrades * 4,
                                  h->stamina_ui_fade_ticks, OWL_STAMINA_TICKS_UI_FADE);
    i32 wi_innerh = wi_inner >> 1;

    rec_i32 rfill_black = {p.x - wi_innerh - 3, p.y + 5, wi_inner + 6, 6};
    rec_i32 rfill_white = rfill_black;
    rec_i32 rfill_whalf = rfill_black;
    rfill_white.w       = lerp_i32(0, rfill_black.w, ftx, ft0);
    rfill_whalf.w       = lerp_i32(0, rfill_black.w, ftx + fty, ft0);
    if (ftx == 0) {
        i32 i    = max_i32(15000, sin_q15(pltf_cur_tick() << 13));
        ctxb.pat = gfx_pattern_interpolate(i, 32769);
    }

    gfx_rec_fill(ctxb, rfill_black, PRIM_MODE_BLACK_WHITE);
    gfx_rec_fill(ctx, rfill_white, PRIM_MODE_WHITE);
    gfx_rec_fill(ctxfh, rfill_whalf, PRIM_MODE_WHITE);

    // lines between stamina bars
    if (ftx && h->stamina_ui_fade_ticks >= OWL_STAMINA_TICKS_UI_FADE / 2) {
        for (i32 n = 1; n < h->stamina_upgrades; n++) {
            rec_i32 rline = {p.x - wi_innerh + (n * wi_inner) / h->stamina_upgrades, p.y + 6, 1, 4};
            gfx_rec_fill(ctx, rline, PRIM_MODE_BLACK);
        }
    }

    v2_i32   p_l      = {p.x - wi_innerh - 8, p.y};
    v2_i32   p_r      = {p.x + wi_innerh + 0, p.y};
    v2_i32   p_i      = {p.x - wi_innerh + 0, p.y};
    texrec_s tr_outer = asset_texrec(TEXID_BUTTONS, 40 * 8, 16 * 16, 8, 16);
    gfx_spr(ctx, tr_outer, p_l, 0, 0);
    gfx_spr(ctx, tr_outer, p_r, SPR_FLIP_X, 0);
    if (0 < wi_inner) {
        texrec_s tr_inner = asset_texrec(TEXID_BUTTONS, 41 * 8, 16 * 16, wi_inner, 16);
        gfx_spr(ctx, tr_inner, p_i, 0, 0);
    }
#endif
}