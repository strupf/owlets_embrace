// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "game.h"
#include "sys/sys.h"

app_s APP;

typedef game_s     GAME_s;
typedef mainmenu_s MAINMENU_s;

GAME_s     GAME;
MAINMENU_s MAINMENU;

void menu_cb_inventory(void *arg)
{
    game_s *g = (game_s *)arg;
    game_open_inventory(g);
}

void menu_cb_reduce_flicker(void *arg)
{
    bool32 b = sys_menu_checkmark(MENUITEM_REDUCE_FLICKER);
    sys_set_reduced_flicker(b);
}

void menu_setup_game()
{
    game_s *g = &GAME;
    sys_menu_clr();

    sys_menu_checkmark_add(MENUITEM_REDUCE_FLICKER, "Reduce flicker", sys_reduced_flicker(),
                           menu_cb_reduce_flicker, NULL);
    sys_menu_item_add(MENUITEM_GAME_INVENTORY, "Inventory", menu_cb_inventory, g);
}

void menu_setup_title()
{
    sys_menu_clr();

    sys_menu_checkmark_add(MENUITEM_REDUCE_FLICKER, "Reduce flicker", sys_reduced_flicker(),
                           menu_cb_reduce_flicker, NULL);
}

void app_init()
{
    spm_init();
    assets_init();
    app_load_assets();
    menu_setup_game();

    game_s *g = &GAME;

    sys_printf("Asset mem left: %u kb\n", (u32)marena_size_rem(&ASSETS.marena) / 1024);
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

    mainmenu_init(&g->mainmenu);
    game_init(g);
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
    game_s *g = &GAME;
    switch (g->state) {
    case GAMESTATE_MAINMENU:
        tex_clr(asset_tex(0), TEX_CLR_WHITE);
        mainmenu_render(&g->mainmenu);
        break;
    case GAMESTATE_GAMEPLAY:
        game_draw(g);
        break;
    }
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

void app_load_assets()
{
    asset_tex_putID(TEXID_DISPLAY, tex_framebuffer());
    asset_tex_loadID(TEXID_TILESET_TERRAIN, "tileset.tex", NULL);
    asset_tex_loadID(TEXID_TILESET_PROPS_BG, "tileset_props_bg.tex", NULL);
    asset_tex_loadID(TEXID_TILESET_PROPS_FG, "tileset_props_fg.tex", NULL);
    asset_tex_loadID(TEXID_MAINMENU, "mainmenu.tex", NULL);
    asset_tex_loadID(TEXID_BG_ART, "bg_art.tex", NULL);
    tex_s texhero;
    asset_tex_loadID(TEXID_HERO, "player.tex", &texhero);
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 4; x++) {
            tex_outline(texhero, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    asset_tex_loadID(TEXID_UI, "ui.tex", NULL);
    asset_tex_loadID(TEXID_UI_ITEMS, "items.tex", NULL);
    tex_s texswitch;
    asset_tex_loadID(TEXID_SWITCH, "switch.tex", &texswitch);
    for (int x = 0; x < 4; x++) {
        tex_outline(texswitch, x * 64, 0, 64, 64, 1, 1);
    }

    asset_tex_putID(TEXID_UI_ITEM_CACHE, tex_create(128, 256, asset_allocator));
    asset_tex_putID(TEXID_OCEAN, tex_create(400, 240, asset_allocator));
    asset_tex_putID(TEXID_AREALABEL, tex_create(256, 64, asset_allocator));

    asset_tex_loadID(TEXID_UI_TEXTBOX, "textbox.tex", NULL);
    asset_tex_loadID(TEXID_HERO_WHIP, "attackanim-sheet.tex", NULL);
    asset_tex_loadID(TEXID_PLANTS, "plants.tex", NULL);
    asset_tex_loadID(TEXID_CLOUDS, "clouds.tex", NULL);
    asset_tex_loadID(TEXID_TITLE, "title.tex", NULL);
    asset_tex_loadID(TEXID_BACKGROUND, "background_forest.tex", NULL);
    asset_tex_loadID(TEXID_TOGGLEBLOCK, "toggleblock.tex", NULL);
    asset_tex_loadID(TEXID_SHROOMY, "shroomysheet.tex", NULL);
    asset_tex_loadID(TEXID_MISCOBJ, "miscobj.tex", NULL);
    asset_tex_loadID(TEXID_BG_CAVE, "bg_cave.tex", NULL);

    tex_s tshroomy = asset_tex(TEXID_SHROOMY);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 8; x++) {
            tex_outline(tshroomy, x * 64, y * 48, 64, 48, 1, 1);
        }
    }

    // prerender 8 rotations
    asset_tex_loadID(TEXID_CRAWLER, "crawler.tex", NULL);
    {
        texrec_s  trcrawler   = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
        gfx_ctx_s ctx_crawler = gfx_ctx_default(trcrawler.t);
        for (int k = 0; k < 4; k++) {
            v2_i32 origin = {32, 48 - 10};
            trcrawler.r.x = k * 64;

            for (int i = 1; i < 8; i++) {
                v2_i32 pp  = {k * 64, i * 64};
                f32    ang = (PI_FLOAT * (f32)i * 0.25f);
                gfx_spr_rotscl(ctx_crawler, trcrawler, pp, origin, -ang, 1.f, 1.f);
            }
        }
        tex_outline(trcrawler.t, 0, 0, trcrawler.t.w, trcrawler.t.h, 1, 1);
    }

    // prerender 16 rotations
    asset_tex_loadID(TEXID_HOOK, "hook.tex", NULL);
    {
        texrec_s  thook    = asset_texrec(TEXID_HOOK, 0, 0, 32, 32);
        gfx_ctx_s ctx_hook = gfx_ctx_default(thook.t);

        for (int i = 1; i < 16; i++) {
            v2_i32 origin = {16, 16};
            v2_i32 pp     = {0, i * 32};
            f32    ang    = (PI_FLOAT * (f32)i * 0.125f);
            gfx_spr_rotscl(ctx_hook, thook, pp, origin, -ang, 1.f, 1.f);
        }
        tex_outline(thook.t, 0, 0, thook.t.w / 2, thook.t.h, 1, 1);
    }

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

    asset_snd_loadID(SNDID_SHROOMY_JUMP, "shroomyjump.wav", NULL);
    asset_snd_loadID(SNDID_HOOK_ATTACH, "hookattach.wav", NULL);
    asset_snd_loadID(SNDID_SPEAK, "speak.wav", NULL);
    asset_snd_loadID(SNDID_STEP, "step.wav", NULL);
    asset_snd_loadID(SNDID_SWITCH, "switch.wav", NULL);
    asset_snd_loadID(SNDID_WHIP, "whip.wav", NULL);
    asset_snd_loadID(SNDID_SWOOSH, "swoosh_0.wav", NULL);
    asset_snd_loadID(SNDID_HIT_ENEMY, "hitenemy.wav", NULL);
    asset_snd_loadID(SNDID_DOOR_TOGGLE, "doortoggle.wav", NULL);
    asset_snd_loadID(SNDID_DOOR_SQUEEK, "doorsqueek.wav", NULL);
}