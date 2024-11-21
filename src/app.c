// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "render.h"
#include "textinput.h"

app_s APP;

#if PLTF_DEV_ENV
void app_load_tex();
void app_load_fnt();
void app_load_snd();
#endif

void app_init()
{
    pltf_audio_set_volume(1.f);
    pltf_accelerometer_set(1);
    spm_init();
    assets_init();
    inp_init();

#if PLTF_DEV_ENV
    SPM.lowestleft_disabled = 1;
    app_load_tex();
    app_load_fnt();
    app_load_snd();
    assets_export();
    SPM.lowestleft_disabled = 0;
#else
    assets_import();
#endif
    g_s *g = &APP.game;

    pltf_log("Asset mem left: %u kb\n", (u32)memarena_size_rem(&ASSETS.marena) / 1024);
    u32 size_tabs = sizeof(g_tile_corners) +
                    sizeof(g_tile_tris) +
                    sizeof(AUD_s);
    pltf_log("static RAM ~%u kb\n\n", (uint)(sizeof(ASSETS_s) +
                                             sizeof(SPM_s) +
                                             sizeof(app_s) +
                                             size_tabs) /
                                          1024);

    pltf_log("size save: %u bytes\n", (uint)(sizeof(save_s)));

    title_init(&APP.title);
    game_init(g);
}

static void app_tick_step();

void app_tick()
{
    app_tick_step();
    aud_cmd_queue_commit();
}

static void app_tick_step()
{
#ifdef PLTF_SDL
    static u32 skiptick;
    if (pltf_sdl_key(SDL_SCANCODE_LSHIFT)) {
        skiptick++;
        if (skiptick & 7) return;
    }
#endif
    inp_update();
    if (textinput_active()) {
        textinput_update();
        return;
    }

    g_s *g = &APP.game;

    switch (g->state) {
    case APP_STATE_TITLE:
        title_update(g, &APP.title);
        break;
    case APP_STATE_GAME:
        game_tick(g);
        break;
    }
}

void app_draw()
{
#ifdef PLTF_PD
    pltf_pd_update_rows(0, 239);
#endif
    tex_clr(asset_tex(0), GFX_COL_WHITE);
    gfx_ctx_s ctx = gfx_ctx_display();
    g_s      *g   = &APP.game;

    switch (g->state) {
    case APP_STATE_TITLE:
        title_render(&APP.title);
        break;
    case APP_STATE_GAME:
        game_draw(g);
        break;
    }

    if (textinput_active()) {
        textinput_draw();
    }

#if GFX_ENDIAN_AT_END
    u32 *p = pltf_1bit_buffer();
    for (u32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n += 4) {
        *p = bswap32(*p), p++;
        *p = bswap32(*p), p++;
        *p = bswap32(*p), p++;
        *p = bswap32(*p), p++;
    }
#endif
}

void app_close()
{
}

void app_resume()
{
    inp_on_resume();
    game_resume(&APP.game);
}

void app_pause()
{
    game_paused(&APP.game);
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    aud_audio(lbuf, rbuf, len);
}

#if PLTF_DEV_ENV
// preprocess textures
void app_load_tex()
{
    asset_tex_loadID(TEXID_TILESET_TERRAIN, "TILESET_TERRAIN", 0);
    asset_tex_loadID(TEXID_TILESET_BG_AUTO, "TILESET_BG_AUTO", 0);
    asset_tex_loadID(TEXID_TILESET_PROPS, "TILESET_PROPS", 0);

#if PLTF_DEBUG
    tex_s tcoll = tex_create(16, 16 * 32, asset_allocator);
    asset_tex_putID(TEXID_COLLISION_TILES, tcoll);
    gfx_ctx_s ctxcoll = gfx_ctx_default(tcoll);
    for (i32 t = 0; t < NUM_TILE_SHAPES; t++) {
        for (i32 i = 0; i < 16; i++) {
            for (i32 j = 0; j < 16; j++) {
                if (tile_solid_pt(t, i, j)) {
                    rec_i32 pr = {j, i + t * 16, 1, 1};
                    gfx_rec_fill(ctxcoll, pr, PRIM_MODE_BLACK);
                }
            }
        }
    }
#endif

    asset_tex_loadID(TEXID_MAINMENU, "mainmenu", 0);
    asset_tex_loadID(TEXID_CRUMBLE, "crumbleblock", 0);
    tex_s tchest;
    asset_tex_loadID(TEXID_CHEST, "chest", &tchest);
    tex_outline(tchest, 0, 0, 96, 32, 1, 1);

    asset_tex_loadID(TEXID_UI, "ui", 0);
    asset_tex_putID(TEXID_PAUSE_TEX, tex_create_opaque(400, 240, asset_allocator));

    tex_s texswitch;
    asset_tex_loadID(TEXID_SWITCH, "switch", &texswitch);
    tex_outline(texswitch, 0, 0, 128, 64, 1, 1);

    tex_s tflyblob;
    asset_tex_loadID(TEXID_FLYBLOB, "flyblob", &tflyblob);
    tex_outline(tflyblob, 0, 0, 1024, 512, 1, 1);

    asset_tex_putID(TEXID_AREALABEL, tex_create(256, 64, asset_allocator));
    asset_tex_loadID(TEXID_JUGGERNAUT, "juggernaut", 0);
    asset_tex_loadID(TEXID_PLANTS, "plants", 0);
    asset_tex_loadID(TEXID_CRABLER, "crabler", 0);

    tex_s tcharger;
    asset_tex_loadID(TEXID_CHARGER, "charger", &tcharger);
    for (i32 y = 0; y < 3; y++) {
        for (i32 x = 0; x < 4; x++) {
            tex_outline(tcharger, x * 128, y * 64, 128, 64, 1, 1);
        }
    }

    tex_s tshroomy;
    asset_tex_loadID(TEXID_SHROOMY, "shroomy", &tshroomy);
    for (i32 y = 0; y < 4; y++) {
        for (i32 x = 0; x < 8; x++) {
            tex_outline(tshroomy, x * 64, y * 48, 64, 48, 1, 1);
        }
    }

    asset_tex_loadID(TEXID_CARRIER, "carrier", 0);
    asset_tex_loadID(TEXID_CLOUDS, "clouds", 0);
    asset_tex_loadID(TEXID_TITLE, "title", 0);
    asset_tex_loadID(TEXID_TOGGLE, "toggle", 0);
    asset_tex_loadID(TEXID_BG_CAVE, "bg_cave", 0);
    asset_tex_loadID(TEXID_BG_MOUNTAINS, "bg_mountains", 0);
    asset_tex_loadID(TEXID_BG_DEEP_FOREST, "bg_deep_forest", 0);
    asset_tex_loadID(TEXID_BG_CAVE_DEEP, "bg_cave_deep", 0);
    asset_tex_loadID(TEXID_BG_FOREST, "bg_forest", 0);
    asset_tex_loadID(TEXID_FLUIDS, "fluids", 0);

    tex_s texpl;
    asset_tex_loadID(TEXID_EXPLOSIONS, "explosions", &texpl);
    for (i32 y = 0; y < 12; y++) {
        for (i32 x = 0; x < 14; x++) {
            tex_outline(texpl, x * 64, y * 64, 64, 64, 0, 1);
        }
    }

    tex_s tskeleton;
    asset_tex_loadID(TEXID_SKELETON, "skeleton", &tskeleton);
    for (i32 y = 0; y < 2; y++) {
        for (i32 x = 0; x < 4; x++) {
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
#if 0
    tex_outline_f(texhero, 64 + 30, 64, 128, 128, 0, 1);
#else
    for (i32 y = 0; y < 32; y++) {
        if (y == 17) continue; // black outline
        for (i32 x = 0; x < 16; x++) {
            tex_outline(texhero, x * 64, y * 64, 64, 64, 1, 1);
        }
    }
#endif

    for (i32 x = 0; x < 16; x++) {
        tex_outline(texhero, x * 64, 17 * 64, 64, 64, 0, 1);
        tex_outline(texhero, x * 64, 17 * 64, 64, 64, 0, 0);
    }

    // prerender 16 rotations
    tex_s thook;
    asset_tex_loadID(TEXID_HOOK, "hook", &thook);
    texrec_s  trhook   = {thook, {0, 0, 32, 32}};
    gfx_ctx_s ctx_hook = gfx_ctx_default(thook);

    for (i32 i = 1; i < 16; i++) {
        v2_i32 origin = {16, 16};
        v2_i32 pp     = {0, i * 32};
        f32    ang    = (PI_FLOAT * (f32)i * 0.125f);
        gfx_spr_rotscl(ctx_hook, trhook, pp, origin, -ang, 1.f, 1.f);
    }
    tex_outline(thook, 0, 0, thook.w / 2, thook.h, 1, 1);

    tex_s twallworm;
    asset_tex_loadID(TEXID_WALLWORM, "wallworm", &twallworm);
    texrec_s  trwallworm_1 = {twallworm, {0, 0, 512, 32}};
    texrec_s  trwallworm   = {twallworm, {0, 0, 32, 32}};
    gfx_ctx_s ctx_wallworm = gfx_ctx_default(twallworm);
    gfx_spr(ctx_wallworm, trwallworm_1, (v2_i32){512, 0}, 0, SPR_MODE_BLACK);

#if 0
    for (i32 i = 1; i < 16; i++) {
        v2_i32 origin = {16, 16};
        f32    ang    = (PI_FLOAT * (f32)i * 0.125f);

        for (i32 n = 0; n < 18; n++) {
            v2_i32 pp      = {n * 32, i * 32};
            trwallworm.r.x = n * 32;

            gfx_spr_rotscl(ctx_wallworm, trwallworm, pp, origin, -ang, 1.f, 1.f);
            trwallworm.r.x += 512;
            pp.x += 512;
            gfx_spr_rotscl(ctx_wallworm, trwallworm, pp, origin, -ang, 1.f, 1.f);
        }
    }
#endif
    tex_outline(twallworm, 512, 0, 512, 512, 0, 1);
    tex_outline(twallworm, 512, 0, 512, 512, 0, 1);

    tex_s tbudplant;
    asset_tex_loadID(TEXID_BUDPLANT, "budplant", &tbudplant);

    for (i32 y = 0; y < 6; y++) {
        for (i32 x = 0; x < 8; x++) {
            tex_outline(tbudplant, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    gfx_ctx_s ctxbudplant = gfx_ctx_default(tbudplant);
    // 4 rotations
    for (i32 i = 1; i < 4; i++) {
        v2_i32 origin = {32, 32};
        f32    ang    = (PI_FLOAT * (f32)i * 0.5f);

        for (i32 y = 0; y < 6; y++) {
            for (i32 x = 0; x < 8; x++) {
                v2_i32   pp         = {(x + i * 8) * 64, y * 64};
                texrec_s trbudplant = {tbudplant, {x * 64, y * 64, 64, 64}};

                gfx_spr_rotscl(ctxbudplant, trbudplant, pp, origin, -ang, 1.f, 1.f);
            }
        }
    }

    asset_tex_loadID(TEXID_KEYBOARD, "keyboard", 0);

    tex_s tnpc;
    asset_tex_loadID(TEXID_NPC, "npc", &tnpc);
    for (i32 y = 0; y < 5; y++) {
        for (i32 x = 0; x < 16; x++) {
            tex_outline(tnpc, x * 64, y * 64, 64, 64, 1, 1);
        }
    }

    tex_s tflyer;
    asset_tex_loadID(TEXID_FLYER, "flyer", &tflyer);
    for (i32 y = 0; y < 1; y++) {
        for (i32 x = 0; x < 4; x++) {
            tex_outline(tflyer, x * 128, y * 96, 128, 96, 1, 1);
        }
    }

    tex_s tflyingbug;
    asset_tex_loadID(TEXID_FLYING_BUG, "flyingbug", &tflyingbug);
    gfx_ctx_s ctx_fbug = gfx_ctx_default(tflyingbug);
    texrec_s  tfbug    = {tflyingbug, {0, 96, 96 * 6, 96}};
    gfx_spr(ctx_fbug, tfbug, (v2_i32){0, 96 * 2}, 0, 0);
    for (i32 y = 0; y < 2; y++) {
        texrec_s tfbugwing = {tflyingbug, {y * 96, 0, 96, 96}};
        for (i32 x = 0; x < 6; x++) {
            gfx_spr(ctx_fbug, tfbugwing, (v2_i32){x * 96, (y + 1) * 96}, 0, 0);
        }
    }
    tex_outline(tflyingbug, 0, 96, 96 * 6, 96 * 2, 1, 1);

    // prerender 8 rotations
    asset_tex_loadID(TEXID_CRAWLER, "crawler", 0);
    {
        texrec_s  trcrawler   = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
        gfx_ctx_s ctx_crawler = gfx_ctx_default(trcrawler.t);
        for (i32 k = 0; k < 8; k++) {
            v2_i32 origin = {32, 48 - 10};
            trcrawler.r.x = k * 64;

            for (i32 i = 1; i < 8; i++) {
                v2_i32 pp  = {k * 64, i * 64};
                f32    ang = (PI_FLOAT * (f32)i * 0.25f);
                gfx_spr_rotscl(ctx_crawler, trcrawler, pp, origin, -ang, 1.f, 1.f);
            }
        }
        tex_outline(trcrawler.t, 0, 0, trcrawler.t.w, trcrawler.t.h, 1, 1);
    }
    asset_tex_loadID(TEXID_WINDGUSH, "windgush", 0);
    asset_tex_loadID(TEXID_TITLE_SCREEN, "titlescreen", 0);
    water_prerender_tiles();
}

void app_load_snd()
{
    asset_snd_loadID(SNDID_ENEMY_DIE, "enemy_die", 0);
    asset_snd_loadID(SNDID_ENEMY_HURT, "enemy_hurt", 0);
    asset_snd_loadID(SNDID_BASIC_ATTACK, "basic_attack", 0);
    asset_snd_loadID(SNDID_COIN, "coin", 0);
    asset_snd_loadID(SNDID_SHROOMY_JUMP, "shroomyjump", 0);
    asset_snd_loadID(SNDID_HOOK_ATTACH, "hookattach", 0);
    asset_snd_loadID(SNDID_SPEAK, "speak", 0);
    asset_snd_loadID(SNDID_STEP, "step", 0);
    asset_snd_loadID(SNDID_SWITCH, "switch", 0);
    asset_snd_loadID(SNDID_SWOOSH, "swoosh_0", 0);
    asset_snd_loadID(SNDID_HIT_ENEMY, "hitenemy", 0);
    asset_snd_loadID(SNDID_DOOR_TOGGLE, "doortoggle", 0);
    asset_snd_loadID(SNDID_DOOR_SQUEEK, "doorsqueek", 0);
    asset_snd_loadID(SNDID_SELECT, "select", 0);
    asset_snd_loadID(SNDID_MENU_NEXT_ITEM, "menu_next_item", 0);
    asset_snd_loadID(SNDID_MENU_NONEXT_ITEM, "menu_no_next_item", 0);
    asset_snd_loadID(SNDID_DOOR_UNLOCKED, "unlockdoor", 0);
    asset_snd_loadID(SNDID_DOOR_KEY_SPAWNED, "keyspawn", 0);
    asset_snd_loadID(SNDID_UPGRADE, "upgrade", 0);
    asset_snd_loadID(SNDID_HOOK_THROW, "throw_hook", 0);
    asset_snd_loadID(SNDID_KB_KEY, "key", 0);
    asset_snd_loadID(SNDID_KB_DENIAL, "denial", 0);
    asset_snd_loadID(SNDID_KB_CLICK, "click", 0);
    asset_snd_loadID(SNDID_KB_SELECTION, "selection", 0);
    asset_snd_loadID(SNDID_KB_SELECTION_REV, "selection-reverse", 0);
    asset_snd_loadID(SNDID_FOOTSTEP_LEAVES, "footstep_leaves", 0);
    asset_snd_loadID(SNDID_FOOTSTEP_GRASS, "footstep_grass", 0);
    asset_snd_loadID(SNDID_FOOTSTEP_MUD, "footstep_mud", 0);
    asset_snd_loadID(SNDID_FOOTSTEP_SAND, "footstep_sand", 0);
    asset_snd_loadID(SNDID_FOOTSTEP_DIRT, "footstep_dirt", 0);
    asset_snd_loadID(SNDID_OWLET_ATTACK_1, "owlet_attack_1", 0);
    asset_snd_loadID(SNDID_OWLET_ATTACK_2, "owlet_attack_2", 0);
    asset_snd_loadID(SNDID_ENEMY_EXPLO, "enemy_explo", 0);
    asset_snd_loadID(SNDID_WING, "wing_sfx", 0);
    asset_snd_loadID(SNDID_WING1, "wing_sfx1", 0);
    asset_snd_loadID(SNDID_HOOK_READY, "hook_ready", 0);
    asset_snd_loadID(SNDID_WATER_SPLASH_BIG, "water_splash_big", 0);
    asset_snd_loadID(SNDID_WATER_SPLASH_SMALL, "water_splash_small", 0);
    asset_snd_loadID(SNDID_WATER_SWIM_1, "water_swim_1", 0);
    asset_snd_loadID(SNDID_WATER_SWIM_2, "water_swim_2", 0);
    asset_snd_loadID(SNDID_WATER_OUT_OF, "water_out_of", 0);
    asset_snd_loadID(SNDID_WEAPON_UNEQUIP, "weapon_unequip", 0);
    asset_snd_loadID(SNDID_WEAPON_EQUIP, "weapon_equip", 0);
    asset_snd_loadID(SNDID_WOOSH_1, "woosh_1", 0);
    asset_snd_loadID(SNDID_WOOSH_2, "woosh_3", 0);
    asset_snd_loadID(SNDID_STOMP_START, "stomp_start", 0);
    asset_snd_loadID(SNDID_STOMP, "stomp", 0);
    asset_snd_loadID(SNDID_SKID, "skid", 0);
    asset_snd_loadID(SNDID_PROJECTILE_SPIT, "projectile_spit", 0);
    asset_snd_loadID(SNDID_PROJECTILE_WALL, "projectile_wall", 0);
}

void app_load_fnt()
{
    asset_fnt_loadID(FNTID_SMALL, "font_small", 0);
    asset_fnt_loadID(FNTID_MEDIUM, "font_med", 0);
    asset_fnt_loadID(FNTID_LARGE, "font_large", 0);
}
#endif