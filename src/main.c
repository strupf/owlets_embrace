// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"
#include "os/backend.h"
#include "os/os_internal.h"

#define SHOW_FPS_UPS      0
#define DIAGRAM_ENABLED   0
#define DIAGRAM_MAX_Y     17
#define DIAGRAM_SPACING_Y 20
#define DIAGRAM_W         TIMING_FRAMES
#define DIAGRAM_H         (NUM_TIMING * DIAGRAM_SPACING_Y + 1)

os_s   g_os;
game_s g_gamestate;

static tex_s tdiagram;

static void draw_frame_diagrams()
{
        gfx_context_s ctx = gfx_context_create(tdiagram);
        ctx.col           = 1;

        int u = (g_os.timings.n >> 3);
        int s = 1 << (7 - (g_os.timings.n & 7));
        int x = g_os.timings.n;
        for (int y = 0; y < tdiagram.h; y++) {
                tdiagram.px[u + y * tdiagram.w_byte] &= ~s;
        }

        for (int n = 0; n < NUM_TIMING; n++) {
                int pos = (n + 1) * DIAGRAM_SPACING_Y;
                int y1  = (int)(g_os.timings.times[n][x] * 100000.f);
                y1      = pos - MIN(y1, DIAGRAM_MAX_Y);
                for (int y = y1; y <= pos; y++) {
                        tdiagram.px[u + y * tdiagram.w_byte] |= s;
                }
        }

        gfx_sprite(ctx, (v2_i32){0, 0},
                   (rec_i32){0, 0, DIAGRAM_W, DIAGRAM_H}, 0);
        for (int y = 0; y <= DIAGRAM_H * 52; y += 2 * 52) {
                int i = u + y;
                g_os.framebuffer[i] |= s;
        }

        for (int n = 0; n < NUM_TIMING; n++) {
                int pos = n * DIAGRAM_SPACING_Y + 7;
                gfx_text_ascii(ctx, &g_os.fnt_tab[FNTID_DEBUG], g_os.timings.labels[n],
                               (v2_i32){DIAGRAM_W + 2, pos});
        }
}

static void frame_diagram()
{

        tdiagram = tex_create(DIAGRAM_W, DIAGRAM_H * 2, 1);
        os_memset(tdiagram.mk, 0xFF, tdiagram.w_byte * tdiagram.h);

        gfx_context_s ctx = gfx_context_create(tdiagram);
        ctx.col           = 1;

        for (int n = 0; n < NUM_TIMING; n++) {
                rec_i32 r = {0, (1 + n) * DIAGRAM_SPACING_Y, TIMING_FRAMES, 1};
                gfx_rec_fill(ctx, r);
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

int os_do_tick()
{
        static float timeacc;
        static float timeacc_fps;
        static int   fps_counter; // frames per second
        static int   ups_counter; // updates per second
        static int   timingcounter;

        float time   = os_time();
        float timedt = time - g_os.lasttime;
        timeacc_fps += timedt;
        timeacc += timedt;
        if (timeacc > OS_DELTA_CAP) timeacc = OS_DELTA_CAP;
        g_os.lasttime = time;

        bool32 renderframe = 0;
        while (timeacc >= OS_FPS_DELTA) {
                timeacc -= OS_FPS_DELTA;
                renderframe = 1;
                timingcounter++;
                ups_counter++;
                if (timingcounter == TIMING_RATE) {
                        timingcounter  = 0;
                        g_os.timings.n = (g_os.timings.n + 1) & (TIMING_FRAMES - 1);
                        for (int n = 0; n < NUM_TIMING; n++) {
                                g_os.timings.times[n][g_os.timings.n] = 0.f;
                        }
                }

                TIMING_BEGIN(TIMING_UPDATE);
                os_spmem_clr();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                g_os.tick++;
                TIMING_END();
                if (g_os.n_spmem > 0) {
                        PRINTF("WARNING: spmem is not reset\n_spmem");
                }
        }

        if (renderframe) {
                fps_counter++;
                TIMING_BEGIN(TIMING_DRAW);
                os_spmem_clr();
                os_backend_graphics_begin();
                game_draw(&g_gamestate);
                TIMING_END();
#if SHOW_FPS_UPS
                gfx_context_s ctx         = gfx_context_create(tex_get(0));
                char          fpstext[16] = {0};
                os_strcat_i32(fpstext, g_os.ups);
                os_strcat(fpstext, "|");
                os_strcat_i32(fpstext, g_os.fps);
                gfx_rec_fill(ctx, (rec_i32){0, 0, 48, 12});
                gfx_text_ascii(ctx, &g_os.fnt_tab[FNTID_DEBUG], fpstext, (v2_i32){2, 2});
#endif

#if DIAGRAM_ENABLED
                draw_frame_diagrams();
#endif
                os_backend_graphics_end();
                os_backend_graphics_flip();
        }

        if (timeacc_fps >= 1.f) {
                timeacc_fps -= 1.f;
                g_os.fps    = fps_counter;
                g_os.ups    = ups_counter;
                ups_counter = 0;
                fps_counter = 0;
        }

        os_backend_graphics_flip();
        return renderframe; // not rendered, don't update display
}

void os_prepare()
{
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        memarena_init(&g_os.assetmem, g_os.assetmem_raw, OS_ASSETMEM_SIZE);
        os_backend_graphics_init();
        os_backend_audio_init();
        os_backend_inp_init();
        g_os.tex_tab[0] = (tex_s){g_os.framebuffer, NULL, 13, 52, 400, 240};
        game_init(&g_gamestate);
        g_os.g_layer_1 = tex_create(400, 240, 1);
        tex_put(TEXID_LAYER_1, g_os.g_layer_1);
        size_t sgame = sizeof(game_s) / 1024;
        size_t sos   = sizeof(os_s) / 1024;
        PRINTF("\n");
        PRINTF("Size game: %lli kb\n", sgame);
        PRINTF("Size os: %lli kb\n", sos);
        PRINTF("= %lli kb\n", sgame + sos);
        frame_diagram();
        g_os.lasttime = os_time();
        g_os.fps      = OS_FPS;
        g_os.ups      = OS_FPS;

        savefile_write(0, &g_gamestate);
        savefile_load(0, &g_gamestate);
}

i32 os_tick()
{
        return g_os.tick;
}

bool32 os_low_fps()
{
        return (g_os.fps <= OS_FPS_LOW);
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

void *assetmem_alloc(size_t s)
{
        return memarena_alloc(&g_os.assetmem, s);
}

void os_spmem_push()
{
        ASSERT(g_os.n_spmem < OS_SPMEM_STACK_HEIGHT);
        g_os.spmemstack[g_os.n_spmem++] = memarena_peek(&g_os.spmem);
}

void os_spmem_pop()
{
        ASSERT(g_os.n_spmem > 0);
        memarena_set(&g_os.spmem, g_os.spmemstack[--g_os.n_spmem]);
}

void *os_spmem_peek()
{
        return memarena_peek(&g_os.spmem);
}

void os_spmem_set(void *p)
{
        memarena_set(&g_os.spmem, p);
}

void os_spmem_clr()
{
        memarena_clr(&g_os.spmem);
        g_os.n_spmem = 0;
}

void *os_spmem_alloc(size_t size)
{
        return memarena_alloc(&g_os.spmem, size);
}

void *os_spmem_alloc_rems(size_t *size)
{
        return memarena_alloc_rem(&g_os.spmem, size);
}

void *os_spmem_allocz(size_t size)
{
        return memarena_allocz(&g_os.spmem, size);
}

void *os_spmem_allocz_rem(size_t *size)
{
        return memarena_allocz_rem(&g_os.spmem, size);
}