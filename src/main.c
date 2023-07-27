#include "game/game.h"
#include "os/os_internal.h"

os_s   g_os;
game_s g_gamestate;

static inline void os_tick()
{
        static float timeacc;

        float time = os_time();
        timeacc += time - g_os.lasttime;
        if (timeacc > OS_DELTA_CAP) timeacc = OS_DELTA_CAP;
        g_os.lasttime     = time;
        float timeacc_tmp = timeacc;
        while (timeacc >= OS_FPS_DELTA) {
                timeacc -= OS_FPS_DELTA;
                os_spmem_clr();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                if (g_os.n_spmem > 0) {
                        PRINTF("WARNING: spmem is not reset\n_spmem");
                }
        }

        if (timeacc != timeacc_tmp) {
                os_spmem_clr();
                gfx_draw_to(g_os.tex_display);
                os_backend_draw_begin();
                game_draw(&g_gamestate);
                os_backend_draw_end();
        }
        os_backend_flip();
}

#ifdef TARGET_DESKTOP
// DESKTOP ====================================================================
#define OS_DESKTOP_SCALE 2

int main()
{
        InitWindow(400 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE, "rl");
        InitAudioDevice();
        Image img = GenImageColor(416, 240, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(60);
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        g_os.tex_display = (tex_s){g_os.framebuffer, 52, 400, 240};
        os_graphics_init(g_os.tex_display);
        os_audio_init();
        game_init(&g_gamestate);
        g_os.lasttime = os_time();
        while (!WindowShouldClose()) {
                os_tick();
        }

        UnloadTexture(g_os.tex);
        CloseAudioDevice();
        CloseWindow();
        return 0;
}

void os_backend_inp_update()
{
        g_os.buttonsp = g_os.buttons;
        g_os.buttons  = 0;
        if (IsKeyDown(KEY_A)) {
                g_os.buttons |= INP_LEFT;
        }
        if (IsKeyDown(KEY_D)) {
                g_os.buttons |= INP_RIGHT;
        }
        if (IsKeyDown(KEY_W)) {
                g_os.buttons |= INP_UP;
        }
        if (IsKeyDown(KEY_S)) {
                g_os.buttons |= INP_DOWN;
        }
        if (IsKeyDown(KEY_COMMA)) {
                g_os.buttons |= INP_B;
        }
        if (IsKeyDown(KEY_PERIOD)) {
                g_os.buttons |= INP_A;
        }
        g_os.crankdockedp = g_os.crankdocked;
        g_os.crankdocked  = 1;
        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = 0;
        }
}

void os_backend_draw_begin()
{
        os_memclr4(g_os.framebuffer, sizeof(g_os.framebuffer));
}

void os_backend_draw_end()
{
        static const Color tab_rgb[2] = {0x38, 0x2B, 0x26, 0xFF,
                                         0xB8, 0xC2, 0xB9, 0xFF};

        for (int y = 0; y < 240; y++) {
                for (int x = 0; x < 400; x++) {
                        int i   = (x >> 3) + y * 52;
                        int k   = x + y * 416;
                        int byt = g_os.framebuffer[i];
                        int bit = (byt & (0x80 >> (x & 7)));

                        g_os.texpx[k] = tab_rgb[bit > 0];
                }
        }
        UpdateTexture(g_os.tex, g_os.texpx);
}

void os_backend_flip()
{
        static const Rectangle rsrc     = {0, 0, 416, 240};
        static const Rectangle rdst     = {0, 0,
                                           416 * OS_DESKTOP_SCALE,
                                           240 * OS_DESKTOP_SCALE};
        static const Vector2   vorg     = {0, 0};
        static const Color     clearcol = {0, 0, 0, 0xFF};
        static const Color     col      = {0xFF, 0xFF, 0xFF, 0xFF};
        BeginDrawing();
        ClearBackground(clearcol);
        DrawTexturePro(g_os.tex, rsrc, rdst, vorg, 0.f, col);
        EndDrawing();
}

#elif defined(TARGET_PD)
// PLAYDATE ===================================================================
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
                g_os.framebuffer = PD->graphics->getFrame();
                memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
                g_os.tex_display = (tex_s){g_os.framebuffer, 52, 400, 240};
                os_graphics_init(g_os.tex_display);
                os_audio_init();
                game_init(&g_gamestate);
                PD->system->resetElapsedTime();
                g_os.lasttime = os_time();
                break;
        }
        return 0;
}

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

#endif