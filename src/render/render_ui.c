// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define ITEM_FRAME_SIZE 64
#define ITEM_BARREL_R   16
#define ITEM_SIZE       32
#define ITEM_X_OFFS     16
#define ITEM_Y_OFFS     16

static void render_item_selection(herodata_s *h);

void render_ui(game_s *g, v2_i32 camoff)
{
    gfx_ctx_s ctx_ui = gfx_ctx_display();

    fade_s *areaname = &g->areaname.fade;
    if (fade_phase(areaname) != 0) {
        int       fade_i       = fade_interpolate(areaname, 0, 100);
        fnt_s     areafont     = asset_fnt(FNTID_LARGE);
        gfx_ctx_s ctx_areafont = ctx_ui;
        ctx_areafont.pat       = gfx_pattern_interpolate(fade_i, 100);

        int areax = 10;
        int areay = 10;

        for (int yy = -2; yy <= +2; yy++) {
            for (int xx = -2; xx <= +2; xx++) {
                fnt_draw_ascii(ctx_areafont, areafont, (v2_i32){areax + xx, areay + yy},
                               g->areaname.label, SPR_MODE_WHITE);
            }
        }
        fnt_draw_ascii(ctx_areafont, areafont, (v2_i32){areax, areay}, g->areaname.label, SPR_MODE_BLACK);
    }

    if (g->herodata.aquired_items) {
        render_item_selection(&g->herodata);
        texrec_s tr_items = asset_texrec(TEXID_UI_ITEM_CACHE,
                                         0, 0, 128, ITEM_FRAME_SIZE);
        gfx_spr(ctx_ui, tr_items, (v2_i32){400 - 92, 240 - 64 + 16}, 0, 0);
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && g->textbox.state == TEXTBOX_STATE_INACTIVE) {
        v2_i32 posc         = obj_pos_center(ohero);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            v2_i32 posi = obj_pos_center(interactable);
            posi        = v2_add(posi, camoff);
            posi.y -= 64 + 16;
            posi.x -= 32;
            int      btn_frame = tick_to_index_freq(g->tick, 2, 50);
            texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
            gfx_spr(ctx_ui, tui, posi, 0, 0);
        }
    }

    textbox_draw(&g->textbox, camoff);

    texrec_s healthbar = asset_texrec(TEXID_UI, 0, 64, 8, 16);
    for (int i = 0; i < 10; i++) {
        v2_i32 barpos = {i * 6, 0};
        gfx_spr(ctx_ui, healthbar, barpos, 0, 0);
    }
}

void render_pause(game_s *g)
{
    spm_push();
    tex_s     tex = tex_create(SYS_DISPLAY_W, SYS_DISPLAY_H, spm_allocator);
    gfx_ctx_s ctx = gfx_ctx_default(tex);

    for (int i = 0; i < SYS_DISPLAY_H * SYS_DISPLAY_WBYTES; i++) {
        tex.px[i] = rngr_i32(0, 255);
    }

    sys_set_menu_image(tex.px, tex.h, tex.wbyte);
    spm_pop();
}

static void render_item_selection(herodata_s *h)
{
    tex_s     texcache = asset_tex(TEXID_UI_ITEM_CACHE);
    gfx_ctx_s ctx      = gfx_ctx_default(texcache);
    texrec_s  tr       = {0};
    tr.t               = asset_tex(TEXID_UI_ITEMS);
    tex_clr(texcache, TEX_CLR_TRANSPARENT);

    // barrel background

    int offs = 15;
    if (!h->itemselection_decoupled) {
        int dtang = abs_i(inp_crank_calc_dt_q16(h->item_angle, inp_crank_q16()));
        offs      = min_i(pow2_i32(dtang) / 4000000, offs);
    }

#define ITEM_OVAL_Y 12

    int turn1 = (inp_crank_q16() + 0x4000) << 2;
    int turn2 = (h->item_angle + 0x4000) << 2;
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
        int aimg = (abar + h->item_angle + 0x4000) & 0xFFFF;
        if (0 <= aimg && aimg < 0x8000) { // maps to [0, 180) deg (visible barrel)
            tr.r.y = h->selected_item * ITEM_SIZE + (aimg * ITEM_SIZE) / 0x8000;
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