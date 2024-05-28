// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "shop.h"
#include "game.h"

#define SHOP_Y_SPACING 32
#define SHOP_FADE      15
#define SHOP_TICKS_BUY 60

bool32 shop_active(game_s *g)
{
    return g->shop.active;
}

void shop_open(game_s *g)
{
    shop_s *shop  = &g->shop;
    *shop         = (shop_s){0};
    shop->fade_in = 1;
    shop->active  = 1;
}

void shop_update(game_s *g)
{
    shop_s *shop = &g->shop;
    if (shop->fade_in) {
        shop->fade_in++;
        if (SHOP_FADE <= shop->fade_in) {
            shop->fade_in = 0;
        }
        return;
    }

    if (shop->fade_out) {
        shop->fade_out++;
        if (SHOP_FADE <= shop->fade_out) {
            shop->fade_out = 0;
            shop->active   = 0;
        }
        return;
    }

    if (inp_action_jp(INP_B)) {
        shop->fade_out = 1;
        return;
    }

    shop->show_interpolator_q8 >>= 1;

    if (shop->buyticks) {
        if (inp_action(INP_A)) {
            shop->buyticks++;
            if (SHOP_TICKS_BUY <= shop->buyticks) {
                snd_play_ext(SNDID_SELECT, 1.f, 1.f);
                shopitem_s *item = &shop->items[shop->selected];
                item->count -= shop->buycount;
                shop->buycount = 1;
                shop->buyticks = 0;
            }
        } else {
            shop->buyticks = max_i(shop->buyticks - 4, 0);
        }
        return;
    }

    if (inp_action_jp(INP_A) && shop->items[shop->selected].count) {
        shop->buyticks = 1;
        return;
    }

    if (inp_action_jp(INP_DD) || inp_action_jp(INP_DU)) {
        int tmp = shop->selected;
        shop->selected += inp_y();
        shop->selected = clamp_i(shop->selected, 0, shop->n_items - 1);
        if (shop->selected == tmp) {
            snd_play_ext(SNDID_MENU_NONEXT_ITEM, 1.f, 1.f);
            shop->show_interpolator_q8 = inp_y() * 24;
        } else {
            snd_play_ext(SNDID_MENU_NEXT_ITEM, 1.f, 1.f);
            if (shop->selected < shop->shows_i1) {
                shop->shows_i1--;
                shop->show_interpolator_q8 = +256;
            }
            if (shop->selected > shop->shows_i1 + 4) {
                shop->shows_i1++;
                shop->show_interpolator_q8 = -256;
            }
        }
    }
}

static void shop_draw_item(gfx_ctx_s ctxf, shop_s *shop, int n, int offs)
{
    v2_i32 pos = {40,
                  40 + n * SHOP_Y_SPACING + offs};
    if (0 < n) {
        gfx_rec_fill(ctxf, (rec_i32){32, pos.y - 10, PLTF_DISPLAY_W - 64, 2}, PRIM_MODE_BLACK);
    }
    fnt_s fnt_l = asset_fnt(FNTID_LARGE);

    shopitem_s *item = &shop->items[n];
    if (item->count == 0) {
        // sold out
    }

    if (shop->selected == n) {
        gfx_rec_fill(ctxf, (rec_i32){32, pos.y - 6, PLTF_DISPLAY_W - 64, SHOP_Y_SPACING - 8}, PRIM_MODE_BLACK);
        int dd = (shop->buyticks * 20) / SHOP_TICKS_BUY;
        gfx_cir_fill(ctxf, (v2_i32){pos.x - 10, pos.y}, dd, PRIM_MODE_INV);
    }

    fnt_draw_ascii(ctxf, fnt_l, pos, shop->items[n].name, SPR_MODE_XOR);
}

void shop_draw(game_s *g)
{
    shop_s   *shop = &g->shop;
    gfx_ctx_s ctxd = gfx_ctx_display();

    int num = SHOP_FADE;
    if (shop->fade_in) {
        num = shop->fade_in;
    }
    if (shop->fade_out) {
        num = SHOP_FADE - shop->fade_out;
    }
    ctxd.pat       = gfx_pattern_interpolate(num, SHOP_FADE);
    texrec_s tshop = asset_texrec(TEXID_UI, 0, 304, 384, 240);
    gfx_spr(ctxd, tshop, (v2_i32){0, 0}, 0, 0);

    gfx_ctx_s ctxf = gfx_ctx_clip(ctxd, 0, 32, PLTF_DISPLAY_W, PLTF_DISPLAY_H - 32);

    int offs = shop->shows_i1 * SHOP_Y_SPACING + ((shop->show_interpolator_q8 * SHOP_Y_SPACING) >> 8);
    for (int n = 0; n < shop->n_items; n++) {
        shop_draw_item(ctxf, shop, n, -offs);
    }
}
