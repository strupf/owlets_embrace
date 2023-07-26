#include "os.h"
#include "game.h"
#include "os/os_internal.h"

os_s   g_os;
game_s g_gamestate;

void os_tick()
{
        float time = os_time();
        g_os.time_acc += time - g_os.lasttime;
        g_os.lasttime = time;

        bool32 ticked = 0;
        while (g_os.time_acc >= OS_FPS_DELTA) {
                g_os.time_acc -= OS_FPS_DELTA;
                ticked = 1;
                os_spmem_clr();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                if (g_os.n_spmem > 0) {
                        PRINTF("WARNING: spmem is not reset\n_spmem");
                }
        }

        if (ticked) {
                os_spmem_clr();
                os_backend_draw_begin();
                game_draw(&g_gamestate);
                os_backend_draw_end();
        }
        os_backend_flip();
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
        return 0;
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