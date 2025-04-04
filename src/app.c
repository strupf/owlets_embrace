// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "app_load.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "render.h"
#include "textinput.h"

app_s *APP;

void app_menu_callback_map(void *ctx, i32 opt)
{
    if (!APP->game.minimap.state) {
        minimap_open_via_menu(&APP->game);
    }
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

    i32 x_half   = 40 >> 1;
    i32 w_attach = 50;
    i32 w_mid    = 70;
    for (i32 x = 0; x < x_half; x++) {

        i32 a_q16 = ((w_attach - w_mid) << 16) / pow_i32(x_half, 3);

        i32 w = ((a_q16 * pow_i32(-(x - x_half), 3)) >> 16) + w_mid;
        // return w;
        //   pltf_log("%i\n", w);
    }

    pltf_audio_set_volume(1.f);
    pltf_accelerometer_set(1);
#if PLTF_PD
    pltf_pd_menu_add("settings", app_menu_callback_settings, 0);
#endif
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
    // mus_play("MUS_05");
    g->save_slot = 0;
    spm_push();
    savefile_s  *s  = spm_alloctz(savefile_s, 1);
    hero_spawn_s hs = {0};
    void        *f  = wad_open_str("SPAWN", 0, 0);
    s->save[SAVE_EV_COMPANION_FOUND / 32] |=
        (u32)1 << (SAVE_EV_COMPANION_FOUND & 31);

    pltf_file_r_checked(f, &hs, sizeof(hero_spawn_s));
    pltf_file_close(f);
    {
        str_cpy(s->name, "Lukas");
        s->map_hash   = hs.hash;
        s->hero_pos.x = hs.x;
        s->hero_pos.y = hs.y;
        s->upgrades   = 0xFFFFFFFFU;
        s->stamina    = 5;
        s->hero_pos.x = hs.x;
        s->hero_pos.y = hs.y;
    }
    savefile_w(0, s);
    spm_pop();

    title_init(&APP->title);

#if TITLE_SKIP_TO_GAME
    title_start_game(APP, 0);
#endif

#endif
    pltf_log("\nAPP MEM remaining: %i %%\n\n",
             (i32)((100 * marena_rem(&APP->ma)) / APP->ma.bufsize));
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

    inp_update();
    if (textinput_active()) {
        textinput_update();
        return;
    }

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
}

void app_draw()
{
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

    if (textinput_active()) {
        textinput_draw();
    }

#if 0
    tex_clr(ctx.dst, GFX_COL_BLACK);

    fnt_s f     = asset_fnt(FNTID_MEDIUM);
    char  ch[2] = {'A'};

    i32 nx = 8;

    for (i32 y = 0; y < 3; y++) {
        for (i32 x = 0; x < nx; x++) {
            fnt_draw_str(ctx, f, (v2_i32){25 + (x * 380) / nx, 100 + 36 * y}, ch, SPR_MODE_WHITE);
            ch[0]++;
        }
    }
#endif
}

void app_close()
{
    aud_destroy();
    if (APP) {
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