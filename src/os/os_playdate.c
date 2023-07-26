#ifdef TARGET_PD
#include "os/os_internal.h"

void os_backend_inp_update()
{
        g_os.buttonsp     = g_os.buttons;
        g_os.crankdockedp = g_os.crankdocked;
        PD_buttonstate(&g_os.buttons, NULL, NULL);
        g_os.crankdocked = PD_crankdocked();

        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = (int)(PD_crank() * 182.04444f) & 0xFFFF;
        }
}

void os_backend_draw_begin()
{
        os_memclr4(g_os.framebuffer, 52 * 240);
}

void os_backend_draw_end()
{
        PD_markUpdatedRows(0, LCD_ROWS - 1); // mark all rows as updated
        PD_drawFPS(0, 0);
        PD_display(); // update all rows
}

void os_backend_flip()
{
        PRINTF("PD");
}

PlaydateAPI *PD;
float (*PD_crank)(void);
int (*PD_crankdocked)(void);
void (*PD_buttonstate)(PDButtons *, PDButtons *, PDButtons *);
void (*PD_log)(const char *fmt, ...);
void (*PD_display)(void);
void (*PD_drawFPS)(int x, int y);
void (*PD_markUpdatedRows)(int start, int end);

int os_tick_pd(void *userdata)
{
        os_tick();
        return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg)
{
        switch (event) {
        case kEventInit:
                PD                 = pd;
                PD_crank           = PD->system->getCrankAngle;
                PD_crankdocked     = PD->system->isCrankDocked;
                PD_buttonstate     = PD->system->getButtonState;
                PD_log             = PD->system->logToConsole;
                PD_display         = PD->graphics->display;
                PD_drawFPS         = PD->system->drawFPS;
                PD_markUpdatedRows = PD->graphics->markUpdatedRows;
                PD->display->setRefreshRate(0.f);
                PD->system->setUpdateCallback(os_tick_pd, PD);
                PD->system->resetElapsedTime();
                g_os.framebuffer = PD->graphics->getFrame();
                memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
                tex_s framebuffer = {g_os.framebuffer, 52, 400, 240};
                os_graphics_init(framebuffer);
                os_audio_init();
                game_init(&g_gamestate);
                break;
        }
        return 0;
}
#endif