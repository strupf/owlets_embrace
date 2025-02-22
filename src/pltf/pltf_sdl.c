// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf_sdl.h"
#include "pltf.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
#define PLTF_SDL_WEB 1
#else
#define PLTF_SDL_WEB 0
#endif
#define PLTF_SDL_SCALE_LAUNCH   1
#define PLTF_SDL_RECORD_1080P   (0 && !PLTF_SDL_WEB)
#define PLTF_SDL_SW_RENDERER    0 || PLTF_SDL_RECORD_1080P
#define PLTF_SDL_USE_DEBUG_RECS 0 && !PLTF_SDL_RECORD_1080P
#define PLTF_SDL_NUM_DEBUG_RECS 256
#define PLTF_SDL_WINDOW_TITLE   "Owlet's Embrace"

typedef struct {
    u32 col;
    u32 ticks;
    i16 x;
    i16 y;
    i16 w;
    i16 h;
} pltf_debug_rec_s;

typedef_struct (pltf_sdl_s) {
    u64               timeorigin;
    u64               time_prev;
    u64               fps_timer;
    i32               fps_tick;
    i32               fps_tick_prev;
    b32               running;
    u32               fps_cap;
    f32               fps_cap_dt;
    f32               delay_timer;
    f32               delay_timer_k;
    //
    u32               pal[2];
    u8                framebuffer[PLTF_DISPLAY_WBYTES * PLTF_DISPLAY_H];
    SDL_Window       *window;
    SDL_Renderer     *renderer;
    SDL_Texture      *tex;
    SDL_Rect          r_src;
    SDL_Rect          r_dst;
    SDL_AudioDeviceID audiodevID;
    SDL_AudioSpec     audiospec;
    SDL_PixelFormat  *pformat;
    b32               is_mono;
    b32               inv;
    f32               vol;
    void (*char_add)(char c, void *ctx);
    void (*char_del)(void *ctx);
    void (*close_inp)(void *ctx);
    void            *ctx;
    u8               keyp[SDL_NUM_SCANCODES];
    u8               keyc[SDL_NUM_SCANCODES];
    i32              n_debug_recs;
    pltf_debug_rec_s debug_recs[PLTF_SDL_NUM_DEBUG_RECS];
};

pltf_sdl_s g_SDL;

void pltf_sdl_input_flush();
void pltf_sdl_step();
void pltf_sdl_resize();
void pltf_sdl_set_FPS_cap(i32 fps);
void pltf_sdl_audio(void *u, u8 *stream, int len);

int main(int argc, char **argv)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    SDL_Init(SDL_INIT_EVENTS |
             SDL_INIT_AUDIO |
             SDL_INIT_GAMECONTROLLER);

    g_SDL.window = SDL_CreateWindow(PLTF_SDL_WINDOW_TITLE,
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
#if PLTF_SDL_RECORD_1080P
                                    1920,
                                    1080,
#elif defined(__EMSCRIPTEN__)
                                    PLTF_DISPLAY_W,
                                    PLTF_DISPLAY_H,
#else
                                    PLTF_DISPLAY_W * PLTF_SDL_SCALE_LAUNCH,
                                    PLTF_DISPLAY_H * PLTF_SDL_SCALE_LAUNCH,
#endif
                                    SDL_WINDOW_ALLOW_HIGHDPI |
                                        SDL_WINDOW_RESIZABLE |
                                        SDL_WINDOW_INPUT_FOCUS);

    SDL_SetWindowMinimumSize(g_SDL.window, PLTF_DISPLAY_W, PLTF_DISPLAY_H);
    g_SDL.renderer = SDL_CreateRenderer(g_SDL.window, -1,
#if PLTF_SDL_SW_RENDERER // software renderer produces regular scaling
                                        SDL_RENDERER_SOFTWARE
#else
                                        0
#endif
    );
    SDL_SetRenderDrawColor(g_SDL.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(g_SDL.renderer, &info);
    Uint32 pformat = info.texture_formats[0];
    g_SDL.tex      = SDL_CreateTexture(g_SDL.renderer,
                                       pformat,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       PLTF_DISPLAY_W,
                                       PLTF_DISPLAY_H);
    SDL_SetTextureScaleMode(g_SDL.tex, SDL_ScaleModeNearest);

    g_SDL.pformat = SDL_AllocFormat(pformat);
    g_SDL.pal[0]  = SDL_MapRGB((const SDL_PixelFormat *)g_SDL.pformat,
                               0x00, 0x00, 0x00); // black
    g_SDL.pal[1]  = SDL_MapRGB((const SDL_PixelFormat *)g_SDL.pformat,
                               0xFF, 0xFF, 0xFF); // white

    // -------------------------------------------------------------------------
    // RUN

    g_SDL.timeorigin = SDL_GetPerformanceCounter();
    i32 res          = pltf_internal_init();
    if (res == 0) {
        SDL_AudioSpec as = {0};
        as.channels      = 2;
        as.freq          = 44100;
        as.format        = AUDIO_S16;
        as.samples       = 256;
        as.callback      = pltf_sdl_audio;

        g_SDL.audiodevID = SDL_OpenAudioDevice(0, 0, &as, &g_SDL.audiospec, 0);
        pltf_log("+++ Audio Device ID: %i\n\n", g_SDL.audiodevID);
        if (g_SDL.audiodevID) {
            SDL_PauseAudioDevice(g_SDL.audiodevID, 0);
        } else {
            pltf_log("+++ SDL: Can't create audio device!\n");
        }

        g_SDL.running   = 1;
        g_SDL.r_src.w   = PLTF_DISPLAY_W;
        g_SDL.r_src.h   = PLTF_DISPLAY_H;
        g_SDL.r_dst.w   = PLTF_DISPLAY_W;
        g_SDL.r_dst.h   = PLTF_DISPLAY_H;
        g_SDL.is_mono   = 1;
        g_SDL.vol       = 0.5f;
        g_SDL.time_prev = (u64)SDL_GetPerformanceCounter();
        pltf_sdl_resize();

#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(pltf_sdl_step, 0, 1);
#else
        pltf_sdl_set_FPS_cap(120);

        while (g_SDL.running) {
            pltf_sdl_step();
        }
#endif
    } else {
        pltf_log("ERROR INIT: %i\n", res);
    }

    // -------------------------------------------------------------------------
    // CLOSE
    if (g_SDL.audiodevID) {
        SDL_CloseAudioDevice(g_SDL.audiodevID);
    }
    pltf_internal_close();
    SDL_FreeFormat(g_SDL.pformat);
    SDL_DestroyTexture(g_SDL.tex);
    SDL_DestroyRenderer(g_SDL.renderer);
    SDL_DestroyWindow(g_SDL.window);
    SDL_Quit();
    return 0;
}

void pltf_sdl_step()
{
    u64 time        = (u64)SDL_GetPerformanceCounter();
    u64 timedt      = time - g_SDL.time_prev;
    g_SDL.time_prev = time;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            g_SDL.running = 0;
            return;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_BACKSPACE && g_SDL.char_del) {
                g_SDL.char_del(g_SDL.ctx);
                break;
            }
            if ((e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_RETURN) && g_SDL.close_inp) {
                g_SDL.close_inp(g_SDL.ctx);
                break;
            }
            break;
        case SDL_WINDOWEVENT:
            switch (e.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_MAXIMIZED: {
                pltf_sdl_resize();
            } break;
            }
            break;
        case SDL_TEXTINPUT: {
            if (!g_SDL.char_add) break;
            for (char *c = &e.text.text[0]; *c != '\0'; c++) {
                if ((int)*c & 0x80) continue; // non ascii
                g_SDL.char_add(*c, g_SDL.ctx);
            }
            break;
        }
        }
    }

    const Uint8 *ks = SDL_GetKeyboardState(0);
    for (i32 n = 0; n < SDL_NUM_SCANCODES; n++) {
        g_SDL.keyc[n] |= ks[n];
    }

    if (pltf_internal_update()) {
        pltf_sdl_input_flush();
        int   pitch;
        void *pixelsptr;
        SDL_LockTexture(g_SDL.tex, NULL, &pixelsptr, &pitch);
        u32 *pixels = (u32 *)pixelsptr;

        for (i32 y = 0; y < PLTF_DISPLAY_H; y++) {
            for (i32 x = 0; x < PLTF_DISPLAY_W; x++) {
                i32 i     = (x >> 3) + y * PLTF_DISPLAY_WBYTES;
                i32 k     = x + y * PLTF_DISPLAY_W;
                i32 bit   = !!(g_SDL.framebuffer[i] & (0x80 >> (x & 7)));
                i32 col   = g_SDL.pal[g_SDL.inv ? !bit : bit];
                pixels[k] = col;
            }
        }

        for (i32 n = g_SDL.n_debug_recs - 1; 0 <= n; n--) {
            pltf_debug_rec_s *dbr = &g_SDL.debug_recs[n];
            if (dbr->ticks == 0) {
                *dbr = g_SDL.debug_recs[--g_SDL.n_debug_recs];
                continue;
            }
            dbr->ticks--;

            i32 x1 = dbr->x;
            i32 y1 = dbr->y;
            i32 x2 = dbr->x + dbr->w - 1;
            i32 y2 = dbr->y + dbr->h - 1;
            i32 u1 = 0 <= x1 ? x1 : 0;
            i32 v1 = 0 <= y1 ? y1 : 0;
            i32 u2 = x2 < PLTF_DISPLAY_W ? x2 : PLTF_DISPLAY_W - 1;
            i32 v2 = y2 < PLTF_DISPLAY_H ? y2 : PLTF_DISPLAY_H - 1;

            if (0 <= y1 && y1 < PLTF_DISPLAY_H)
                for (i32 x = u1; x <= u2; x++)
                    pixels[x + y1 * PLTF_DISPLAY_W] = dbr->col;
            if (0 <= y2 && y2 < PLTF_DISPLAY_H)
                for (i32 x = u1; x <= u2; x++)
                    pixels[x + y2 * PLTF_DISPLAY_W] = dbr->col;
            if (0 <= x1 && x1 < PLTF_DISPLAY_W)
                for (i32 y = v1; y <= v2; y++)
                    pixels[x1 + y * PLTF_DISPLAY_W] = dbr->col;
            if (0 <= x2 && x2 < PLTF_DISPLAY_W)
                for (i32 y = v1; y <= v2; y++)
                    pixels[x2 + y * PLTF_DISPLAY_W] = dbr->col;
        }
        SDL_UnlockTexture(g_SDL.tex);
    }

    SDL_SetRenderDrawColor(g_SDL.renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(g_SDL.renderer);
    SDL_RenderCopy(g_SDL.renderer, g_SDL.tex, &g_SDL.r_src, &g_SDL.r_dst);
    SDL_RenderPresent(g_SDL.renderer);

#ifndef __EMSCRIPTEN__
    if (!g_SDL.fps_cap) return;

    // custom FPS cap solution in SDL
    // try to sleep for the desired amount of time
    // use a multiplier delay_timer_k to fine tune the sleep over time
    // so we actually hit e.g. 120 FPS instead of 114 FPS
    g_SDL.fps_tick++;
    g_SDL.fps_timer += timedt;
    u64 one_s = (u64)SDL_GetPerformanceFrequency();

    if (one_s <= g_SDL.fps_timer) {
        g_SDL.fps_timer -= one_s;
        f32 avg_fps  = (f32)(g_SDL.fps_tick + g_SDL.fps_tick_prev) * 0.5f;
        f32 diff     = avg_fps - (f32)g_SDL.fps_cap;
        f32 diff_abs = 0.f <= diff ? diff : -diff;
        f32 k_change = 0.f;

        // greater change to multiplier if actual FPS is far off
        if (2.0f <= diff_abs) k_change = .025f;
        else if (1.00f <= diff_abs) k_change = .010f;
        else if (0.25f <= diff_abs) k_change = .001f;
        if (0.f < diff) g_SDL.delay_timer_k += k_change;
        if (0.f > diff) g_SDL.delay_timer_k -= k_change;
        g_SDL.fps_tick_prev = g_SDL.fps_tick;
        g_SDL.fps_tick      = 0;
    }

    f32 tdt = (f32)((u64)SDL_GetPerformanceCounter() - time) / (f32)one_s;
    g_SDL.delay_timer += (g_SDL.fps_cap_dt - tdt) * g_SDL.delay_timer_k;
    g_SDL.delay_timer = 0.f <= g_SDL.delay_timer ? g_SDL.delay_timer : 0.f;
    if (1.f <= g_SDL.delay_timer) {
        u32 sleepfor = (u32)g_SDL.delay_timer;
        g_SDL.delay_timer -= (f32)sleepfor;
        SDL_Delay((Uint32)sleepfor);
    }
#endif
}

void pltf_sdl_resize()
{
    int w, h;
    SDL_GetWindowSizeInPixels(g_SDL.window, &w, &h);
    f32 sx = (f32)w / (f32)g_SDL.r_src.w;
    f32 sy = (f32)h / (f32)g_SDL.r_src.h;
    // integer scaling - patterns look terrible stretched
#if PLTF_SDL_RECORD_1080P
    g_SDL.r_dst.w = 1800;
    g_SDL.r_dst.h = 1080;
#elif PLTF_SDL_RECORD_480P
    g_SDL.r_dst.w = 800;
    g_SDL.r_dst.h = 480;
#else
    i32 si        = (i32)(sx <= sy ? sx : sy);
    g_SDL.r_dst.w = g_SDL.r_src.w * si;
    g_SDL.r_dst.h = g_SDL.r_src.h * si;
#endif
    g_SDL.r_dst.x = (w - g_SDL.r_dst.w) / 2;
    g_SDL.r_dst.y = (h - g_SDL.r_dst.h) / 2;
    pltf_sync_timestep();
}

void pltf_sdl_set_FPS_cap(i32 fps)
{
    if (fps <= 0) {
        g_SDL.fps_cap     = 0;
        g_SDL.fps_cap_dt  = 0.f;
        g_SDL.delay_timer = 0.f;
        return;
    }
    g_SDL.delay_timer_k = 1000.f; // 1000 milliseconds starting value
    g_SDL.fps_cap       = fps;
    g_SDL.fps_cap_dt    = 1.f / (f32)fps;
}

// stream is an interlaced byte buffer: LLRRLLRRLL...
// len is buffer length in bytes (datatype size * channels * length)
void pltf_sdl_audio(void *u, u8 *stream, int len)
{
    static i16 lbuf[0x1000];
    static i16 rbuf[0x1000];

    mclr(lbuf, sizeof(lbuf));
    mclr(rbuf, sizeof(rbuf));
    i32 samples = len / (2 * sizeof(i16));
    pltf_internal_audio(lbuf, rbuf, samples);

    i16 *s = (i16 *)stream;
    i16 *l = lbuf;
    i16 *r = rbuf;

    g_SDL.is_mono = 0;
    for (i32 n = 0; n < samples; n++) {
        i32 vl = (i32)((f32)*l++ * g_SDL.vol);
        i32 vr = g_SDL.is_mono ? vl : (i32)((f32)*r++ * g_SDL.vol);
        *s++   = vl;
        *s++   = vr;
    }
}

void pltf_sdl_txt_inp_set_cb(void (*char_add)(char c, void *ctx), void (*char_del)(void *ctx), void (*close_inp)(void *ctx), void *ctx)
{
    g_SDL.char_add  = char_add;
    g_SDL.char_del  = char_del;
    g_SDL.close_inp = close_inp;
    g_SDL.ctx       = ctx;
}

void pltf_sdl_txt_inp_clr_cb()
{
    g_SDL.char_add  = NULL;
    g_SDL.char_del  = NULL;
    g_SDL.close_inp = NULL;
    g_SDL.ctx       = NULL;
}

// BACKEND =====================================================================

void pltf_audio_set_volume(f32 vol)
{
    pltf_audio_lock();
    g_SDL.vol = vol;
    pltf_audio_unlock();
}

f32 pltf_audio_get_volume()
{
    return g_SDL.vol;
}

f32 pltf_seconds()
{
    u64 d = (u64)SDL_GetPerformanceCounter() - g_SDL.timeorigin;
    return (f32)d / (f32)SDL_GetPerformanceFrequency();
}

void pltf_1bit_invert(bool32 i)
{
    g_SDL.inv = i;
}

void *pltf_1bit_buffer()
{
    return g_SDL.framebuffer;
}

void pltf_debugr(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32 t)
{
#if PLTF_SDL_USE_DEBUG_RECS
    if (PLTF_SDL_NUM_DEBUG_RECS <= g_SDL.n_debug_recs) return;

    pltf_debug_rec_s rec = {0};
    rec.col              = SDL_MapRGB((const SDL_PixelFormat *)g_SDL.pformat,
                                      r, g, b);
    rec.ticks            = 0 < t ? t : 1;
    rec.x                = ssat(x, 16);
    rec.y                = ssat(y, 16);
    rec.w                = ssat(w, 16);
    rec.h                = ssat(h, 16);

    g_SDL.debug_recs[g_SDL.n_debug_recs++] = rec;
#endif
}

bool32 pltf_accelerometer_enabled()
{
    return 0;
}

void pltf_accelerometer_set(bool32 enabled)
{
}

void pltf_accelerometer(f32 *x, f32 *y, f32 *z)
{
    *x = 0.f, *y = 0.f, *z = 0.f;
}

void *pltf_file_open_r(const char *path)
{
    return fopen(path, "rb");
}

void *pltf_file_open_w(const char *path)
{
    return fopen(path, "wb");
}

void *pltf_file_open_a(const char *path)
{
    return fopen(path, "ab");
}

bool32 pltf_file_close(void *f)
{
    return (fclose((FILE *)f) == 0);
}

bool32 pltf_file_del(const char *path)
{
    return (remove(path) == 0);
}

i32 pltf_file_tell(void *f)
{
    return ftell((FILE *)f);
}

i32 pltf_file_seek_set(void *f, i32 pos)
{
    return (i32)fseek((FILE *)f, pos, SEEK_SET);
}

i32 pltf_file_seek_cur(void *f, i32 pos)
{
    return (i32)fseek((FILE *)f, pos, SEEK_CUR);
}

i32 pltf_file_seek_end(void *f, i32 pos)
{
    return (i32)fseek((FILE *)f, pos, SEEK_END);
}

i32 pltf_file_w(void *f, const void *buf, usize bsize)
{
    return (i32)fwrite(buf, 1, bsize, (FILE *)f);
}

i32 pltf_file_r(void *f, void *buf, usize bsize)
{
    return (i32)fread(buf, 1, bsize, (FILE *)f);
}

void pltf_audio_lock()
{
    SDL_LockAudioDevice(g_SDL.audiodevID);
}

void pltf_audio_unlock()
{
    SDL_UnlockAudioDevice(g_SDL.audiodevID);
}

bool32 pltf_sdl_key(i32 k)
{
    return g_SDL.keyc[k];
}

bool32 pltf_sdl_jkey(i32 k)
{
    return (g_SDL.keyc[k] && !g_SDL.keyp[k]);
}

void pltf_sdl_input_flush()
{
    mcpy(g_SDL.keyp, g_SDL.keyc, sizeof(g_SDL.keyc));
    mclr(g_SDL.keyc, sizeof(g_SDL.keyc));
}