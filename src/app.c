// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/assets.h"
#include "core/inp.h"
#include "core/spm.h"
#include "render.h"

app_s APP;

#if TIMING_ENABLED
timing_s TIMING;
#endif

static void app_on_finish_startup()
{
    app_s *a           = &APP;
    a->state           = APP_ST_TITLE;
    g_s        *g      = &a->game;
    savefile_s *s      = &a->save;
    g->savefile        = s;
    err32 err_settings = settings_load(&SETTINGS); // try to use settings file
    app_set_mode(SETTINGS.mode);
    game_init(g);
#if TIMING_ENABLED
    TIMING.show = TIMING_SHOW_DEFAULT;
#endif
#if PLTF_PD
#if TIMING_ENABLED
    pltf_pd_menu_add_check("Timings", TIMING_SHOW_DEFAULT, app_menu_callback_timing, 0);
#endif
#endif

#if 1
    // dev only ---------------------------
    typedef struct {
        u8  map_name[MAP_WAD_NAME_LEN];
        i32 x;
        i32 y;
    } owl_spawn_s;

    // delete all files
    savefile_del(0);
    savefile_del(1);
    savefile_del(2);

    // create a playtesting savefile
    owl_spawn_s hs = {0};
    void       *f  = wad_open_str("SPAWN", 0, 0);
    pltf_file_r_checked(f, &hs, sizeof(owl_spawn_s));
    pltf_file_close(f);

    mclr(s, sizeof(savefile_s));
    {
        str_cpy(s->name, "Demo");
        savefile_saveID_put(s, SAVEID_COMPANION_FOUND);
        mcpy(s->map_name, hs.map_name, sizeof(s->map_name));
        s->hero_pos.x = hs.x;
        s->hero_pos.y = hs.y;
        s->upgrades   = OWL_UPGRADE_COMPANION |
                      OWL_UPGRADE_FLY |
                      OWL_UPGRADE_CLIMB |
                      OWL_UPGRADE_HOOK;
        s->stamina    = 5;
        s->health_max = 6;
    }
    savefile_w(0, s);
#endif

    title_init(&a->title);
    usize mrem = marena_rem(&a->ma);
    pltf_log("\nAPP MEM remaining: %i%% (%i kB)\n\n",
             (i32)((100 * mrem) / a->ma.bufsize),
             (i32)(mrem / 1024));
    aud_set_stereo(1);
    aud_set_vol(256, 256);
    aud_cmd_queue_commit();
    pltf_sync_timestep();
}

i32 app_init()
{
    // setup critical things for asset loading etc
    app_s *a = &APP;
    marena_init(&a->ma, a->mem_app, sizeof(a->mem_app));
    spm_init(a->mem_spm, sizeof(a->mem_spm));
    err32 err_wad_core = wad_init_file("oe.wad");
    if (err_wad_core != 0) {
        return (err_wad_core | ASSETS_ERR_WAD_INIT);
    }
    app_load_init(&a->load);
#if APP_LOAD_STATIC_RES_AT_ONCE
    app_load_tasks_timed(&a->load, 0);
    app_on_finish_startup();
#endif
    return 0;
}

static void app_tick_step();

void app_tick()
{
    app_s *a = &APP;
    switch (a->state) {
    case APP_ST_LOAD: {
        a->timer++;
        if (app_load_tasks_timed(&a->load, 18) && 50 <= a->timer) {
            a->timer = 0;
            app_on_finish_startup();
        }
        break;
    }
    case APP_ST_LOAD_ERR: {
        break;
    }
    default: {
        app_tick_step();
        aud_cmd_queue_commit();
        break;
    }
    }
}

static void app_tick_step()
{
    app_s *a = &APP;

#if PLTF_DEV_ENV
    // slowmotion during dev
    static u32 skiptick;
    if (pltf_sdl_key(SDL_SCANCODE_LSHIFT)) {
        skiptick++;
        if (skiptick & 7) return;
    }

#if 0
    if (pltf_sdl_jkey(SDL_GetScancodeFromKey(SDLK_PLUS))) {
        APP.aud.v_sfx_q8 = min_i32(APP.aud.v_sfx_q8 + 32, 256);
    }
    if (pltf_sdl_jkey(SDL_GetScancodeFromKey(SDLK_MINUS))) {
        APP.aud.v_sfx_q8 = max_i32(APP.aud.v_sfx_q8 - 32, 0);
    }
    if (pltf_sdl_jkey(SDL_SCANCODE_2)) {
        APP.aud.v_sfx_q8 = min_i32(APP.aud.v_sfx_q8 + 32, 256);
    }
    if (pltf_sdl_jkey(SDL_SCANCODE_1)) {
        APP.aud.v_sfx_q8 = max_i32(APP.aud.v_sfx_q8 - 32, 0);
    }
#endif
#endif
#if TIMING_ENABLED
    timing_beg(TIMING_ID_TICK);
#endif
    inp_update();
#if PLTF_PD
    if (a->crank_requested) {
        if (pltf_pd_crank_docked()) {
            a->crank_ui_tick++;
        } else {
            a->crank_ui_tick = 0;
        }
    }
#endif

    g_s *g = &a->game;

    if (a->sm.active) {
        settings_update(&a->sm);
    } else {
        switch (a->state) {
        case APP_ST_TITLE: {
            title_update(a, &a->title);
            break;
        }
        case APP_ST_GAME: {
            inp_state_s istate = inp_cur().c;
            game_tick(g, istate);
            break;
        }
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

    app_s    *a   = &APP;
    g_s      *g   = &a->game;
    gfx_ctx_s ctx = gfx_ctx_display();

    switch (a->state) {
    case APP_ST_LOAD: {
        texrec_s tr = asset_texrec(TEXID_COVER, 0, 0, 400, 240);
        gfx_spr(ctx, tr, (v2_i32){0}, 0, 0);
        rec_i32   rfill   = {0, 0, 400, 240};
        gfx_ctx_s ctxfill = ctx;
        ctxfill.pat       = gfx_pattern_interpolate(min_i32(40, 40 - a->timer), 40);
        gfx_rec_fill(ctxfill, rfill, PRIM_MODE_BLACK);

        // tiny loading bar
        rec_i32 rbar = {0, 240 - 1, app_load_progress_mul(&a->load, 400), 2};
        gfx_rec_fill(ctx, rbar, PRIM_MODE_WHITE);
        break;
    }
    case APP_ST_LOAD_ERR: {
        tex_clr(ctx.dst, GFX_COL_BLACK);
        break;
    }
    case APP_ST_TITLE: {
        title_render(&a->title);
        break;
    }

    case APP_ST_GAME: {
        game_draw(g);
        break;
    }
    }

    if (a->sm.active) {
        settings_draw(&a->sm);
    }
#if TIMING_ENABLED
    timing_end(TIMING_ID_DRAW);
    timing_end_frame();
#endif
#if PLTF_DEV_ENV && 0
    // test out dither alignment
    if (pltf_sdl_key(SDL_SCANCODE_SPACE)) {
        gfx_ctx_s cc = ctx;
        gfx_rec_fill(cc, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK_WHITE);
    }
#endif

#if PLTF_PD
    // "USE THE CRANK!"
    if (a->crank_requested && a->crank_ui_tick) {
        // 40 = duration of "use crank!"
        // 90 = 3 turns x 2.5 ticks per frame x 12 frames
        i32 t  = a->crank_ui_tick % (40 + 90);
        i32 fx = 0;
        i32 fy = 0;

        if (40 <= t) { // 10/25 = 2.5 = ticks per frame
            fy = 1 + (((t - 40) * 10) / 25) % 12;
        }

        v2_i32 p = {0};
        if (pltf_pd_flip_y()) {
            fx  = 1;
            p.y = 4;
        } else {
            p.x = 400 - 128;
            p.y = 240 - 56;
        }
        texrec_s tr = asset_texrec(TEXID_USECRANK, fx * 128, fy * 52, 128, 52);
        gfx_spr(ctx, tr, p, 0, 0);
    }
#endif
}

void app_close()
{
    // aud_destroy();
    if (APP.state == APP_ST_GAME) {
        g_s *g = &APP.game;
        game_update_savefile(g);
        savefile_w(g->save_slot, g->savefile);
    }
}

void app_resume()
{
    inp_on_resume();
    game_resume(&APP.game);
    APP.crank_ui_tick = 0;
}

void app_pause()
{
#if PLTF_PD
    if (APP.state == APP_ST_TITLE) {
        title_paused(&APP.title);
    } else {
        game_paused(&APP.game);
    }
#endif
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    aud_audio(lbuf, rbuf, len);
}

void app_mirror(b32 enable)
{
    if (enable) {
        APP.flags |= APP_FLAG_PD_MIRROR;
    } else {
        APP.flags &= ~APP_FLAG_PD_MIRROR;
    }
}

void *app_alloc(usize s)
{
    void *mem = marena_alloc(&APP.ma, s);
    assert(mem);
    mclr(mem, s);
    return mem;
}

void *app_alloc_aligned(usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&APP.ma, s, alignment);
    assert(mem);
    mclr(mem, s);
    return mem;
}

void *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment)
{
    return app_alloc_aligned(s, alignment);
}

void *app_alloc_aligned_rem(usize alignment, usize *rem)
{
    marena_align(&APP.ma, alignment);
    return marena_alloc_rem(&APP.ma, rem);
}

allocator_s app_allocator()
{
    allocator_s a = {app_alloc_aligned_ctx, &APP.ma};
    return a;
}

void app_set_mode(i32 mode)
{
    switch (mode) {
    case SETTINGS_MODE_NORMAL:
        pltf_set_fps_mode(PLTF_FPS_MODE_UNCAPPED);
        break;
    case SETTINGS_MODE_STREAMING:
        pltf_set_fps_mode(PLTF_FPS_MODE_UNCAPPED);
        break;
    case SETTINGS_MODE_POWER_SAVING:
        pltf_set_fps_mode(PLTF_FPS_MODE_40);
        break;
    }
}

void app_crank_requested(b32 enable)
{
    if (enable) {
        if (!APP.crank_requested) {
            APP.crank_ui_tick = 0;
        }
        APP.crank_requested = 1;
    } else {
        APP.crank_requested = 0;
        APP.crank_ui_tick   = 0;
    }
}

void app_menu_callback_timing(void *ctx, i32 opt)
{
#if TIMING_ENABLED
    TIMING.show = opt;
#endif
}

void app_menu_callback_resetsave(void *ctx, i32 opt)
{
    cs_resetsave_enter(&APP.game);
}

void app_menu_callback_map(void *ctx, i32 opt)
{
    if (!APP.game.minimap.state) {
        minimap_open_via_menu(&APP.game);
    }
}