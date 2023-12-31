// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "game.h"
#include "sys/sys.h"

typedef game_s GAME_s;
GAME_s         GAME;

void app_init()
{
    spm_init();
    assets_init();

    usize size_tabs = sizeof(g_animated_tiles) +
                      sizeof(g_pxmask_tab) +
                      sizeof(tilecolliders) +
                      sizeof(g_item_desc) +
                      sizeof(AUD_s);
    sys_printf("size RES: %u kb\n", (uint)sizeof(ASSETS_s) / 1024);
    sys_printf("size SPM: %u kb\n", (uint)sizeof(SPM_s) / 1024);
    sys_printf("size GAM: %u kb\n", (uint)sizeof(GAME_s) / 1024);
    sys_printf("size TAB: %u kb\n", (uint)(size_tabs / 1024));
    sys_printf("  = %u kb\n\n", (uint)(sizeof(ASSETS_s) +
                                       sizeof(SPM_s) +
                                       sizeof(GAME_s) +
                                       size_tabs) /
                                    1024);

    asset_tex_putID(TEXID_DISPLAY, tex_framebuffer());
    asset_tex_loadID(TEXID_TILESET_TERRAIN, "tileset.tex", NULL);
    asset_tex_loadID(TEXID_TILESET_BG, "tileset_bg.tex", NULL);
    tex_s texhero;
    if (0 <= asset_tex_loadID(TEXID_HERO, "player.tex", &texhero)) {
        for (int y = 0; y < 6; y++) {
            for (int x = 0; x < 4; x++) {
                tex_outline(texhero, x * 64, y * 64, 64, 64, 1, 1);
            }
        }
    }

    asset_tex_loadID(TEXID_UI, "ui.tex", NULL);
    asset_tex_loadID(TEXID_UI_ITEMS, "items.tex", NULL);
    tex_s texswitch;
    if (0 <= asset_tex_loadID(TEXID_SWITCH, "switch.tex", &texswitch)) {
        for (int x = 0; x < 4; x++) {
            tex_outline(texswitch, x * 64, 0, 64, 64, 1, 1);
        }
    }
    asset_tex_putID(TEXID_UI_ITEM_CACHE, tex_create(128, 256, asset_allocator));

    asset_tex_loadID(TEXID_UI_TEXTBOX, "textbox.tex", NULL);
    asset_tex_loadID(TEXID_HERO_WHIP, "attackanim-sheet.tex", NULL);
    asset_tex_loadID(TEXID_PLANTS, "plants.tex", NULL);
    asset_tex_loadID(TEXID_CLOUDS, "clouds.tex", NULL);
    asset_tex_loadID(TEXID_TITLE, "title.tex", NULL);
    asset_tex_loadID(TEXID_BACKGROUND, "background_forest.tex", NULL);
    asset_tex_loadID(TEXID_TOGGLEBLOCK, "toggleblock.tex", NULL);
    asset_tex_loadID(TEXID_SHROOMY, "shroomysheet.tex", NULL);
    asset_tex_loadID(TEXID_CRAWLER, "crawler.tex", NULL);

#ifdef SYS_DEBUG
    tex_s tcoll = tex_create(16, 16 * 32, asset_allocator);
    asset_tex_putID(TEXID_COLLISION_TILES, tcoll);
    {
        gfx_ctx_s ctxcoll = gfx_ctx_default(tcoll);
        for (int t = 0; t < 22; t++) {
            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 16; j++) {
                    if (g_pxmask_tab[t * 16 + i] & (0x8000 >> j)) {
                        rec_i32 pr = {j, i + t * 16, 1, 1};
                        gfx_rec_fill(ctxcoll, pr, PRIM_MODE_BLACK);
                    }
                }
            }
        }
    }
#endif

    asset_fnt_loadID(FNTID_SMALL, "font_small.json", NULL);
    asset_fnt_loadID(FNTID_MEDIUM, "font_med.json", NULL);
    asset_fnt_loadID(FNTID_LARGE, "font_large.json", NULL);

    asset_snd_loadID(SNDID_HOOK_ATTACH, "hookattach.wav", NULL);
    asset_snd_loadID(SNDID_SPEAK, "speak.wav", NULL);
    asset_snd_loadID(SNDID_STEP, "step.wav", NULL);
    asset_snd_loadID(SNDID_SWITCH, "switch.wav", NULL);
    asset_snd_loadID(SNDID_WHIP, "whip.wav", NULL);
    asset_snd_loadID(SNDID_SWOOSH, "swoosh_0.wav", NULL);

    //    sys_load_wavdata("assets/snd/hookattach_lo.wav", spm_allocator);

    mainmenu_init(&GAME.mainmenu);
    game_init(&GAME);
}

void app_tick()
{
    inp_update();
    game_s *g = &GAME;
    switch (g->state) {
    case GAMESTATE_MAINMENU:
        mainmenu_update(g, &g->mainmenu);
        break;
    case GAMESTATE_GAMEPLAY:
        game_tick(g);
        break;
    }
    aud_update();
}

void app_draw()
{
    sys_display_update_rows(0, SYS_DISPLAY_H - 1);
    tex_clr(asset_tex(0), TEX_CLR_WHITE);

#if 1
    game_s *g = &GAME;
    switch (g->state) {
    case GAMESTATE_MAINMENU:
        mainmenu_render(&g->mainmenu);
        break;
    case GAMESTATE_GAMEPLAY:
        game_draw(g);
        break;
    }
#else
    gfx_ctx_s ctx   = gfx_ctx_default(asset_tex(0));
    texrec_s  props = asset_texrec(TEXID_HERO, 0, 0, 256, 256);
    gfx_spr(ctx, props, (v2_i32){0, 0}, 0, GAME.tick & 7);

    /*
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));
    rec_i32   rr  = {0, 0, 1, 1};

    int from = -80;
    int to   = 160;
    int ii   = 350;

    gfx_lin(ctx, (v2_i32){0, 10}, (v2_i32){400, 10}, 0);
    gfx_lin(ctx, (v2_i32){0, 230}, (v2_i32){400, 230}, 0);
    gfx_lin(ctx, (v2_i32){10, 0}, (v2_i32){10, 240}, 0);
    gfx_lin(ctx, (v2_i32){10 + ii, 0}, (v2_i32){10 + ii, 240}, 0);

    for (int i = 0; i <= ii; i++) {

        int e = ease_in_out_quad(from, to, i, ii);
        rr.x  = 10 + i;
        rr.y  = 230 - ((e - from) * 220) / (to - from);
        gfx_rec_fill(ctx, rr, PRIM_MODE_BLACK);
    }
    */
#endif
}

void app_close()
{
}

void app_resume()
{
    game_resume(&GAME);
}

void app_pause()
{
    game_paused(&GAME);
}