// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"
#include "os/os_internal.h"

#define DIAGRAM_ENABLED   0
#define DIAGRAM_MAX_Y     17
#define DIAGRAM_SPACING_Y 20
#define DIAGRAM_W         TIMING_FRAMES
#define DIAGRAM_H         (NUM_TIMING * DIAGRAM_SPACING_Y + 1)

static inline void os_prepare();
static inline int  os_do_tick();

void os_backend_graphics_init();
void os_backend_graphics_close();
void os_backend_audio_init();
void os_backend_audio_close();
void os_backend_inp_init();
void os_backend_inp_update();
void os_backend_graphics_begin();
void os_backend_graphics_end();
void os_backend_graphics_flip();

os_s   g_os;
game_s g_gamestate;

#ifdef TARGET_DESKTOP
// DESKTOP =====================================================================
int main()
{
        os_prepare();
        while (!WindowShouldClose()) {
                os_do_tick();
        }

        os_backend_audio_close();
        os_backend_graphics_close();
        return 0;
}
#elif defined(TARGET_PD)
// PLAYDATE ====================================================================
PlaydateAPI *PD;
void (*PD_log)(const char *fmt, ...);
float (*PD_elapsedtime)();

int os_do_tick_pd(void *userdata)
{
        return os_do_tick();
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg)
{
        switch (event) {
        case kEventInit:
                PD             = pd;
                PD_log         = PD->system->logToConsole;
                PD_elapsedtime = PD->system->getElapsedTime;
                PD->system->setUpdateCallback(os_do_tick_pd, PD);
                PD->system->resetElapsedTime();
                os_prepare();
                break;
        }
        return 0;
}

#endif
// =============================================================================

static tex_s tdiagram;

static void draw_frame_diagrams()
{
        int u = (g_os.timings.n >> 3);
        int s = 1 << (7 - (g_os.timings.n & 7));
        int x = g_os.timings.n;
        for (int y = 0; y < tdiagram.h; y++) {
                tdiagram.px[u + y * tdiagram.w_byte] &= ~s;
        }

        for (int n = 0; n < NUM_TIMING; n++) {
                int pos = (n + 1) * DIAGRAM_SPACING_Y;
                int y1  = (int)(g_os.timings.times[n][x] * 1000000.f);
                y1      = pos - MIN(y1, DIAGRAM_MAX_Y);
                for (int y = y1; y <= pos; y++) {
                        tdiagram.px[u + y * tdiagram.w_byte] |= s;
                }
        }

        gfx_sprite_fast(tdiagram, (v2_i32){0, 0},
                        (rec_i32){0, 0, DIAGRAM_W, DIAGRAM_H});
        for (int y = 0; y <= DIAGRAM_H * 52; y += 2 * 52) {
                int i = u + y;
                g_os.framebuffer[i] |= s;
        }
        fnt_s font = fnt_get(FNTID_DEBUG);
        for (int n = 0; n < NUM_TIMING; n++) {
                int pos = n * DIAGRAM_SPACING_Y + 7;
                gfx_text_ascii(&font, g_os.timings.labels[n],
                               DIAGRAM_W + 2, pos);
        }
}

static void frame_diagram()
{
        tdiagram = tex_create(DIAGRAM_W, DIAGRAM_H * 2, 1);
        os_memset(tdiagram.mk, 0xFF, tdiagram.w_byte * tdiagram.h);
        gfx_draw_to(tdiagram);
        for (int n = 0; n < NUM_TIMING; n++) {
                rec_i32 r = {0, (1 + n) * DIAGRAM_SPACING_Y, TIMING_FRAMES, 1};
                gfx_rec_fill(r, 1);
        }

        os_strcat(g_os.timings.labels[TIMING_UPDATE], "tick");
        os_strcat(g_os.timings.labels[TIMING_ROPE], "rope");
        os_strcat(g_os.timings.labels[TIMING_DRAW_TILES], "tiles");
        os_strcat(g_os.timings.labels[TIMING_DRAW], "draw");
        os_strcat(g_os.timings.labels[TIMING_HERO_UPDATE], "hero");
        os_strcat(g_os.timings.labels[TIMING_SOLID_UPDATE], "solid");
        os_strcat(g_os.timings.labels[TIMING_HERO_MOVE], "h_move");
        os_strcat(g_os.timings.labels[TIMING_HERO_HOOK], "h_hook");
}

static inline int os_do_tick()
{
        static float timeacc;

        float time = os_time();
        timeacc += time - g_os.lasttime;
        if (timeacc > OS_DELTA_CAP) timeacc = OS_DELTA_CAP;
        g_os.lasttime            = time;
        float      timeacc_tmp   = timeacc;
        static int timingcounter = 0;
        while (timeacc >= OS_FPS_DELTA) {
                timingcounter++;
                if (timingcounter == TIMING_RATE) {
                        timingcounter  = 0;
                        g_os.timings.n = (g_os.timings.n + 1) & (TIMING_FRAMES - 1);
                        for (int n = 0; n < NUM_TIMING; n++) {
                                g_os.timings.times[n][g_os.timings.n] = 0.f;
                        }
                }

                TIMING_BEGIN(TIMING_UPDATE);
                timeacc -= OS_FPS_DELTA;
                os_spmem_clr();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                g_os.tick++;
                if (g_os.n_spmem > 0) {
                        // PRINTF("WARNING: spmem is not reset\n_spmem");
                }
                TIMING_END();
        }

        if (timeacc != timeacc_tmp) {
#if DIAGRAM_ENABLED
                TIMING_BEGIN(TIMING_DRAW);
#endif
                os_spmem_clr();
                g_os.dst = g_os.tex_tab[0];
                os_backend_graphics_begin();
                game_draw(&g_gamestate);

#if DIAGRAM_ENABLED
                TIMING_END();
                draw_frame_diagrams();
#endif
                os_backend_graphics_end();
                os_backend_graphics_flip();
                return 1; // update the display
        }
        os_backend_graphics_flip();
        return 0; // not rendered, don't update display
}

static inline void os_prepare()
{
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        memarena_init(&g_os.assetmem, g_os.assetmem_raw, OS_ASSETMEM_SIZE);
        os_backend_graphics_init();
        os_backend_audio_init();
        os_backend_inp_init();
        g_os.tex_tab[0] = (tex_s){g_os.framebuffer, NULL, 13, 52, 400, 240};
        game_init(&g_gamestate);

        size_t sgame = sizeof(game_s) / 1024;
        size_t sos   = sizeof(os_s) / 1024;
        PRINTF("\n");
        PRINTF("Size game: %lli kb\n", sgame);
        PRINTF("Size os: %lli kb\n", sos);
        PRINTF("= %lli kb\n", sgame + sos);
        frame_diagram();
        g_os.lasttime = os_time();
}

i32 os_tick()
{
        return g_os.tick;
}

typedef struct {
        int   ID;
        float time;
} debugtime_s;

static struct {
        debugtime_s stack[4];
        int         n;
} g_debugtime;

void i_time_begin(int ID)
{
        debugtime_s *dt = &g_debugtime.stack[g_debugtime.n++];
        dt->ID          = ID;
        dt->time        = os_time();
}

void i_time_end()
{
        debugtime_s dt        = g_debugtime.stack[--g_debugtime.n];
        float       time      = os_time() - dt.time;
        timings_s  *t         = &g_os.timings;
        t->times[dt.ID][t->n] = MAX(time, t->times[dt.ID][t->n]);
}