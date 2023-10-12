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

// STATIC_ASSERT(0, "testassert");

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
                timeacc -= OS_FPS_DELTA;
                ups_counter++;
                os_spmem_clr();
                os_audio_tick();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                g_os.tick++;
                if (g_os.n_spmem > 0) {
                        PRINTF("WARNING: spmem is not reset\n_spmem");
                }
        }

        bool32 rendered = (timeaccp != timeacc);
        if (rendered) {
                fps_counter++;
                os_spmem_clr();
                game_draw(&g_gamestate);
#if OS_SHOW_FPS
                gfx_context_s ctx         = gfx_context_create(tex_get(0));
                char          fpstext[16] = {0};
                os_strcat_i32(fpstext, g_os.ups);
                os_strcat(fpstext, "|");
                os_strcat_i32(fpstext, g_os.fps);
                gfx_rec_fill(ctx, (rec_i32){0, 0, 48, 12});
                gfx_text_ascii(ctx, &g_os.fnt_tab[FNTID_DEBUG], fpstext, (v2_i32){2, 2});

#endif
                os_mark_display_rows(0, 239);
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
        PRINTF("Size game: %i kb\n", sgame / 1024);
        PRINTF("Size os: %i kb\n", sos / 1024);
        PRINTF("= %i kb\n", (sgame + sos) / 1024);
        g_os.lasttime = os_time();
        g_os.fps      = OS_FPS;
        g_os.ups      = OS_FPS;
}

void os_mark_display_rows(int a, int b)
{
#ifdef OS_PLAYDATE
        PD->graphics->markUpdatedRows(a, b);
#else
        int i0 = max_i(a, 0);
        int i1 = min_i(b, 239);
        for (int i = i0; i <= i1; i++) {
                g_os.rows_update[i] = 1;
        }
#endif
}

i32 os_tick()
{
        return g_os.tick;
}

bool32 os_low_fps()
{
        return (g_os.fps <= OS_FPS_LOW);
}

int os_inp_raw()
{
        return g_os.buttons;
}

int os_inpp_raw()
{
        return g_os.buttonsp;
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