// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys.h"
#include "sys_backend.h"
//
#include "SDL2/SDL.h"
#include <stdio.h>

#define SYS_SDL_SCALE             1
#define SYS_USE_INTEGER_SCALING   1
#define SYS_REDUCE_FLICKER        1 // space to swap at runtime
#define SYS_SDL_COLOR_BLACK_WHITE 1

static_assert(SYS_FILE_SEEK_SET == RW_SEEK_SET, "seek");
static_assert(SYS_FILE_SEEK_CUR == RW_SEEK_CUR, "seek");
static_assert(SYS_FILE_SEEK_END == RW_SEEK_END, "seek");

enum {
    BE_MENU_ITEM_CHECKMARK,
    BE_MENU_ITEM_FUNCTION,
};

typedef struct {
    int type;
    int value;
    void (*cb)(void *arg);
    void *arg;
    char  title[32];
} SDL_menu_item_s;

static struct {
    bool32            running;
    Uint64            timeorigin;
    u32               fps_cap;
    f64               fps_cap_dt;
    f64               delay_timer;
    f64               delay_timer_k;
    i32               n_menu_items;
    SDL_menu_item_s   menu_items[8];
    //
    u8                framebuffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
    u8                menuimg[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
    u8                update_row[SYS_DISPLAY_H];
    SDL_Window       *window;
    SDL_Renderer     *renderer;
    SDL_Texture      *texture;
    SDL_Rect          r_src;
    SDL_Rect          r_dst;
    SDL_AudioDeviceID audiodevID;
    SDL_AudioSpec     audiospec;
    bool32            is_mono;
    i32               inv;
    f32               vol;
    f32               crank;
    i32               pausebtn;
    i32               pausebtnp;
    bool32            paused;
} OS_SDL;

static void                backend_SDL_audio(void *u, Uint8 *stream, int len);
static SDL_GameController *backend_SDL_gamecontroller();
static bool32              backend_SDL_hit_pause();

static void SDL_on_resize()
{
    int w, h;
    SDL_GetWindowSize(OS_SDL.window, &w, &h);
    f32 sx = (f32)w / (f32)OS_SDL.r_src.w;
    f32 sy = (f32)h / (f32)OS_SDL.r_src.h;
#if SYS_USE_INTEGER_SCALING // patterns look terrible stretched
    int si         = (int)(sx <= sy ? sx : sy);
    OS_SDL.r_dst.w = OS_SDL.r_src.w * si;
    OS_SDL.r_dst.h = OS_SDL.r_src.h * si;
#else
    if (sx < sy) {
        OS_SDL.r_dst.w = w;
        OS_SDL.r_dst.h = (int)(sx * (f32)OS_SDL.r_src.h + .5f);
    } else {
        OS_SDL.r_dst.w = (int)(sy * (f32)OS_SDL.r_src.w + .5f);
        OS_SDL.r_dst.h = h;
    }
#endif
    OS_SDL.r_dst.x = (w - OS_SDL.r_dst.w) / 2;
    OS_SDL.r_dst.y = (h - OS_SDL.r_dst.h) / 2;
}

int main(int argc, char **argv)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

    OS_SDL.window = SDL_CreateWindow("Owlet's Embrace",
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     SYS_DISPLAY_W * SYS_SDL_SCALE + 16,
                                     SYS_DISPLAY_H * SYS_SDL_SCALE + 16,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);
    SDL_SetWindowMinimumSize(OS_SDL.window, SYS_DISPLAY_W, SYS_DISPLAY_H);
    OS_SDL.renderer = SDL_CreateRenderer(OS_SDL.window, -1, 0);
    SDL_SetRenderDrawColor(OS_SDL.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(OS_SDL.renderer, &info);
    Uint32 pformat = info.texture_formats[0];
    OS_SDL.texture = SDL_CreateTexture(OS_SDL.renderer,
                                       pformat,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SYS_DISPLAY_W,
                                       SYS_DISPLAY_H);

    SDL_PixelFormat *f = SDL_AllocFormat(pformat);

    const Uint32 pal[2] = {
#if SYS_SDL_COLOR_BLACK_WHITE
        SDL_MapRGB((const SDL_PixelFormat *)f, 0x00, 0x00, 0x00), // black
        SDL_MapRGB((const SDL_PixelFormat *)f, 0xFF, 0xFF, 0xFF)  // white
#else
        SDL_MapRGB((const SDL_PixelFormat *)f, 0x31, 0x2F, 0x28), // black
        SDL_MapRGB((const SDL_PixelFormat *)f, 0xB1, 0xAF, 0xA8)  // white
#endif
    };
    SDL_FreeFormat(f);

    SDL_AudioSpec frmt = {0};
    frmt.channels      = 2;
    frmt.freq          = 44100;
    frmt.format        = AUDIO_S16;
    frmt.samples       = 256;
    frmt.callback      = backend_SDL_audio;

    OS_SDL.audiodevID = SDL_OpenAudioDevice(NULL, 0, &frmt, &OS_SDL.audiospec, 0);
    if (OS_SDL.audiodevID) {
        SDL_PauseAudioDevice(OS_SDL.audiodevID, 0);
    }

    OS_SDL.running    = 1;
    OS_SDL.r_src.w    = SYS_DISPLAY_W;
    OS_SDL.r_src.h    = SYS_DISPLAY_H;
    OS_SDL.r_dst.w    = SYS_DISPLAY_W * SYS_SDL_SCALE;
    OS_SDL.r_dst.h    = SYS_DISPLAY_H * SYS_SDL_SCALE;
    OS_SDL.timeorigin = SDL_GetPerformanceCounter();
    OS_SDL.is_mono    = 1;
    OS_SDL.vol        = 0.5f;
    backend_display_row_updated(0, SYS_DISPLAY_H - 1);
    SDL_on_resize();
    sys_init();
    sys_set_FPS(120);

    Uint64 time_prev     = SDL_GetPerformanceCounter();
    Uint64 fps_timer     = 0;
    i32    fps_tick      = 0;
    i32    fps_tick_prev = 0;

    while (OS_SDL.running) {
        Uint64 time   = SDL_GetPerformanceCounter();
        Uint64 timedt = time - time_prev;
        time_prev     = time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym != SDLK_ESCAPE) break;
            case SDL_QUIT: OS_SDL.running = 0; break;
            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED: {
                    SDL_on_resize();
                } break;
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

        bool32 draw_requested = 1;
        if (!OS_SDL.paused) {
            draw_requested = sys_step(NULL);
        }

        if (draw_requested) {
            int   pitch;
            void *pixelsptr;
            SDL_LockTexture(OS_SDL.texture, NULL, &pixelsptr, &pitch);
            {
                Uint32 *pixels = (Uint32 *)pixelsptr;

                for (int y = 0; y < SYS_DISPLAY_H; y++) {
                    if (!OS_SDL.update_row[y]) continue;
                    OS_SDL.update_row[y] = 0;
                    for (int x = 0; x < SYS_DISPLAY_W; x++) {
                        int i     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
                        int k     = x + y * SYS_DISPLAY_W;
                        int byt   = OS_SDL.framebuffer[i];
                        int bit   = !!(byt & (0x80 >> (x & 7)));
                        pixels[k] = pal[OS_SDL.inv ? !bit : bit];
                    }
                    if (!OS_SDL.paused) continue;
                    continue;
                    for (int x = 200; x < SYS_DISPLAY_W; x++) {
                        pixels[x + y * SYS_DISPLAY_W] = pal[(x & 1 ? 1 : 0)];
                    }
                }
            }
            SDL_UnlockTexture(OS_SDL.texture);
        }

        SDL_SetRenderDrawColor(OS_SDL.renderer, 0x31, 0x2F, 0x28, 0xFF);
        SDL_RenderClear(OS_SDL.renderer);
        SDL_RenderCopy(OS_SDL.renderer, OS_SDL.texture, &OS_SDL.r_src, &OS_SDL.r_dst);
        SDL_RenderPresent(OS_SDL.renderer);

        // FPS cap in SDL
        if (OS_SDL.fps_cap) {
            fps_tick++;
            fps_timer += timedt;
            Uint64 one_second = SDL_GetPerformanceFrequency();
            if (one_second <= fps_timer) {
                fps_timer -= one_second;
                f64 avg_fps  = (f64)(fps_tick + fps_tick_prev) * 0.5;
                f64 diff     = avg_fps - (f64)OS_SDL.fps_cap;
                f64 k_change = 0.0;
                f64 diff_abs = 0.0 <= diff ? diff : -diff;
                if (2.0 <= diff_abs) k_change = 0.025;
                else if (1.00 <= diff_abs) k_change = 0.010;
                else if (0.25 <= diff_abs) k_change = 0.001;
                if (0.0 < diff) OS_SDL.delay_timer_k += k_change;
                if (0.0 > diff) OS_SDL.delay_timer_k -= k_change;
                fps_tick_prev = fps_tick;
                fps_tick      = 0;
            }

            f64 tdt = (f64)(SDL_GetPerformanceCounter() - time) / (f64)one_second;
            OS_SDL.delay_timer += (OS_SDL.fps_cap_dt - tdt) * 1000.0 * OS_SDL.delay_timer_k;
            OS_SDL.delay_timer = 0.0 <= OS_SDL.delay_timer ? OS_SDL.delay_timer : 0.0;
            if (1.0 <= OS_SDL.delay_timer) {
                Uint32 sleepfor = (Uint32)OS_SDL.delay_timer;
                OS_SDL.delay_timer -= (f64)sleepfor;
                SDL_Delay(sleepfor);
            }
        }
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
    assert(0 <= a && b < SYS_DISPLAY_H);
    for (int i = a; i <= b; i++) {
        OS_SDL.update_row[i] = 1;
    }
}

// stream is an interlaced byte buffer: LLRRLLRRLL...
// len is buffer length in bytes (datatype size * channels * length)
static void backend_SDL_audio(void *u, Uint8 *stream, int len)
{
    static i16 lbuf[0x1000];
    static i16 rbuf[0x1000];
    memset(lbuf, 0, sizeof(lbuf));
    memset(rbuf, 0, sizeof(rbuf));

    int samples = len / (2 * sizeof(i16));
    int active  = sys_audio(NULL, lbuf, rbuf, samples);
    if (!active) {
        memset(stream, 0, len);
        return;
    }

    i16 *s = (i16 *)stream;
    i16 *l = lbuf;

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
    if (ay > +16384) b |= SYS_INP_DPAD_D;
    if (ay < -16384) b |= SYS_INP_DPAD_U;
    if (ax > +16384) b |= SYS_INP_DPAD_R;
    if (ax < -16384) b |= SYS_INP_DPAD_L;
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
    SDL_GameController *c    = backend_SDL_gamecontroller();
    const Uint8        *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP] ||
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
        OS_SDL.crank += 0.02f;
    if (keys[SDL_SCANCODE_DOWN] ||
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
        OS_SDL.crank -= 0.02f;
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
    Uint64 d = SDL_GetPerformanceCounter() - OS_SDL.timeorigin;
    return (f32)d / (f32)SDL_GetPerformanceFrequency();
}

u32 *backend_framebuffer()
{
    return (u32 *)OS_SDL.framebuffer;
}

u32 *backend_display_buffer()
{
    return backend_framebuffer();
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
    size_t s = SDL_RWread((SDL_RWops *)f, buf, 1, (size_t)bufsize);
    return (int)s;
}

int backend_file_write(void *f, const void *buf, usize bufsize)
{
    size_t s = SDL_RWwrite((SDL_RWops *)f, buf, 1, (size_t)bufsize);
    return (int)s;
}

int backend_file_tell(void *f)
{
    Sint64 i = SDL_RWtell((SDL_RWops *)f);
    return (int)i;
}

int backend_file_seek(void *f, int pos, int origin)
{
    Sint64 i = SDL_RWseek((SDL_RWops *)f, (Sint64)pos, origin);
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

bool32 backend_set_menu_image(void *px, int h, int wbyte)
{
    if (!px) {
        return 0;
    }
    int y2 = SYS_DISPLAY_H < h ? SYS_DISPLAY_H : h;
    int b2 = SYS_DISPLAY_WBYTES < wbyte ? SYS_DISPLAY_WBYTES : wbyte;
    for (int y = 0; y < y2; y++) {
        for (int b = 0; b < b2; b++) {
            int i                 = b + y * wbyte;
            int k                 = b + y * SYS_DISPLAY_WBYTES;
            OS_SDL.menuimg[k]     = ((u8 *)px)[i];
            OS_SDL.framebuffer[k] = ((u8 *)px)[i];
        }
    }
    sys_display_update_rows(0, SYS_DISPLAY_H - 1);
    return 1;
}

void backend_display_flush()
{
}

void backend_set_FPS(int fps)
{
    if (fps <= 0) {
        OS_SDL.fps_cap     = 0;
        OS_SDL.fps_cap_dt  = 0.;
        OS_SDL.delay_timer = 0.;
        return;
    }
    OS_SDL.delay_timer_k = 1.;
    OS_SDL.fps_cap       = fps;
    OS_SDL.fps_cap_dt    = 1. / (f64)fps;
}

SDL_menu_item_s *backend_menu_item_create(const char *title, void (*cb)(void *arg), void *arg)
{
    SDL_menu_item_s *mi = &OS_SDL.menu_items[OS_SDL.n_menu_items++];
    mi->cb              = cb;
    mi->arg             = arg;
    for (int i = 0;; i++) {
        mi->title[i] = title[i];
        if (title[i] == '\0') break;
    }
    return mi;
}

void *backend_menu_item_add(const char *title, void (*cb)(void *arg), void *arg)
{
    SDL_menu_item_s *mi = backend_menu_item_create(title, cb, arg);
    mi->type            = BE_MENU_ITEM_FUNCTION;
    return mi;
}

void *backend_menu_checkmark_add(const char *title, int val, void (*cb)(void *arg), void *arg)
{
    SDL_menu_item_s *mi = backend_menu_item_create(title, cb, arg);
    mi->type            = BE_MENU_ITEM_CHECKMARK;
    mi->value           = val;
    return mi;
}

void *backend_menu_options_add(const char *title, const char **options,
                               int count, void (*cb)(void *arg), void *arg)
{
    return NULL;
}

int backend_menu_value(void *ptr)
{
    if (!ptr) return 0;
    SDL_menu_item_s *mi = (SDL_menu_item_s *)ptr;
    return (mi->type == BE_MENU_ITEM_CHECKMARK ? mi->value : 0);
}

void backend_menu_clr()
{
    memset(OS_SDL.menu_items, 0, sizeof(OS_SDL.menu_items));
    OS_SDL.n_menu_items = 0;
}

void backend_set_volume(f32 vol)
{
    OS_SDL.vol = 0.f <= vol ? (vol <= 1.f ? vol : 1.f) : 0.f;
}

void backend_display_inv(int i)
{
    OS_SDL.inv = i;
}