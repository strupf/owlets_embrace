// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys.h"
#include "sys_backend.h"
//
#include "SDL2/SDL.h"
#include <stdio.h>

#define SYS_SDL_SCALE 1

static_assert(SYS_FILE_SEEK_SET == RW_SEEK_SET, "seek");
static_assert(SYS_FILE_SEEK_CUR == RW_SEEK_CUR, "seek");
static_assert(SYS_FILE_SEEK_END == RW_SEEK_END, "seek");

static struct {
    int               running;
    u64               timeorigin;
    u8                framebuffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
    u8                menuimg[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
    u8                update_row[SYS_DISPLAY_H];
    Uint32            color_pal[2];
    SDL_Window       *window;
    SDL_Renderer     *renderer;
    SDL_Texture      *texture;
    SDL_Rect          r_src;
    SDL_Rect          r_dst;
    SDL_AudioDeviceID audiodevID;
    SDL_AudioSpec     audiospec;
    int               is_mono;
    int               inv;
    f32               vol;
    f32               crank;
    int               pausebtn;
    int               pausebtnp;
    int               paused;
    int               spacepressed;
} OS_SDL;

static void                backend_SDL_audio(void *unused, u8 *stream, int len);
static SDL_GameController *backend_SDL_gamecontroller();
static bool32              backend_SDL_hit_pause();

int main(int argc, char **argv)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    SDL_Init(SDL_INIT_EVENTS |
             SDL_INIT_TIMER |
             SDL_INIT_VIDEO |
             SDL_INIT_AUDIO |
             SDL_INIT_GAMECONTROLLER);

    OS_SDL.window   = SDL_CreateWindow("SDL2 Window",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       SYS_DISPLAY_W * SYS_SDL_SCALE,
                                       SYS_DISPLAY_H * SYS_SDL_SCALE,
                                       SDL_WINDOW_RESIZABLE);
    OS_SDL.renderer = SDL_CreateRenderer(OS_SDL.window,
                                         -1,
                                         SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawColor(OS_SDL.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(OS_SDL.renderer, &info);
    Uint32 pformat = info.texture_formats[0];
    OS_SDL.texture = SDL_CreateTexture(OS_SDL.renderer,
                                       pformat,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SYS_DISPLAY_W,
                                       SYS_DISPLAY_H);

    SDL_PixelFormat       *format = SDL_AllocFormat(pformat);
    const SDL_PixelFormat *f      = (const SDL_PixelFormat *)format;
    OS_SDL.color_pal[0]           = SDL_MapRGBA(f, 0x31, 0x2F, 0x28, 0xFF);
    OS_SDL.color_pal[1]           = SDL_MapRGBA(f, 0xB1, 0xAF, 0xA8, 0xFF);
    SDL_FreeFormat(format);

    SDL_AudioSpec frmt = {0};
    frmt.channels      = 2;
    frmt.freq          = 44100;
    frmt.format        = AUDIO_S16;
    frmt.samples       = 256;
    frmt.callback      = backend_SDL_audio;

    OS_SDL.audiodevID = SDL_OpenAudioDevice(NULL, 0, &frmt, &OS_SDL.audiospec, 0);
    if (0 < OS_SDL.audiodevID) {
        SDL_PauseAudioDevice(OS_SDL.audiodevID, 0);
    }

    OS_SDL.running    = 1;
    OS_SDL.r_src.w    = SYS_DISPLAY_W;
    OS_SDL.r_src.h    = SYS_DISPLAY_H;
    OS_SDL.r_dst.w    = SYS_DISPLAY_W * SYS_SDL_SCALE;
    OS_SDL.r_dst.h    = SYS_DISPLAY_H * SYS_SDL_SCALE;
    OS_SDL.timeorigin = SDL_GetPerformanceCounter();
    OS_SDL.is_mono    = 1;
    OS_SDL.vol        = 1.f;
    backend_display_row_updated(0, SYS_DISPLAY_H);

    sys_init();

    while (OS_SDL.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: OS_SDL.running = 0; break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                    e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    e.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                    int w, h;
                    SDL_GetWindowSize(OS_SDL.window, &w, &h);
                    f32 sx = (f32)w / (f32)OS_SDL.r_src.w;
                    f32 sy = (f32)h / (f32)OS_SDL.r_src.h;
                    if (sx < sy) {
                        OS_SDL.r_dst.w = w;
                        OS_SDL.r_dst.h = (int)(sx * (f32)OS_SDL.r_src.h + .5f);
                    } else {
                        OS_SDL.r_dst.h = h;
                        OS_SDL.r_dst.w = (int)(sy * (f32)OS_SDL.r_src.w + .5f);
                    }
                    OS_SDL.r_dst.x = (w - OS_SDL.r_dst.w) / 2;
                    OS_SDL.r_dst.y = (h - OS_SDL.r_dst.h) / 2;
                }
                break;
            }
        }

        OS_SDL.pausebtnp = OS_SDL.pausebtn;
        OS_SDL.pausebtn  = backend_SDL_hit_pause();

        if (!OS_SDL.pausebtnp && OS_SDL.pausebtn) {
            if (OS_SDL.paused) {
                OS_SDL.paused = 0;
                sys_resume();
            } else {
                OS_SDL.paused = 1;
                sys_pause();
            }
        }

        int i = 1;
        if (!OS_SDL.paused) {
            i = sys_tick(NULL);
        }

        if (i != 0) {
            int   pitch;
            void *pixelsptr;
            SDL_LockTexture(OS_SDL.texture, NULL, &pixelsptr, &pitch);
            Uint32 *pixels = (Uint32 *)pixelsptr;

            for (int y = 0; y < SYS_DISPLAY_H; y++) {
                if (OS_SDL.update_row[y] == 0) continue;
                OS_SDL.update_row[y] = 0;
                for (int x = 0; x < SYS_DISPLAY_W; x++) {
                    int i     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
                    int k     = x + y * SYS_DISPLAY_W;
                    int byt   = OS_SDL.framebuffer[i];
                    int bit   = (byt & (0x80 >> (x & 7))) > 0;
                    pixels[k] = OS_SDL.color_pal[OS_SDL.inv ? !bit : bit];
                }

                if (OS_SDL.paused) {
                    for (int x = 200; x < SYS_DISPLAY_W; x++) {
                        pixels[x + y * SYS_DISPLAY_W] = OS_SDL.color_pal[0];
                    }
                }
            }
            SDL_UnlockTexture(OS_SDL.texture);
        }

        SDL_SetRenderDrawColor(OS_SDL.renderer, 0x31, 0x2F, 0x28, 255);
        SDL_RenderClear(OS_SDL.renderer);
        SDL_RenderCopy(OS_SDL.renderer, OS_SDL.texture, &OS_SDL.r_src, &OS_SDL.r_dst);
        SDL_RenderPresent(OS_SDL.renderer);
    }
    sys_close();

    SDL_CloseAudioDevice(OS_SDL.audiodevID);
    SDL_DestroyTexture(OS_SDL.texture);
    SDL_DestroyRenderer(OS_SDL.renderer);
    SDL_DestroyWindow(OS_SDL.window);
    SDL_Quit();
    return 0;
}

void backend_display_row_updated(int a, int b)
{
    int i0 = a < 0 ? 0 : a;
    int i1 = b >= SYS_DISPLAY_H ? SYS_DISPLAY_H - 1 : b;
    for (int i = i0; i <= i1; i++) {
        OS_SDL.update_row[i] = 1;
    }
}

// stream is an interlaced byte buffer: LLRRLLRRLL...
// len is buffer length in bytes (datatype size * channels * length)
static void backend_SDL_audio(void *unused, u8 *stream, int len)
{
    static i16 lbuf[0x1000];
    static i16 rbuf[0x1000];
    memset(lbuf, 0, sizeof(lbuf));
    memset(rbuf, 0, sizeof(rbuf));

    int samples = len / (2 * sizeof(i16));
    int active  = sys_audio_cb(NULL, lbuf, rbuf, samples);
    if (!active) {
        memset(stream, 0, len);
        return;
    }

    i16 *s         = (i16 *)stream;
    i16 *l         = lbuf;
    OS_SDL.is_mono = 1;

    if (OS_SDL.is_mono) {
        for (int n = 0; n < samples; n++) {
            int v = (int)((f32)*l++ * OS_SDL.vol);
            if (v < I16_MIN) v = I16_MIN;
            if (v > I16_MAX) v = I16_MAX;
            *s++ = v;
            *s++ = v;
        }
    } else {
        i16 *r = (i16 *)rbuf;
        for (int n = 0; n < samples; n++) {
            *s++ = (i16)((f32)*l++ * OS_SDL.vol);
            *s++ = (i16)((f32)*r++ * OS_SDL.vol);
        }
    }
}

void backend_modify_audio()
{
    SDL_LockAudioDevice(OS_SDL.audiodevID);
    SDL_UnlockAudioDevice(OS_SDL.audiodevID);
}

static SDL_GameController *backend_SDL_gamecontroller()
{
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i))
            return SDL_GameControllerOpen(i);
    }
    return NULL;
}

static bool32 backend_SDL_hit_pause()
{
    SDL_GameController *c    = backend_SDL_gamecontroller();
    const Uint8        *keys = SDL_GetKeyboardState(NULL);

    return (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_START) ||
            keys[SDL_SCANCODE_RETURN]);
}

int backend_inp()
{
    int b = 0;

    SDL_GameController *c    = backend_SDL_gamecontroller();
    const Uint8        *keys = SDL_GetKeyboardState(NULL);

    int ax = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    int ay = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
    if (ay > +1000) b |= SYS_INP_DPAD_D;
    if (ay < -1000) b |= SYS_INP_DPAD_U;
    if (ax > +1000) b |= SYS_INP_DPAD_R;
    if (ax < -1000) b |= SYS_INP_DPAD_L;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_B)) b |= SYS_INP_A;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_A)) b |= SYS_INP_B;

    if (keys[SDL_SCANCODE_W]) b |= SYS_INP_DPAD_U;
    if (keys[SDL_SCANCODE_S]) b |= SYS_INP_DPAD_D;
    if (keys[SDL_SCANCODE_A]) b |= SYS_INP_DPAD_L;
    if (keys[SDL_SCANCODE_D]) b |= SYS_INP_DPAD_R;
    if (keys[SDL_SCANCODE_PERIOD]) b |= SYS_INP_A;
    if (keys[SDL_SCANCODE_COMMA]) b |= SYS_INP_B;
    return b;
}

int backend_key(int key)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    return keys[key];
}

float backend_crank()
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP]) OS_SDL.crank += 0.01f;
    if (keys[SDL_SCANCODE_DOWN]) OS_SDL.crank -= 0.01f;
    if (OS_SDL.crank < 0.f) OS_SDL.crank += 1.f;
    if (OS_SDL.crank > 1.f) OS_SDL.crank -= 1.f;

    return OS_SDL.crank;
}

int backend_crank_docked()
{
    return 0;
}

f32 backend_seconds()
{
    u64 dt_counter = SDL_GetPerformanceCounter() - OS_SDL.timeorigin;
    return (f32)dt_counter / (f32)SDL_GetPerformanceFrequency();
}

u8 *backend_framebuffer()
{
    return OS_SDL.framebuffer;
}

void *backend_file_open(const char *path, int mode)
{
    switch (mode) {
    case SYS_FILE_R: return (void *)SDL_RWFromFile(path, "rb");
    case SYS_FILE_W: return (void *)SDL_RWFromFile(path, "w");
    }
    return NULL;
}

int backend_file_close(void *f)
{
    int i = SDL_RWclose((SDL_RWops *)f);
    return i;
}

int backend_file_read(void *f, void *buf, usize bufsize)
{
    usize s = SDL_RWread((SDL_RWops *)f, buf, 1, bufsize);
    return (int)s;
}

int backend_file_write(void *f, const void *buf, usize bufsize)
{
    usize s = SDL_RWwrite((SDL_RWops *)f, buf, 1, bufsize);
    return (int)s;
}

int backend_file_tell(void *f)
{
    i64 i = SDL_RWtell((SDL_RWops *)f);
    return (int)i;
}

int backend_file_seek(void *f, int pos, int origin)
{
    i64 i = SDL_RWseek((SDL_RWops *)f, (i64)pos, origin);
    return (int)i;
}

int backend_file_remove(const char *path)
{
    return remove(path);
}

int backend_debug_space()
{
    int       n_keys;
    const u8 *keys = SDL_GetKeyboardState(&n_keys);
    return (keys[SDL_SCANCODE_SPACE]);
}

void backend_set_menu_image(u8 *px, int h, int wbyte)
{
    int y2 = SYS_DISPLAY_H < h ? SYS_DISPLAY_H : h;
    int b2 = SYS_DISPLAY_WBYTES < wbyte ? SYS_DISPLAY_WBYTES : wbyte;
    for (int y = 0; y < y2; y++) {
        for (int b = 0; b < b2; b++) {
            u8 p                                           = px[b + y * wbyte];
            OS_SDL.menuimg[b + y * SYS_DISPLAY_WBYTES]     = px[b + y * wbyte];
            OS_SDL.framebuffer[b + y * SYS_DISPLAY_WBYTES] = p;
        }
    }
    sys_display_update_rows(0, SYS_DISPLAY_H - 1);
}