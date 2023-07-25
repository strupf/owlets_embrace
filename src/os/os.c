#include "os.h"

#define OS_FPS_DELTA       0.0166667f
#define DESKTOP_GAME_SCALE 1

static struct {
        float time_acc;
        float lasttime;

#if defined(TARGET_DESKTOP)
        u8        framebuffer[52 * 240];
        Color     texpx[416 * 240];
        Texture2D tex;
        int       scalingmode;
        rec_i32   view;
        int       buttons;
        int       buttonsp;
#else

#endif

        bool32 crankdocked;
        bool32 crankdockedp;
        i32    crank_q16;
        i32    crankp_q16;
} g_os;

#define SCRATCHMEM_SIZE         0x100000
#define SRCATCHMEM_STACK_HEIGHT 16
static struct {
        char           *p;
        int             n;
        char           *stack[SRCATCHMEM_STACK_HEIGHT];
        ALIGNAS(4) char byte[SCRATCHMEM_SIZE];
} g_scratchmem;

void os_inp_update()
{
        g_os.buttonsp     = g_os.buttons;
        g_os.buttons      = 0;
        g_os.crankdockedp = g_os.crankdocked;
        g_os.crankdocked  = 1;

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

        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = 0;
        }
}

void os_tick()
{
        float time = os_time();
        g_os.time_acc += time - g_os.lasttime;
        g_os.lasttime = time;

        bool32 ticked = 0;
        while (g_os.time_acc >= OS_FPS_DELTA) {
                g_os.time_acc -= OS_FPS_DELTA;
                ticked = 1;
                os_inp_update();
                scratchmem_clr();
                // game_update();
                if (g_scratchmem.n > 0) {
                        c_printf("WARNING: scratchmem is not reset\n");
                }
        }

        if (ticked) {
                scratchmem_clr();
#if defined(TARGET_DESKTOP)
                c_memclr4(g_os.framebuffer, sizeof(g_os.framebuffer));
#else

#endif

                // game_draw();

#if defined(TARGET_DESKTOP)
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
#else

#endif
        }

#if defined(TARGET_DESKTOP)
        static const Rectangle rsrc     = {0, 0, 416, 240};
        static const Rectangle rdst     = {0, 0,
                                           416 * DESKTOP_GAME_SCALE,
                                           240 * DESKTOP_GAME_SCALE};
        static const Vector2   vorg     = {0, 0};
        static const Color     clearcol = {0, 0, 0, 0xFF};
        static const Color     col      = {0xFF, 0xFF, 0xFF, 0xFF};
        BeginDrawing();
        ClearBackground(clearcol);
        DrawTexturePro(g_os.tex, rsrc, rdst, vorg, 0.f, col);
        EndDrawing();
#endif
}

#if defined(TARGET_DESKTOP)
int main()
{
        g_os.lasttime = os_time();
        InitWindow(400, 240, "rl");
        InitAudioDevice();
        Color col = {0};
        Image img = GenImageColor(416, 240, col);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
                os_tick();
        }

        UnloadTexture(g_os.tex);
        CloseAudioDevice();
        CloseWindow();
        return 0;
}
#elif defined(TARGET_PD)
PlaydateAPI *PD;
float (*PD_crank)(void);
int (*PD_crankdocked)(void);
void (*PD_buttonstate)(PDButtons *, PDButtons *, PDButtons *);
void (*PD_log)(const char *fmt, ...);
#endif

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

void scratchmem_push()
{
        c_assert(g_scratchmem.n < SRCATCHMEM_STACK_HEIGHT);
        g_scratchmem.stack[g_scratchmem.n++] = g_scratchmem.p;
}

void scratchmem_pop()
{
        c_assert(g_scratchmem.n > 0);
        g_scratchmem.p = g_scratchmem.stack[--g_scratchmem.n];
}

void scratchmem_clr()
{
        g_scratchmem.p = &g_scratchmem.byte[0];
        g_scratchmem.n = 0;
}

void *scratchmem_alloc(size_t size)
{
        size_t s  = ((size + 3u) & ~3u);
        char  *pr = &g_scratchmem.byte[SCRATCHMEM_SIZE - s];
        c_assert(g_scratchmem.p <= pr);
        void *mem = (void *)g_scratchmem.p;
        g_scratchmem.p += s;
        return mem;
}

void *scratchmem_alloc_remaining()
{
        size_t size;
        void  *mem = scratchmem_alloc_remainings(&size);
        return mem;
}

void *scratchmem_alloc_remainings(size_t *size)
{
        char *pr = &g_scratchmem.byte[SCRATCHMEM_SIZE];
        c_assert(g_scratchmem.p < pr);
        int s = pr - g_scratchmem.p;
        c_assert(s > 0);
        void *mem      = (void *)g_scratchmem.p;
        g_scratchmem.p = pr;
        *size          = s;
        return mem;
}

void *scratchmem_alloc_zero(size_t size)
{
        void *m = scratchmem_alloc(size);
        c_memclr4(m, size);
        return m;
}

void *scratchmem_alloc_zero_remaining()
{
        size_t s;
        void  *m = scratchmem_alloc_remainings(&s);
        c_memclr4(m, s);
        return m;
}

void *scratchmem_alloc_zero_remainings(size_t *size)
{
        void *m = scratchmem_alloc_remainings(size);
        c_memclr4(m, *size);
        return m;
}