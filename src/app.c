// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "game.h"
#include "render/render.h"
#include "sys/sys.h"

game_s GAME;

typedef struct {
    i32 x;
} app_s;

static app_s APP;

static void app_load_tex();
static void app_load_fnt();
static void app_load_snd();
static void app_load_game();

void menu_cb_map(void *arg)
{
    game_s *g = (game_s *)arg;
    if (g->substate == 0) {
        menu_screen_open(g, &g->menu_screen);
    }
}

void menu_cb_equipment(void *arg)
{
    game_s *g = (game_s *)arg;
}

void menu_cb_item(void *arg)
{
    assert(arg);
    game_s *g = (game_s *)arg;

    int val = sys_menu_value(1);
    if (val == 0) {
        g->item.selected = HERO_ITEM_HOOK;
    }
    if (val == 1) {
        g->item.selected = HERO_ITEM_WEAPON;
    }
}

void app_set_menu_gameplay()
{
    game_s *g = &GAME;
    sys_menu_clr();
    sys_menu_item_add(0, "Map", menu_cb_map, g);
    sys_menu_item_add(0, "Equipment", menu_cb_equipment, g);
}

void app_set_menu_title()
{
    sys_menu_clr();
}

static void app_load_game()
{
    game_s *g = &GAME;

    sys_printf("Asset mem left: %u kb\n", (u32)marena_size_rem(&ASSETS.marena) / 1024);
    usize size_tabs = sizeof(g_pxmask_tab) +
                      sizeof(tilecolliders) +
                      sizeof(AUD_s);
    sys_printf("size RES: %u kb\n", (uint)sizeof(ASSETS_s) / 1024);
    sys_printf("size SPM: %u kb\n", (uint)sizeof(SPM_s) / 1024);
    sys_printf("size GAM: %u kb\n", (uint)sizeof(game_s) / 1024);
    sys_printf("size TAB: %u kb\n", (uint)(size_tabs / 1024));
    sys_printf("  = %u kb\n\n", (uint)(sizeof(ASSETS_s) +
                                       sizeof(SPM_s) +
                                       sizeof(game_s) +
                                       size_tabs) /
                                    1024);

    sys_printf("size save: %u bytes\n", (uint)(sizeof(save_s)));

    title_init(&g->title);
    game_init(g);
}

void app_init()
{
    spm_init();
    assets_init();
    aud_init();
#if defined(SYS_PD)
    assets_import();
#else
    app_load_tex();
    app_load_fnt();
    app_load_snd();
    assets_export();
#endif
    app_load_game();
    app_set_menu_title();
}

void app_tick()
{
    inp_update();

    game_s *g = &GAME;
    switch (g->state) {
    case APP_STATE_TITLE:
        title_update(g, &g->title);
        break;
    case APP_STATE_GAME:
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
    case APP_STATE_TITLE:
        title_render(&g->title);
        break;
    case APP_STATE_GAME:
        game_draw(g);
        break;
    }
}

void app_close()
{
}

void app_resume()
{
    inp_on_resume();
    game_resume(&GAME);
}

void app_pause()
{
    game_paused(&GAME);
}

void app_audio(i16 *buf, int len)
{
    aud_audio(buf, len);
}

static void app_load_tex()
{
    asset_tex_loadID(TEXID_TILESET_TERRAIN, "tileset", NULL);
    asset_tex_loadID(TEXID_TILESET_PROPS_BG, "tileset_props_bg", NULL);
    asset_tex_loadID(TEXID_TILESET_PROPS_FG, "tileset_props_fg", NULL);

#if defined(SYS_DEBUG) && 0
    tex_s tcoll = tex_create(16, 16 * 32, asset_allocator);
    asset_tex_putID(TEXID_COLLISION_TILES, tcoll);
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

#endif

    asset_tex_loadID(TEXID_MAINMENU, "mainmenu", NULL);
    asset_tex_loadID(TEXID_CRUMBLE, "crumbleblock", NULL);

    asset_tex_loadID(TEXID_UI, "ui", NULL);
    asset_tex_putID(TEXID_PAUSE_TEX, tex_create_opaque(400, 240, asset_allocator));
    asset_tex_loadID(TEXID_UI_ITEMS, "items", NULL);

    tex_s texswitch;
    asset_tex_loadID(TEXID_SWITCH, "switch", &texswitch);
    tex_outline(texswitch, 0, 0, 128, 64, 1, 1);

    asset_tex_putID(TEXID_AREALABEL, tex_create(256, 64, asset_allocator));
    asset_tex_loadID(TEXID_UI_TEXTBOX, "textbox", NULL);
    asset_tex_loadID(TEXID_JUGGERNAUT, "juggernaut", NULL);
    asset_tex_loadID(TEXID_PLANTS, "plants", NULL);
    asset_tex_loadID(TEXID_WIGGLE_DECO, "wiggle_deco", NULL);

    tex_s tcharger;
    asset_tex_loadID(TEXID_CHARGER, "charger", &tcharger);
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 4; x++) {
            tex_outline(tcharger, x * 128, y * 64, 128, 64, 1, 1);
        }
    }

    tex_s tshroomy;
    asset_tex_loadID(TEXID_SHROOMY, "shroomy", &tshroomy);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 8; x++) {
            tex_outline(tshroomy, x * 64, y * 48, 64, 48, 1, 1);
        }
    }

    asset_tex_loadID(TEXID_CARRIER, "carrier", NULL);
#if 0
    {
        texrec_s  trcarrier   = asset_texrec(TEXID_CARRIER, 0, 0, 64, 64);
        gfx_ctx_s ctx_carrier = gfx_ctx_default(trcarrier.t);
        for (int x = 0; x < 1; x++) {
            tex_outline(trcarrier.t, x * 96, 0, 96, 64, 1, 1);
        }
    }
#endif

    asset_tex_loadID(TEXID_CLOUDS, "clouds", NULL);
    asset_tex_loadID(TEXID_TITLE, "title", NULL);
    asset_tex_loadID(TEXID_TOGGLE, "toggle", NULL);
    asset_tex_loadID(TEXID_BG_CAVE, "bg_cave", NULL);
    asset_tex_loadID(TEXID_BG_MOUNTAINS, "bg_mountains", NULL);
    asset_tex_loadID(TEXID_BG_DEEP_FOREST, "bg_deep_forest", NULL);
    asset_tex_loadID(TEXID_BG_CAVE_DEEP, "bg_cave_deep", NULL);

    tex_s tskeleton;
    asset_tex_loadID(TEXID_SKELETON, "skeleton", &tskeleton);
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 4; x++) {
            tex_outline(tskeleton, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    tex_s tmisc;
    asset_tex_loadID(TEXID_MISCOBJ, "miscobj", &tmisc);
    tex_outline(tmisc, 0, 192, 64 * 2, 64, 1, 1);
    tex_outline(tmisc, 0, 384, 64 * 6, 128, 0, 1);
    tex_outline(tmisc, 0, 384, 64 * 6, 128, 0, 1);

    tex_s texhero;
    asset_tex_loadID(TEXID_HERO, "player", &texhero);
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 16; x++) {
            tex_outline(texhero, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    // prerender 16 rotations
    tex_s thook;
    asset_tex_loadID(TEXID_HOOK, "hook", &thook);
    texrec_s  trhook   = {thook, {0, 0, 32, 32}};
    gfx_ctx_s ctx_hook = gfx_ctx_default(thook);

    for (int i = 1; i < 16; i++) {
        v2_i32 origin = {16, 16};
        v2_i32 pp     = {0, i * 32};
        f32    ang    = (PI_FLOAT * (f32)i * 0.125f);
        gfx_spr_rotscl(ctx_hook, trhook, pp, origin, -ang, 1.f, 1.f);
    }
    tex_outline(thook, 0, 0, thook.w / 2, thook.h, 1, 1);

    tex_s tnpc;
    asset_tex_loadID(TEXID_NPC, "npc", &tnpc);
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 16; x++) {
            tex_outline(tnpc, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    tex_s tflyer;
    asset_tex_loadID(TEXID_FLYER, "flyer", &tflyer);
    for (int y = 0; y < 1; y++) {
        for (int x = 0; x < 4; x++) {
            tex_outline(tflyer, x * 128, y * 96, 128, 96, 1, 1);
        }
    }

    // prerender 8 rotations
    asset_tex_loadID(TEXID_CRAWLER, "crawler", NULL);
    {
        texrec_s  trcrawler   = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
        gfx_ctx_s ctx_crawler = gfx_ctx_default(trcrawler.t);
        for (int k = 0; k < 8; k++) {
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
    asset_tex_loadID(TEXID_WINDGUSH, "windgush", NULL);

    water_prerender_tiles();
}

static void app_load_snd()
{
    asset_snd_loadID(SNDID_ENEMY_DIE, "enemy_die", NULL);
    asset_snd_loadID(SNDID_ENEMY_HURT, "enemy_hurt", NULL);
    asset_snd_loadID(SNDID_BASIC_ATTACK, "basic_attack", NULL);
    asset_snd_loadID(SNDID_COIN, "coin", NULL);
    asset_snd_loadID(SNDID_ATTACK_DASH, "dash_attack", NULL);
    asset_snd_loadID(SNDID_ATTACK_SPIN, "spin_attack", NULL);
    asset_snd_loadID(SNDID_ATTACK_SLIDE_GROUND, "slide_ground", NULL);
    asset_snd_loadID(SNDID_ATTACK_SLIDE_AIR, "slide_air", NULL);
    asset_snd_loadID(SNDID_SHROOMY_JUMP, "shroomyjump", NULL);
    asset_snd_loadID(SNDID_HOOK_ATTACH, "hookattach", NULL);
    asset_snd_loadID(SNDID_SPEAK, "speak", NULL);
    asset_snd_loadID(SNDID_STEP, "step", NULL);
    asset_snd_loadID(SNDID_SWITCH, "switch", NULL);
    asset_snd_loadID(SNDID_WHIP, "whip", NULL);
    asset_snd_loadID(SNDID_SWOOSH, "swoosh_0", NULL);
    asset_snd_loadID(SNDID_HIT_ENEMY, "hitenemy", NULL);
    asset_snd_loadID(SNDID_DOOR_TOGGLE, "doortoggle", NULL);
    asset_snd_loadID(SNDID_DOOR_SQUEEK, "doorsqueek", NULL);
    asset_snd_loadID(SNDID_SELECT, "select", NULL);
    asset_snd_loadID(SNDID_MENU_NEXT_ITEM, "menu_next_item", NULL);
    asset_snd_loadID(SNDID_MENU_NONEXT_ITEM, "menu_no_next_item", NULL);
    asset_snd_loadID(SNDID_DOOR_UNLOCKED, "unlockdoor", NULL);
    asset_snd_loadID(SNDID_DOOR_KEY_SPAWNED, "keyspawn", NULL);
    asset_snd_loadID(SNDID_UPGRADE, "upgrade", NULL);
    asset_snd_loadID(SNDID_CRUMBLE, "crumble", NULL);
    asset_snd_loadID(SNDID_HOOK_THROW, "throw_hook", NULL);
}

static void app_load_fnt()
{
    asset_fnt_loadID(FNTID_SMALL, "font_small", NULL);
    asset_fnt_loadID(FNTID_MEDIUM, "font_med", NULL);
    asset_fnt_loadID(FNTID_LARGE, "font_large", NULL);
}