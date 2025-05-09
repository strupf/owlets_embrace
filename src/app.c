// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "render.h"

app_s *APP;
#if TIMING_ENABLED
timing_s TIMING;
#endif

err32 app_load_assets();

void app_menu_callback_timing(void *ctx, i32 opt)
{
#if TIMING_ENABLED
    TIMING.show = opt;
#endif
}

void app_menu_callback_resetsave(void *ctx, i32 opt)
{
    cs_resetsave_enter(&APP->game);
}

void app_menu_callback_map(void *ctx, i32 opt)
{
    if (!APP->game.minimap.state) {
        minimap_open_via_menu(&APP->game);
    }
}

void app_menu_callback_mus(void *ctx, i32 opt)
{
}

void app_menu_callback_settings(void *ctx, i32 opt)
{
}

i32 app_init()
{
    // allocate majority of memory dynamically so
    // a proper "not enough memory" error can be reported
    APP = (app_s *)pltf_mem_alloc_aligned(sizeof(app_s), APP_STRUCT_ALIGNMENT);
    if (!APP) {
        return 1;
    }
    mclr(APP, sizeof(app_s));

    err32 err_wad_core = wad_init_file("oe.wad");
    if (err_wad_core != 0) {
        return (err_wad_core | ASSETS_ERR_WAD_INIT);
    }

    err32 err_wad_mus = 0;
    err_wad_mus |= wad_init_file("oe_mus_stereo.wad");
    if (err_wad_mus) {
        // more like a warning
    }

    pltf_audio_set_volume(1.f);
    pltf_accelerometer_set(1);
    err32 err_settings = settings_load(&SETTINGS); // try to use settings file
    aud_init();
    assets_init();
    marena_init(&APP->ma, APP->mem, sizeof(APP->mem));
    inp_init();

    err32 err_assets = app_load_assets();
    app_set_mode(SETTINGS.mode);

    spm_init();
    g_s *g = &APP->game;
    game_init(g);
    savefile_s *s = &APP->save;
    g->savefile   = s;

#if TIMING_ENABLED
    TIMING.show = TIMING_SHOW_DEFAULT;
#endif
#if PLTF_PD
#if TIMING_ENABLED
    pltf_pd_menu_add_check("Timings", TIMING_SHOW_DEFAULT, app_menu_callback_timing, 0);
#endif
#endif
#if 1
    typedef struct {
        u32 hash;
        i32 x;
        i32 y;
    } hero_spawn_s;

    savefile_del(0);
    savefile_del(1);
    savefile_del(2);

    // create a playtesting savefile

    hero_spawn_s hs = {0};
    void        *f  = wad_open_str("SPAWN", 0, 0);
    pltf_file_r_checked(f, &hs, sizeof(hero_spawn_s));
    pltf_file_close(f);

    mclr(s, sizeof(savefile_s));
    {
        str_cpy(s->name, "Demo");
        // savefile_save_event_register(s, SAVE_EV_COMPANION_FOUND);
        s->map_hash   = hs.hash;
        s->hero_pos.x = hs.x;
        s->hero_pos.y = hs.y;
        s->upgrades =
            HERO_UPGRADE_SWIM |
            HERO_UPGRADE_STOMP |
            HERO_UPGRADE_FLY |
            // HERO_UPGRADE_HOOK |
            HERO_UPGRADE_CLIMB |
            0;
        s->stamina = 2;
    }
    savefile_w(0, s);

    title_init(&APP->title);
#if !TITLE_SKIP_TO_GAME
    mus_play_extv("M_SHOWCASE", 0, 0, 0, 500, 256);
#endif
#endif
    usize mrem = marena_rem(&APP->ma);
    pltf_log("\nAPP MEM remaining: %i%% (%i kB)\n\n",
             (i32)((100 * mrem) / APP->ma.bufsize),
             (i32)(mrem / 1024));
    pltf_sync_timestep();
    return 0;
}

static void app_tick_step();

void app_tick()
{
    app_tick_step();
    aud_cmd_queue_commit();
}

static void app_tick_step()
{
#if PLTF_DEV_ENV
    // slowmotion during dev
    static u32 skiptick;
    if (pltf_sdl_key(SDL_SCANCODE_LSHIFT)) {
        skiptick++;
        if (skiptick & 7) return;
    }

    if (pltf_sdl_jkey(SDL_GetScancodeFromKey(SDLK_PLUS))) {
        APP->aud.v_sfx_q8 = min_i32(APP->aud.v_sfx_q8 + 32, 256);
    }
    if (pltf_sdl_jkey(SDL_GetScancodeFromKey(SDLK_MINUS))) {
        APP->aud.v_sfx_q8 = max_i32(APP->aud.v_sfx_q8 - 32, 0);
    }
    if (pltf_sdl_jkey(SDL_SCANCODE_2)) {
        APP->aud.v_sfx_q8 = min_i32(APP->aud.v_sfx_q8 + 32, 256);
    }
    if (pltf_sdl_jkey(SDL_SCANCODE_1)) {
        APP->aud.v_sfx_q8 = max_i32(APP->aud.v_sfx_q8 - 32, 0);
    }
#endif
#if TIMING_ENABLED
    timing_beg(TIMING_ID_TICK);
#endif
    inp_update();

    g_s *g = &APP->game;

    switch (APP->state) {
    case APP_ST_TITLE: {
        title_update(APP, &APP->title);
        break;
    }
    case APP_ST_GAME: {
        inp_state_s istate = inp_cur().c;
        game_tick(g, istate);
        break;
    }
    }
#if TIMING_ENABLED
    timing_end(TIMING_ID_TICK);
#endif
}

void app_draw()
{
#if TIMING_ENABLED
    timing_beg(TIMING_ID_DRAW);
#endif
#if PLTF_PD
    pltf_pd_update_rows(0, 239);
#endif
    tex_clr(asset_tex(0), GFX_COL_WHITE);
    gfx_ctx_s ctx = gfx_ctx_display();
    g_s      *g   = &APP->game;

    switch (APP->state) {
    case APP_ST_TITLE:
        title_render(&APP->title);
        break;
    case APP_ST_GAME:
        game_draw(g);
        break;
    }

    settings_menu_s *sm = &APP->settings_menu;
    if (sm->active) {
        settings_menu_draw(sm);
    }
#if TIMING_ENABLED
    timing_end(TIMING_ID_DRAW);
    timing_end_frame();
#endif
}

void app_close()
{
    aud_destroy();
    if (APP) {
        if (APP->state == APP_ST_GAME) {
            g_s *g = &APP->game;
            game_update_savefile(g);
            savefile_w(g->save_slot, g->savefile);
        }
        pltf_mem_free_aligned(APP);
    }
}

void app_resume()
{
    inp_on_resume();
    game_resume(&APP->game);
}

void app_pause()
{
#if PLTF_PD
    if (APP->state == APP_ST_TITLE) {
        title_paused(&APP->title);
    } else {
        game_paused(&APP->game);
    }
#endif
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    aud_audio(&APP->aud, lbuf, rbuf, len);
}

void *app_alloc(usize s)
{
    void *mem = marena_alloc(&APP->ma, s);
    assert(mem);
    mclr(mem, s);
    return mem;
}

void *app_alloc_aligned(usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&APP->ma, s, alignment);
    assert(mem);
    mclr(mem, s);
    return mem;
}

void *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment)
{
    return app_alloc_aligned(s, alignment);
}

void app_set_mode(i32 mode)
{
    switch (mode) {
    case SETTINGS_MODE_NORMAL: {
        pltf_set_fps_mode(PLTF_FPS_MODE_UNCAPPED);
        break;
    }
    case SETTINGS_MODE_STREAMING: {
        pltf_set_fps_mode(PLTF_FPS_MODE_UNCAPPED);
        break;
    }
    case SETTINGS_MODE_POWER_SAVING: {
        pltf_set_fps_mode(PLTF_FPS_MODE_40);
        break;
    }
    case SETTINGS_MODE_30_FPS: {
        pltf_set_fps_mode(PLTF_FPS_MODE_30);
        break;
    }
    }
}