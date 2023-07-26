#ifdef TARGET_DESKTOP
#include "os/os_internal.h"

#define OS_DESKTOP_SCALE 1

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

int main()
{
        g_os.lasttime = os_time();
        InitWindow(400 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE, "rl");
        InitAudioDevice();
        Color col = {0};
        Image img = GenImageColor(416, 240, col);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(60);
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        tex_s framebuffer = {g_os.framebuffer, 52, 400, 240};
        os_graphics_init(framebuffer);
        os_audio_init();
        game_init(&g_gamestate);

        while (!WindowShouldClose()) {
                os_tick();
        }

        UnloadTexture(g_os.tex);
        CloseAudioDevice();
        CloseWindow();
        return 0;
}
#endif