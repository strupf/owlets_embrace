// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys.h"
#include "sys_backend.h"
#include "sys_types.h"

#define SYS_SHOW_CONSOLE 0 // enable or display hardware console
#define SYS_SHOW_FPS     0 // enable fps/ups counter
//
#define SYS_UPS_DT       0.0200f // 1 /  50
#define SYS_UPS_DT_TEST  0.0195f // 1 / ~51
#define SYS_UPS_DT_CAP   0.0600f // 3 /  50

#if SYS_SHOW_CONSOLE || SYS_SHOW_FPS
static const u32 sys_consolefont[512];
#endif
#if SYS_SHOW_CONSOLE
#define SYS_CONSOLE_LINE_CHARS 50
#define SYS_CONSOLE_LINES      40
#define SYS_CONSOLE_TICKS      (SYS_UPS * 6)

static void sys_draw_console();
#endif

static struct {
    void *menu_items[8];
    int   inp;
    f32   crank;
    int   crank_docked;
    u32   tick;
    f32   lasttime;
    f32   ups_timeacc;
#if SYS_SHOW_FPS
    f32 fps_timeacc;
    int fps_counter;
    int fps; // updates per second
#endif
#if SYS_SHOW_CONSOLE
    char console_out[SYS_CONSOLE_LINES * SYS_CONSOLE_LINE_CHARS];
    int  console_x;
    int  console_ticks;
#endif
} SYS;

void sys_init()
{
#if SYS_SHOW_FPS
    SYS.fps = SYS_UPS;
#endif
    SYS.lasttime = backend_seconds();
    app_init();
}

static inline void sys_tick_()
{
    SYS.inp          = backend_inp();
    SYS.crank        = backend_crank();
    SYS.crank_docked = backend_crank_docked();
    SYS.tick++;
    app_tick();
}

static inline void sys_draw_()
{
    app_draw();
#if SYS_SHOW_CONSOLE
    sys_draw_console();
#endif
#if SYS_SHOW_FPS
    char fps[2] = {'0' + (SYS.fps / 10), '0' + (SYS.fps % 10)};
    u8  *fb     = (u8 *)backend_framebuffer();
    for (int k = 0; k <= 1; k++) {
        int cx = ((int)fps[k] & 31);
        int cy = ((int)fps[k] >> 5) << 3;
        for (int n = 0; n < 8; n++) {
            fb[k + n * SYS_DISPLAY_WBYTES] = ((u8 *)sys_consolefont)[cx + ((cy + n) << 5)];
        }
    }
    SYS.fps_counter++;
#endif
}

// there are some frame skips when using the exact delta time and evaluating
// if an update tick should run (@50 FPS cap on hardware)
//
// https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
int sys_step(void *arg)
{
    f32 time     = backend_seconds();
    f32 timedt   = time - SYS.lasttime;
    SYS.lasttime = time;
    SYS.ups_timeacc += timedt;
    if (SYS_UPS_DT_CAP < SYS.ups_timeacc) {
        SYS.ups_timeacc = SYS_UPS_DT_CAP;
    }

    int n_upd = 0;
    while (SYS_UPS_DT_TEST <= SYS.ups_timeacc) {
        SYS.ups_timeacc -= SYS_UPS_DT;
        n_upd++;
#if defined(SYS_SDL) && defined(SYS_DEBUG)
        static int slowtick;
        if (sys_key(SYS_KEY_LSHIFT) && (slowtick++ & 31))
            continue;
#endif
        sys_tick_();
    }

    if (n_upd) {
        sys_draw_();
    }

#if SYS_SHOW_FPS
    SYS.fps_timeacc += timedt;
    if (1.f <= SYS.fps_timeacc) {
        SYS.fps_timeacc -= 1.f;
        SYS.fps         = SYS.fps_counter;
        SYS.fps_counter = 0;
    }
#endif
    return (n_upd);
}

void sys_close()
{
    app_close();
}

void sys_pause()
{
    app_pause();
}

void sys_resume()
{
    app_resume();
}

int sys_audio(void *context, i16 *lbuf, i16 *rbuf, int len)
{
    app_audio(lbuf, len);
    return 1;
}

void sys_log(const char *str)
{
#if SYS_SHOW_CONSOLE
    SYS.console_ticks = SYS_CONSOLE_TICKS;
    for (const char *c = str; *c != '\0'; c++) {
        SYS.console_out[SYS.console_x++] = *c;
        if (*c == '\n' ||
            SYS.console_x >= SYS_CONSOLE_LINE_CHARS) {
            memmove(&SYS.console_out[SYS_CONSOLE_LINE_CHARS],
                    &SYS.console_out[0],
                    SYS_CONSOLE_LINE_CHARS * (SYS_CONSOLE_LINES - 1));
            memset(&SYS.console_out[0],
                   0,
                   SYS_CONSOLE_LINE_CHARS);
            SYS.console_x = *c == '\n' ? 2 : 0;
        }
    }
#endif
}

sys_display_s sys_display()
{
    sys_display_s s;
    s.px    = backend_framebuffer();
    s.w     = SYS_DISPLAY_W;
    s.h     = SYS_DISPLAY_H;
    s.wbyte = SYS_DISPLAY_WBYTES;
    s.wword = SYS_DISPLAY_WWORDS;
    return s;
}

u32 sys_tick()
{
    return SYS.tick;
}

void sys_display_update_rows(int a, int b)
{
    assert(0 <= a && b < SYS_DISPLAY_H);
    backend_display_row_updated(a, b);
}

int sys_inp()
{
    return backend_inp();
}

int sys_key(int k)
{
    return backend_key(k);
}

f32 sys_crank()
{
    return backend_crank();
}

int sys_crank_docked()
{
    return backend_crank_docked();
}

void sys_set_menu_image(void *px, int h, int wbyte)
{
    backend_set_menu_image(px, h, wbyte);
}

void sys_set_FPS(int fps)
{
    backend_set_FPS(fps);
}

void sys_menu_item_add(int ID, const char *title, void (*cb)(void *arg), void *arg)
{
    void *mi           = backend_menu_item_add(title, cb, arg);
    SYS.menu_items[ID] = mi;
}

void sys_menu_checkmark_add(int ID, const char *title, int val, void (*cb)(void *arg), void *arg)
{
    void *mi           = backend_menu_checkmark_add(title, val, cb, arg);
    SYS.menu_items[ID] = mi;
}

bool32 sys_menu_checkmark(int ID)
{
    return backend_menu_checkmark(SYS.menu_items[ID]);
}

void sys_menu_clr()
{
    backend_menu_clr();
    memset(SYS.menu_items, 0, sizeof(SYS.menu_items));
}

sys_file_s *sys_fopen(const char *path, const char *mode)
{
    switch (mode[0]) {
    case 'r': return (sys_file_s *)backend_file_open(path, SYS_FILE_R);
    case 'w': return (sys_file_s *)backend_file_open(path, SYS_FILE_W);
    }
    return NULL;
}

int sys_fclose(sys_file_s *f)
{
    return backend_file_close(f);
}

size_t sys_fread(void *buf, size_t size, size_t count, sys_file_s *f)
{
    int i = backend_file_read(f, buf, size * count);
    return (i * count);
}

size_t sys_fwrite(const void *buf, size_t size, size_t count, sys_file_s *f)
{
    int i = backend_file_write(f, buf, size * count);
    return (i * count);
}

int sys_ftell(sys_file_s *f)
{
    return backend_file_tell(f);
}

int sys_fseek(sys_file_s *f, int pos, int origin)
{
    return backend_file_seek(f, pos, origin);
}

void sys_set_volume(f32 vol)
{
    backend_set_volume(vol);
}

void sys_display_inv(int i)
{
    backend_display_inv(i);
}

#if SYS_CONFIG_ONLY_BACKEND
void app_init()
{}
void app_tick()
{}
void app_draw()
{}
void app_close()
{}
void app_resume()
{}
void app_pause()
{}
void app_audio(i16 *buf, int len)
{}
#endif

#if SYS_SHOW_CONSOLE

static void sys_log_push_line()
{
    memmove(&SYS.console_out[SYS_CONSOLE_LINE_CHARS],
            &SYS.console_out[0],
            SYS_CONSOLE_LINE_CHARS * (SYS_CONSOLE_LINES - 1));
    memset(&SYS.console_out[0],
           0,
           SYS_CONSOLE_LINE_CHARS);
}

static void sys_log_clr()
{
    memset(SYS.console_out, 0, sizeof(SYS.console_out));
}

static void sys_draw_console()
{
    if (SYS.console_ticks <= 0) return;
    SYS.console_ticks--;

    u8 *fb = (u8 *)backend_framebuffer();
    for (int y = 10; y < SYS_CONSOLE_LINES; y++) {
        int yy = SYS_CONSOLE_LINES - y - 1;
        for (int x = 0; x < SYS_CONSOLE_LINE_CHARS; x++) {
            int k = x + yy * SYS_CONSOLE_LINE_CHARS;
            int c = (int)SYS.console_out[k];
            if (!(32 <= c && c < 127)) continue; // printable
            int cx = c & 31;
            int cy = c >> 5;
            for (int n = 0; n < 8; n++) {
                fb[x + ((y << 3) + n) * 52] =
                    ((u8 *)sys_consolefont)[cx + ((cy * 8 + n) << 5)];
            }
        }
    }
    sys_display_update_rows(0, 239);
}

#endif
#if SYS_SHOW_CONSOLE || SYS_SHOW_FPS
// bitmap data of IBM EGA 8x8
static const u32 sys_consolefont[512] = {
    0x6C7E7E00U, 0x00103810U, 0x0FFF00FFU, 0x997F3F3CU, 0x66180280U, 0x18003E7FU, 0x00001818U, 0x00000000U,
    0xFEFF8100U, 0x00107C38U, 0x07C33CFFU, 0x5A633366U, 0x663C0EE0U, 0x3C0063DBU, 0x3018183CU, 0xFF182400U,
    0xFEDBA500U, 0x1838387CU, 0x0F9966E7U, 0x3C7F3F66U, 0x667E3EF8U, 0x7E0038DBU, 0x600C187EU, 0xFF3C66C0U,
    0xFEFF8100U, 0x3C7CFEFEU, 0x7DBD42C3U, 0xE7633066U, 0x6618FEFEU, 0x18006C7BU, 0xFEFE1818U, 0x7E7EFFC0U,
    0x7CC3BD00U, 0x3CFEFE7CU, 0xCCBD42C3U, 0xE763303CU, 0x66183EF8U, 0x7E7E6C1BU, 0x600C7E18U, 0x3CFF66C0U,
    0x38E79900U, 0x187C7C38U, 0xCC9966E7U, 0x3C677018U, 0x007E0EE0U, 0x3C7E381BU, 0x30183C18U, 0x18FF24FEU,
    0x10FF8100U, 0x00383810U, 0xCCC33CFFU, 0x5AE6F07EU, 0x663C0280U, 0x187ECC1BU, 0x00001818U, 0x00000000U,
    0x007E7E00U, 0x007C7C08U, 0x78FF00FFU, 0x99C0E018U, 0x00180000U, 0xFF007800U, 0x00000000U, 0x00000000U,
    0x6C6C3000U, 0x60380030U, 0x00006018U, 0x06000000U, 0x7878307CU, 0xFC38FC1CU, 0x00007878U, 0x78600018U,
    0x6C6C7800U, 0x606CC67CU, 0x30663030U, 0x0C000000U, 0xCCCC70C6U, 0xCC60C03CU, 0x3030CCCCU, 0xCC300030U,
    0xFE6C7800U, 0xC038CCC0U, 0x303C1860U, 0x18000000U, 0x0C0C30CEU, 0x0CC0F86CU, 0x3030CCCCU, 0x0C18FC60U,
    0x6C003000U, 0x00761878U, 0xFCFF1860U, 0x3000FC00U, 0x383830DEU, 0x18F80CCCU, 0x00007C78U, 0x180C00C0U,
    0xFE003000U, 0x00DC300CU, 0x303C1860U, 0x60000000U, 0x0C6030F6U, 0x30CC0CFEU, 0x00000CCCU, 0x30180060U,
    0x6C000000U, 0x00CC66F8U, 0x30663030U, 0xC0300030U, 0xCCCC30E6U, 0x30CCCC0CU, 0x303018CCU, 0x0030FC30U,
    0x6C003000U, 0x0076C630U, 0x00006018U, 0x80300030U, 0x78FCFC7CU, 0x3078781EU, 0x30307078U, 0x30600018U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000060U, 0x00000000U, 0x00000000U, 0x60000000U, 0x00000000U,
    0x3CFC307CU, 0x3CFEFEF8U, 0xE61E78CCU, 0x38C6C6F0U, 0x78FC78FCU, 0xC6CCCCFCU, 0x78FECCC6U, 0x001078C0U,
    0x666678C6U, 0x6662626CU, 0x660C30CCU, 0x6CE6EE60U, 0xCC66CC66U, 0xC6CCCCB4U, 0x60C6CCC6U, 0x00381860U,
    0xC066CCDEU, 0xC0686866U, 0x6C0C30CCU, 0xC6F6FE60U, 0xE066CC66U, 0xC6CCCC30U, 0x608CCC6CU, 0x006C1830U,
    0xC07CCCDEU, 0xC0787866U, 0x780C30FCU, 0xC6DEFE60U, 0x707CCC7CU, 0xD6CCCC30U, 0x60187838U, 0x00C61818U,
    0xC066FCDEU, 0xCE686866U, 0x6CCC30CCU, 0xC6CED662U, 0x1C6CDC60U, 0xFECCCC30U, 0x60323038U, 0x0000180CU,
    0x6666CCC0U, 0x6660626CU, 0x66CC30CCU, 0x6CC6C666U, 0xCC667860U, 0xEE78CC30U, 0x6066306CU, 0x00001806U,
    0x3CFCCC78U, 0x3EF0FEF8U, 0xE67878CCU, 0x38C6C6FEU, 0x78E61CF0U, 0xC630FC78U, 0x78FE78C6U, 0x00007802U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0xFF000000U,
    0x00E00030U, 0x0038001CU, 0xE00C30E0U, 0x00000070U, 0x00000000U, 0x00000010U, 0x1C000000U, 0x0076E018U,
    0x00600030U, 0x006C000CU, 0x60000060U, 0x00000030U, 0x00000000U, 0x00000030U, 0x30000000U, 0x10DC3018U,
    0x78607818U, 0x7660780CU, 0x660C706CU, 0x78F8CC30U, 0x7CDC76DCU, 0xC6CCCC7CU, 0x30FCCCC6U, 0x38003018U,
    0xCC7C0C00U, 0xCCF0CC7CU, 0x6C0C3076U, 0xCCCCFE30U, 0xC076CC66U, 0xD6CCCC30U, 0xE098CC6CU, 0x6C001C00U,
    0xC0667C00U, 0xCC60FCCCU, 0x780C3066U, 0xCCCCFE30U, 0x7866CC66U, 0xFECCCC30U, 0x3030CC38U, 0xC6003018U,
    0xCC66CC00U, 0x7C60C0CCU, 0x6CCC3066U, 0xCCCCD630U, 0x0C607C7CU, 0xFE78CC34U, 0x30647C6CU, 0xC6003018U,
    0x78DC7600U, 0x0CF07876U, 0xE6CC78E6U, 0x78CCC678U, 0xF8F00C60U, 0x6C307618U, 0x1CFC0CC6U, 0xFE00E018U,
    0x00000000U, 0xF8000000U, 0x00780000U, 0x00000000U, 0x00001EF0U, 0x00000000U, 0x0000F800U, 0x00000000U,
    0x7E1C0078U, 0x0030E0CCU, 0xCCE0CC7EU, 0x30C6E07CU, 0x783E001CU, 0x00780000U, 0x18CCC300U, 0x0EF8CC38U,
    0xC300CCCCU, 0x00300000U, 0x000000C3U, 0x303800C6U, 0xCC6C0000U, 0xE0CCE0CCU, 0x180018CCU, 0x1BCCCC6CU,
    0x3C7800C0U, 0x78787878U, 0x7078783CU, 0x006C7038U, 0x00CC7FFCU, 0x00000000U, 0x7ECC3C00U, 0x18CC7864U,
    0x06CCCCCCU, 0xC00C0C0CU, 0x30CCCC66U, 0x78C63018U, 0x78FE0C60U, 0xCCCC7878U, 0xC0CC66CCU, 0x3CFAFCF0U,
    0x3EFCCC78U, 0xC07C7C7CU, 0x30FCFC7EU, 0xCCFE3018U, 0xCCCC7F78U, 0xCCCCCCCCU, 0xC0CC66CCU, 0x18C63060U,
    0x66C0CC18U, 0x78CCCCCCU, 0x30C0C060U, 0xFCC63018U, 0xCCCCCC60U, 0xCCCCCCCCU, 0x7ECC3C7CU, 0x18CFFCE6U,
    0x3F787E0CU, 0x0C7E7E7EU, 0x7878783CU, 0xCCC6783CU, 0x78CE7FFCU, 0x7E7E7878U, 0x1878180CU, 0xD8C630FCU,
    0x00000078U, 0x38000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x180000F8U, 0x70C73000U,
    0x0000381CU, 0x383CFC00U, 0xC3000030U, 0x000018C3U, 0x18DB5522U, 0x00361818U, 0x00363600U, 0x00183636U,
    0x1C1C0000U, 0x6C6C00F8U, 0xC6000000U, 0xCC3318C6U, 0x1877AA88U, 0x00361818U, 0x00363600U, 0x00183636U,
    0x00007078U, 0x6C6CCC00U, 0xCC000030U, 0x666600CCU, 0x18DB5522U, 0x0036F818U, 0xFE36F6F8U, 0x00F836F6U,
    0xCC78300CU, 0x383EECF8U, 0xDEFCFC60U, 0x33CC18DBU, 0x18EEAA88U, 0x00361818U, 0x06360618U, 0x00183606U,
    0xCCCC307CU, 0x0000FCCCU, 0x330CC0C0U, 0x66661837U, 0x18DB5522U, 0xFEF6F8F8U, 0xF636F6F8U, 0xF8F8FEFEU,
    0xCCCC30CCU, 0x7C7EDCCCU, 0x660CC0CCU, 0xCC33186FU, 0x1877AA88U, 0x36361818U, 0x36363618U, 0x18000000U,
    0x7E78787EU, 0x0000CCCCU, 0xCC000078U, 0x000018CFU, 0x18DB5522U, 0x36361818U, 0x36363618U, 0x18000000U,
    0x00000000U, 0x00000000U, 0x0F000000U, 0x00000003U, 0x18EEAA88U, 0x36361818U, 0x36363618U, 0x18000000U,
    0x18001818U, 0x36181800U, 0x00360036U, 0x18360036U, 0x36000036U, 0x36000018U, 0xFF001818U, 0xFF0FF000U,
    0x18001818U, 0x36181800U, 0x00360036U, 0x18360036U, 0x36000036U, 0x36000018U, 0xFF001818U, 0xFF0FF000U,
    0x18001818U, 0x361F1800U, 0xFFF73F37U, 0xFFF7FF37U, 0x3600FF36U, 0x36001F1FU, 0xFF0018FFU, 0xFF0FF000U,
    0x18001818U, 0x36181800U, 0x00003030U, 0x00000030U, 0x36000036U, 0x36001818U, 0xFF001818U, 0xFF0FF000U,
    0x1FFFFF1FU, 0x371FFFFFU, 0xF7FF373FU, 0xFFF7FF37U, 0x3FFFFFFFU, 0xFF3F1F1FU, 0xFF1FF8FFU, 0x000FF0FFU,
    0x18180000U, 0x36181800U, 0x36003600U, 0x00360036U, 0x00361800U, 0x36361800U, 0xFF180018U, 0x000FF0FFU,
    0x18180000U, 0x36181800U, 0x36003600U, 0x00360036U, 0x00361800U, 0x36361800U, 0xFF180018U, 0x000FF0FFU,
    0x18180000U, 0x36181800U, 0x36003600U, 0x00360036U, 0x00361800U, 0x36361800U, 0xFF180018U, 0x000FF0FFU,
    0x00000000U, 0x000000FCU, 0x1C3838FCU, 0x78380600U, 0x18603000U, 0x0030180EU, 0x0F000038U, 0x00007078U,
    0xFEFC7800U, 0x766600CCU, 0x306C6C30U, 0xCC600C00U, 0x303030FCU, 0x7630181BU, 0x0C00006CU, 0x0000186CU,
    0x6CCCCC76U, 0xDC667E60U, 0x18C6C678U, 0xCCC07E7EU, 0x6018FC00U, 0xDC00181BU, 0x0C00006CU, 0x003C306CU,
    0x6CC0F8DCU, 0x1866D830U, 0x7CC6FECCU, 0xCCF8DBDBU, 0x303030FCU, 0x00FC1818U, 0x0C001838U, 0x003C606CU,
    0x6CC0CCC8U, 0x1866D860U, 0xCC6CC6CCU, 0xCCC0DBDBU, 0x18603000U, 0x76001818U, 0xEC181800U, 0x003C786CU,
    0x6CC0F8DCU, 0x187CD8CCU, 0xCC6C6C78U, 0xCC607E7EU, 0x000000FCU, 0xDC30D818U, 0x6C000000U, 0x003C0000U,
    0x6CC0C076U, 0x186070FCU, 0x78EE3830U, 0xCC386000U, 0xFCFCFC00U, 0x0030D818U, 0x3C000000U, 0x00000000U,
    0x0000C000U, 0x00C00000U, 0x000000FCU, 0x0000C000U, 0x00000000U, 0x00007018U, 0x1C000000U, 0x00000000U};
#endif