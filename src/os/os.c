// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"
#include "os_internal.h"

os_s g_os;

tex_s g_tex_screen;
tex_s g_tex_layer_1;
tex_s g_tex_layer_2;
tex_s g_tex_layer_3;

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
        timeacc       = min_f(timeacc + timedt, OS_DELTA_CAP);
        g_os.lasttime = time;

        const float timeaccp = timeacc;
        while (timeacc >= OS_FPS_DELTA) {
                TIMING_TICK_DIAGRAM();
                TIMING_BEGIN(TIMING_UPDATE);
                timeacc -= OS_FPS_DELTA;
                ups_counter++;
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

        bool32 rendered = (timeaccp != timeacc);
        if (rendered) {
                fps_counter++;
                TIMING_BEGIN(TIMING_DRAW);
                os_spmem_clr();
                game_draw(&g_gamestate);
                TIMING_END();
#if OS_SHOW_FPS
                gfx_context_s ctx         = gfx_context_create(tex_get(0));
                char          fpstext[16] = {0};
                os_strcat_i32(fpstext, g_os.ups);
                os_strcat(fpstext, "|");
                os_strcat_i32(fpstext, g_os.fps);
                gfx_rec_fill(ctx, (rec_i32){0, 0, 48, 12});
                gfx_text_ascii(ctx, &g_os.fnt_tab[FNTID_DEBUG], fpstext, (v2_i32){2, 2});
#endif
                TIMING_DRAW_DIAGRAM();
                os_backend_graphics_end();
        }

        if (timeacc_fps >= 1.f) {
                timeacc_fps -= 1.f;
                g_os.fps    = fps_counter;
                g_os.ups    = ups_counter;
                ups_counter = 0;
                fps_counter = 0;
        }

        os_backend_graphics_flip();
        return rendered;
}

void os_prepare()
{
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        memarena_init(&g_os.assetmem, g_os.assetmem_raw, OS_ASSETMEM_SIZE);
        os_backend_init();

        int ii = log2_32(77);
        PRINTF("log2: %i\n", ii);

        g_tex_screen  = (tex_s){g_os.framebuffer, NULL, 13, 52, 400, 240};
        g_tex_layer_1 = tex_create(400, 240, 1);
        g_tex_layer_2 = tex_create(400, 240, 1);
        g_tex_layer_3 = tex_create(400, 240, 1);

        g_os.tex_tab[0] = g_tex_screen;
        g_os.g_layer_1  = g_tex_layer_1;
        game_init(&g_gamestate);

        tex_put(TEXID_LAYER_1, g_tex_layer_1);
        size_t sgame = sizeof(g_gamestate) +
                       sizeof(g_tileanimations) +
                       sizeof(g_tileIDs) +
                       sizeof(tilecolliders);
        size_t sos = sizeof(os_s);
        PRINTF("\n");
        PRINTF("Size game: %lli kb\n", sgame / 1024);
        PRINTF("Size os: %lli kb\n", sos / 1024);
        PRINTF("= %lli kb\n", (sgame + sos) / 1024);
        g_os.lasttime = os_time();
        g_os.fps      = OS_FPS;
        g_os.ups      = OS_FPS;
}

i32 os_tick()
{
        return g_os.tick;
}

bool32 os_low_fps()
{
        return (g_os.fps <= OS_FPS_LOW);
}

/* NW  N  NE         Y -1
 *   \ | /           |
 * W__\|/___E   -1 --+--> X +1
 *    /|\            |
 *   / | \           V +1
 * SW  S  SE
 */
int os_inp_dpad_direction()
{
        int m = ((os_inp_dpad_x() + 1) << 2) | (os_inp_dpad_y() + 1);
        switch (m) {
        case 0x0: return INP_DPAD_NW;   // 0000 (-1 | -1)
        case 0x1: return INP_DPAD_W;    // 0001 (-1 |  0)
        case 0x2: return INP_DPAD_SW;   // 0010 (-1 | +1)
        case 0x4: return INP_DPAD_N;    // 0100 ( 0 | -1)
        case 0x5: return INP_DPAD_NONE; // 0101 ( 0 |  0)
        case 0x6: return INP_DPAD_S;    // 0110 ( 0 | +1)
        case 0x8: return INP_DPAD_NE;   // 1000 (+1 | -1)
        case 0x9: return INP_DPAD_E;    // 1001 (+1 |  0)
        case 0xA: return INP_DPAD_SE;   // 1010 (+1 | +1)
        }
        ASSERT(0);
        return 0;
}

int os_inp_dpad_x()
{
        if (os_inp_pressed(INP_LEFT))
                return -1;
        if (os_inp_pressed(INP_RIGHT))
                return +1;
        return 0;
}

int os_inp_dpad_y()
{
        if (os_inp_pressed(INP_UP))
                return -1;
        if (os_inp_pressed(INP_DOWN))
                return +1;
        return 0;
}

void os_inp_set_pressedp(int b)
{
        g_os.buttonsp |= b;
}

bool32 os_inp_pressed(int b)
{
        return (g_os.buttons & b);
}

bool32 os_inp_pressedp(int b)
{
        return (g_os.buttonsp & b);
}

bool32 os_inp_just_released(int b)
{
        return (!(g_os.buttons & b) && (g_os.buttonsp & b));
}

bool32 os_inp_just_pressed(int b)
{
        return ((g_os.buttons & b) && !(g_os.buttonsp & b));
}

int os_inp_crank_change()
{
        i32 dt = g_os.crank_q16 - g_os.crankp_q16;
        if (dt <= -32768) return (dt + 65536);
        if (dt >= +32768) return (dt - 65536);
        return dt;
}

int os_inp_crank()
{
        return g_os.crank_q16;
}

int os_inp_crankp()
{
        return g_os.crankp_q16;
}

bool32 os_inp_crank_dockedp()
{
        return g_os.crankdockedp;
}

bool32 os_inp_crank_docked()
{
        return g_os.crankdocked;
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

#if OS_SHOW_TIMING
#define TIMING_FRAMES     64
#define TIMING_RATE       16
#define DIAGRAM_MAX_Y     17
#define DIAGRAM_SPACING_Y 20
#define DIAGRAM_W         TIMING_FRAMES
#define DIAGRAM_H         (NUM_TIMING * DIAGRAM_SPACING_Y + 1)

typedef struct {
        int   ID;
        float time;
} debugtime_s;

static struct {
        debugtime_s stack[4];
        int         nstack;
        char        labels[NUM_TIMING][16];
        float       times[NUM_TIMING][TIMING_FRAMES];
        int         n;
} g_timing;

void i_time_tick()
{
        static int timingcounter;
        timingcounter++;
        if (timingcounter == TIMING_RATE) {
                timingcounter = 0;
                g_timing.n    = (g_timing.n + 1) & (TIMING_FRAMES - 1);
                for (int n = 0; n < NUM_TIMING; n++) {
                        g_timing.times[n][g_timing.n] = 0.f;
                }
        }
}

void i_time_draw()
{
        static tex_s  tdiagram;
        static bool32 is_init = 0;
        if (!is_init) {
                is_init  = 1;
                tdiagram = tex_create(DIAGRAM_W, DIAGRAM_H * 2, 1);
                os_memset(tdiagram.mk, 0xFF, tdiagram.w_byte * tdiagram.h);

                gfx_context_s ctx = gfx_context_create(tdiagram);
                ctx.col           = 1;

                for (int n = 0; n < NUM_TIMING; n++) {
                        rec_i32 r = {0, (1 + n) * DIAGRAM_SPACING_Y, TIMING_FRAMES, 1};
                        gfx_rec_fill(ctx, r);
                }

                os_strcat(g_timing.labels[TIMING_UPDATE], "tick");
                os_strcat(g_timing.labels[TIMING_ROPE], "rope");
                os_strcat(g_timing.labels[TIMING_DRAW_TILES], "tiles");
                os_strcat(g_timing.labels[TIMING_DRAW], "draw");
                os_strcat(g_timing.labels[TIMING_HERO_UPDATE], "hero");
                os_strcat(g_timing.labels[TIMING_SOLID_UPDATE], "solid");
                os_strcat(g_timing.labels[TIMING_HERO_MOVE], "h_move");
                os_strcat(g_timing.labels[TIMING_HERO_HOOK], "h_hook");
        }

        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.src           = tdiagram;

        int u = (g_timing.n >> 3);
        int s = 1 << (7 - (g_timing.n & 7));
        int x = g_timing.n;
        for (int y = 0; y < tdiagram.h; y++) // clear current coloumn
                tdiagram.px[u + y * tdiagram.w_byte] &= ~s;

        for (int n = 0; n < NUM_TIMING; n++) { // draw timing bar in diagrams
                int y2 = (n + 1) * DIAGRAM_SPACING_Y;
                int y1 = y2 - MIN(DIAGRAM_MAX_Y, (int)(g_timing.times[n][x] * 100000.f));
                for (int y = y1; y <= y2; y++)
                        tdiagram.px[u + y * tdiagram.w_byte] |= s;
        }

        gfx_sprite(ctx, (v2_i32){0, 0}, (rec_i32){0, 0, DIAGRAM_W, DIAGRAM_H}, 0);
        for (int y = 0; y <= DIAGRAM_H * 52; y += 2 * 52) // draw vertical marker line for current time
                g_os.framebuffer[u + y] |= s;

        for (int n = 0; n < NUM_TIMING; n++) { // draw labels
                int   y = n * DIAGRAM_SPACING_Y + 7;
                fnt_s f = g_os.fnt_tab[FNTID_DEBUG];
                gfx_text_ascii(ctx, &f, g_timing.labels[n], (v2_i32){DIAGRAM_W + 2, y});
        }
}

void i_time_begin(int ID)
{
        debugtime_s *dt = &g_timing.stack[g_timing.nstack++];
        dt->ID          = ID;
        dt->time        = os_time();
}

void i_time_end()
{
        debugtime_s dt                    = g_timing.stack[--g_timing.nstack];
        float       time                  = os_time() - dt.time;
        g_timing.times[dt.ID][g_timing.n] = max_f(time, g_timing.times[dt.ID][g_timing.n]);
}
#endif