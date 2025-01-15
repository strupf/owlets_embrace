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

void  *APP_MEM_RAW;
app_s *APP;

i32 app_init()
{
    // allocate majority of memory dynamically so
    // a proper "not enough memory" error can be reported
    APP_MEM_RAW = pltf_mem_alloc(sizeof(app_s) + APP_STRUCT_ALIGNMENT);
    if (!APP_MEM_RAW) {
        return APP_ERR_MEM;
    }
    APP = (app_s *)align_ptr(APP_MEM_RAW, APP_STRUCT_ALIGNMENT);
    mclr(APP, sizeof(app_s));
    spm_init();

    i32 err_wad_core = 0;
    err_wad_core |= wad_init_file("oe.wad");

    if (err_wad_core != 0) {
        return err_wad_core;
    }

    i32 err_wad_mus = 0;
    err_wad_mus |= wad_init_file("oe_mus_mono.wad");
    err_wad_mus |= wad_init_file("oe_mus_stereo.wad");

    aud_init();
    assets_init();
    marena_init(&APP->ma, APP->mem, sizeof(APP->mem));
    pltf_audio_set_volume(1.f);
    pltf_accelerometer_set(1);
    inp_init();
    settings_load(); // try to use a user settings file, or use the defaults
    g_s *g = &APP->game;
    title_init(&APP->title);
    game_init(g);
    app_load_assets();

#if 1
    // create a playtesting savefile
    mus_play("MUS003");
    g->save_slot = 0;
    spm_push();
    save_s *s = spm_alloctz(save_s, 1);
    {
        s->map_hash   = wad_hash("L_START");
        s->hero_pos.x = 100;
        s->hero_pos.y = 100;
        s->upgrades   = 0xFFFFFFFFU;
        s->stamina    = 5;
    }
    savefile_w(0, s);
    spm_pop();
    game_load_savefile(g);
#endif

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
#endif
    inp_update();
    if (textinput_active()) {
        textinput_update();
        return;
    }

    g_s *g = &APP->game;

    switch (APP->state) {
    case APP_ST_LOAD_INIT: {
        APP->state = APP_ST_LOAD;
        break;
    }
    case APP_ST_LOAD: {
        APP->state = APP_ST_GAME;
        break;
    }
    case APP_ST_TITLE: {
        title_update(g, &APP->title);
        break;
    }
    case APP_ST_GAME: {
        game_tick(g);
        break;
    }
    }
}

void app_draw()
{
#ifdef PLTF_PD
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

    if (textinput_active()) {
        textinput_draw();
    }
}

void app_close()
{
    aud_destroy();
    if (APP_MEM_RAW) {
        pltf_mem_free(APP_MEM_RAW);
    }
}

void app_resume()
{
    inp_on_resume();
    game_resume(&APP->game);
}

void app_pause()
{
    game_paused(&APP->game);
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    aud_audio(lbuf, rbuf, len);
}

void *app_alloc(usize s)
{
    void *mem = marena_alloc(&APP->ma, s);
    assert(mem);
    return mem;
}

void *app_alloc_aligned(usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&APP->ma, s, alignment);
    assert(mem);
    return mem;
}

void *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment)
{
    void *mem = marena_alloc_aligned((marena_s *)ctx, s, alignment);
    assert(mem);
    return mem;
}