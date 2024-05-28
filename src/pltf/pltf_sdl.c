// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf.h"

#define PLTF_SDL_WINDOW_TITLE "Owlet's Embrace"

typedef struct pltf_sdl_s {
    bool32            running;
    u64               timeorigin;
    u32               fps_cap;
    f64               fps_cap_dt;
    f64               delay_timer;
    f64               delay_timer_k;
    //
    u32               pal[2];
    u8                framebuffer[52 * 240];
    SDL_Window       *window;
    SDL_Renderer     *renderer;
    SDL_Texture      *texture;
    SDL_Rect          r_src;
    SDL_Rect          r_dst;
    SDL_AudioDeviceID audiodevID;
    SDL_AudioSpec     audiospec;
    bool32            is_mono;
    bool32            inv;
    f32               vol;

} pltf_sdl_s;

pltf_sdl_s g_SDL;

void pltf_sdl_init();
void pltf_sdl_resize();
void pltf_sdl_set_FPS_cap(i32 fps);
void pltf_sdl_audio(void *u, u8 *stream, int len);

int main(int argc, char **argv)
{
    pltf_sdl_init();
    pltf_internal_init();

    u64 time_prev     = SDL_GetPerformanceCounter();
    u64 fps_timer     = 0;
    i32 fps_tick      = 0;
    i32 fps_tick_prev = 0;

    while (g_SDL.running) {
        u64 time   = SDL_GetPerformanceCounter();
        u64 timedt = time - time_prev;
        time_prev  = time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym != SDLK_ESCAPE) break;
            case SDL_QUIT: g_SDL.running = 0; break;
            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED: {
                    pltf_sdl_resize();
                } break;
                }
                break;
            }
        }

        if (pltf_internal_update()) {
            int   pitch;
            void *pixelsptr;
            SDL_LockTexture(g_SDL.texture, NULL, &pixelsptr, &pitch);
            u32 *pixels = (u32 *)pixelsptr;

            for (i32 y = 0; y < 240; y++) {
                for (i32 x = 0; x < 400; x++) {
                    i32 i     = (x >> 3) + y * 52;
                    i32 k     = x + y * 400;
                    i32 byt   = g_SDL.framebuffer[i];
                    i32 bit   = !!(byt & (0x80 >> (x & 7)));
                    pixels[k] = g_SDL.pal[g_SDL.inv ? !bit : bit];
                }
            }
            SDL_UnlockTexture(g_SDL.texture);
        }

        SDL_SetRenderDrawColor(g_SDL.renderer, 0x31, 0x2F, 0x28, 0xFF);
        SDL_RenderClear(g_SDL.renderer);
        SDL_RenderCopy(g_SDL.renderer, g_SDL.texture, &g_SDL.r_src, &g_SDL.r_dst);
        SDL_RenderPresent(g_SDL.renderer);

        // FPS cap in SDL
        if (g_SDL.fps_cap) {
            fps_tick++;
            fps_timer += timedt;
            Uint64 one_second = SDL_GetPerformanceFrequency();
            if (one_second <= fps_timer) {
                fps_timer -= one_second;
                f64 avg_fps  = (f64)(fps_tick + fps_tick_prev) * 0.5;
                f64 diff     = avg_fps - (f64)g_SDL.fps_cap;
                f64 k_change = 0.0;
                f64 diff_abs = 0.0 <= diff ? diff : -diff;
                if (2.0 <= diff_abs) k_change = 0.025;
                else if (1.00 <= diff_abs) k_change = 0.010;
                else if (0.25 <= diff_abs) k_change = 0.001;
                if (0.0 < diff) g_SDL.delay_timer_k += k_change;
                if (0.0 > diff) g_SDL.delay_timer_k -= k_change;
                fps_tick_prev = fps_tick;
                fps_tick      = 0;
            }

            f64 tdt = (f64)(SDL_GetPerformanceCounter() - time) / (f64)one_second;
            g_SDL.delay_timer += (g_SDL.fps_cap_dt - tdt) * 1000. * g_SDL.delay_timer_k;
            g_SDL.delay_timer = 0. <= g_SDL.delay_timer ? g_SDL.delay_timer : 0.;
            if (1. <= g_SDL.delay_timer) {
                Uint32 sleepfor = (Uint32)g_SDL.delay_timer;
                g_SDL.delay_timer -= (f64)sleepfor;
                SDL_Delay(sleepfor);
            }
        }
    }
    pltf_internal_close();

    SDL_CloseAudioDevice(g_SDL.audiodevID);
    SDL_DestroyTexture(g_SDL.texture);
    SDL_DestroyRenderer(g_SDL.renderer);
    SDL_DestroyWindow(g_SDL.window);
    SDL_Quit();
    return 0;
}

void pltf_sdl_init()
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    SDL_Init(SDL_INIT_EVENTS |
             SDL_INIT_AUDIO |
             SDL_INIT_GAMECONTROLLER);

    g_SDL.window = SDL_CreateWindow(PLTF_SDL_WINDOW_TITLE,
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    400 * 2,
                                    240 * 2,
                                    SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

    SDL_SetWindowMinimumSize(g_SDL.window, 400, 240);
    g_SDL.renderer = SDL_CreateRenderer(g_SDL.window, -1, 0);

    SDL_SetRenderDrawColor(g_SDL.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(g_SDL.renderer, &info);
    Uint32 pformat = info.texture_formats[0];
    g_SDL.texture  = SDL_CreateTexture(g_SDL.renderer,
                                       pformat,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       400,
                                       240);

    SDL_PixelFormat *f = SDL_AllocFormat(pformat);
    g_SDL.pal[0]       = SDL_MapRGB((const SDL_PixelFormat *)f, 0x31, 0x2F, 0x28); // black
    g_SDL.pal[1]       = SDL_MapRGB((const SDL_PixelFormat *)f, 0xB1, 0xAF, 0xA8); // white
    SDL_FreeFormat(f);

    SDL_AudioSpec frmt = {0};
    frmt.channels      = 2;
    frmt.freq          = 44100;
    frmt.format        = AUDIO_S16;
    frmt.samples       = 256;
    frmt.callback      = pltf_sdl_audio;

    g_SDL.audiodevID = SDL_OpenAudioDevice(NULL, 0, &frmt, &g_SDL.audiospec, 0);
    if (g_SDL.audiodevID) {
        SDL_PauseAudioDevice(g_SDL.audiodevID, 0);
    }

    g_SDL.running    = 1;
    g_SDL.r_src.w    = 400;
    g_SDL.r_src.h    = 240;
    g_SDL.r_dst.w    = 400 * 2;
    g_SDL.r_dst.h    = 240 * 2;
    g_SDL.timeorigin = SDL_GetPerformanceCounter();
    g_SDL.is_mono    = 1;
    g_SDL.vol        = 0.5f;
    pltf_sdl_resize();
    pltf_sdl_set_FPS_cap(120);
}

void pltf_sdl_resize()
{
    int w, h;
    SDL_GetWindowSize(g_SDL.window, &w, &h);
    f32 sx = (f32)w / (f32)g_SDL.r_src.w;
    f32 sy = (f32)h / (f32)g_SDL.r_src.h;
#if 1 // integer scaling - patterns look terrible stretched
    i32 si        = (i32)(sx <= sy ? sx : sy);
    g_SDL.r_dst.w = g_SDL.r_src.w * si;
    g_SDL.r_dst.h = g_SDL.r_src.h * si;
#else
    if (sx < sy) {
        OS_SDL.r_dst.w = w;
        OS_SDL.r_dst.h = (i32)(sx * (f32)OS_SDL.r_src.h + .5f);
    } else {
        OS_SDL.r_dst.w = (i32)(sy * (f32)OS_SDL.r_src.w + .5f);
        OS_SDL.r_dst.h = h;
    }
#endif
    g_SDL.r_dst.x = (w - g_SDL.r_dst.w) / 2;
    g_SDL.r_dst.y = (h - g_SDL.r_dst.h) / 2;
}

void pltf_sdl_set_FPS_cap(i32 fps)
{
    if (fps <= 0) {
        g_SDL.fps_cap     = 0;
        g_SDL.fps_cap_dt  = 0.;
        g_SDL.delay_timer = 0.;
        return;
    }
    g_SDL.delay_timer_k = 1.;
    g_SDL.fps_cap       = fps;
    g_SDL.fps_cap_dt    = 1. / (f64)fps;
}

bool32 pltf_sdl_key(i32 k)
{
    const u8 *keys = SDL_GetKeyboardState(NULL);
    return keys[k];
}

// stream is an interlaced byte buffer: LLRRLLRRLL...
// len is buffer length in bytes (datatype size * channels * length)
void pltf_sdl_audio(void *u, u8 *stream, int len)
{
    static i16 lbuf[0x1000];
    static i16 rbuf[0x1000];

    memset(lbuf, 0, sizeof(lbuf));
    memset(rbuf, 0, sizeof(rbuf));
    i32 samples = len / (2 * sizeof(i16));
    pltf_internal_audio(lbuf, rbuf, samples);

    i16 *s = (i16 *)stream;
    i16 *l = lbuf;
    i16 *r = (i16 *)rbuf;

    if (g_SDL.is_mono) {
        for (i32 n = 0; n < samples; n++) {
            i32 v = i16_sat((i32)((f32)*l++ * g_SDL.vol));
            *s++  = v;
            *s++  = v;
        }
    } else {
        for (i32 n = 0; n < samples; n++) {
            *s++ = (i16)((f32)*l++ * g_SDL.vol);
            *s++ = (i16)((f32)*r++ * g_SDL.vol);
        }
    }
}

void pltf_sdl_set_vol(f32 vol)
{
#ifdef PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    g_SDL.vol = vol;
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

f32 pltf_sdl_vol()
{
    return g_SDL.vol;
}

void pltf_sdl_audio_lock()
{
    SDL_LockAudioDevice(g_SDL.audiodevID);
}

void pltf_sdl_audio_unlock()
{
    SDL_UnlockAudioDevice(g_SDL.audiodevID);
}

// BACKEND =====================================================================

f32 pltf_seconds()
{
    u64 d = SDL_GetPerformanceCounter() - g_SDL.timeorigin;
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

void *pltf_file_open(const char *path, i32 mode)
{
    switch (mode) {
    case PLTF_FILE_R: return fopen(path, "rb");
    case PLTF_FILE_W: return fopen(path, "wb");
    case PLTF_FILE_A: return fopen(path, "ab");
    }
    return NULL;
}

bool32 pltf_file_close(void *f)
{
    return (fclose(f) == 0);
}

bool32 pltf_file_del(const char *path)
{
    return (remove(path) == 0);
}

i32 pltf_file_tell(void *f)
{
    return ftell(f);
}

i32 pltf_file_seek(void *f, i32 pos, i32 origin)
{
    return fseek(f, pos, origin);
}

i32 pltf_file_w(void *f, const void *buf, usize bsize)
{
    usize w = fwrite(buf, 1, bsize, f);
    return (i32)w;
}

i32 pltf_file_r(void *f, void *buf, usize bsize)
{
    usize r = fread(buf, 1, bsize, f);
    return (i32)r;
}