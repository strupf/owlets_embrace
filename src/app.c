// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "game.h"
#include "inp.h"
#include "spm.h"
#include "sys/sys.h"

typedef game_s GAME_s;
GAME_s         GAME;

void app_init()
{
    spm_init();
    assets_init();

    sys_printf("size RES: %u kb\n", (uint)sizeof(ASSETS_s) / 1024);
    sys_printf("size SPM: %u kb\n", (uint)sizeof(SPM_s) / 1024);
    sys_printf("size GAM: %u kb\n", (uint)sizeof(GAME_s) / 1024);
    sys_printf("  = %u kb\n", (uint)(sizeof(ASSETS_s) +
                                     sizeof(SPM_s) +
                                     sizeof(GAME_s)) /
                                  1024);

    asset_tex_put(TEXID_DISPLAY, tex_framebuffer());
    asset_tex_load(TEXID_TILESET, "assets/tileset.tex");
    asset_tex_load(TEXID_HERO, "assets/player.tex");
    asset_tex_load(TEXID_UI, "assets/ui.tex");
    asset_tex_load(TEXID_UI_ITEMS, "assets/items.tex");
    asset_tex_put(TEXID_UI_ITEM_CACHE, tex_create(128, 256, assetmem_alloc));
    asset_tex_load(TEXID_UI_TEXTBOX, "assets/textbox.tex");

    asset_snd_load(SNDID_HOOK_ATTACH, "assets/snd/hookattach.wav");
    asset_fnt_load(FNTID_DEFAULT, "assets/font_default.json");
    asset_fnt_load(FNTID_DIALOG, "assets/font_dialog.json");

#ifdef SYS_DEBUG
    tex_s tcoll = tex_create(16, 16 * 32, assetmem_alloc);
    asset_tex_put(TEXID_COLLISION_TILES, tcoll);
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

    spriteanimdata_s hd = {0};
    hd.frames           = assetmem_alloc(sizeof(spriteanimframe_s) * 4);
    hd.frames[0].ticks  = 10;
    hd.frames[1].x      = 64;
    hd.frames[1].ticks  = 10;
    hd.frames[2].x      = 128;
    hd.frames[2].ticks  = 10;
    hd.frames[3].x      = 64 + 128;
    hd.frames[3].ticks  = 10;
    hd.w                = 64;
    hd.h                = 64;
    hd.tex              = asset_tex(TEXID_HERO);
    hd.n_frames         = 4;

    asset_anim_put(ANIMID_HERO, hd);

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
}

void app_draw()
{
    sys_display_update_rows(0, SYS_DISPLAY_H - 1);
    tex_clr(asset_tex(0), TEX_CLR_WHITE);

    game_s *g = &GAME;
    switch (g->state) {
    case GAMESTATE_MAINMENU:
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
}

void app_pause()
{
}
