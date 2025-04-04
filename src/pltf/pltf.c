// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf.h"

#define PLTF_SHOW_FPS 0 // show FPS, UPS, logic time and draw time

typedef struct pltf_s {
    void *framebuffer;
    i32   tick;
    f32   lasttime;
    f32   ups_timeacc; // fixed timestep delta accumulator
    i32   fps_mode;
    u32   fps_mode_step; // counter for regular frame skipping
#if PLTF_SHOW_FPS
    f32 fps_timeacc; // 1-second interval accumulator for updating
    f32 ups_ft_acc;  // accumulator time spent updating
    f32 fps_ft_acc;  // accumulator time spent rendering
    u16 fps_counter; // fps counter for a 1-second interval
    u16 ups_counter; // ups counter for a 1-second interval
    u16 fps;         // fps of the last 1 second
    u16 ups;         // ups of the last 1 second
    u16 ups_ft;      // avg ms spent per update tick of the last 1 second
    u16 fps_ft;      // avg ms spent per render tick of the last 1 second
#endif
} pltf_s;

pltf_s           PLTF;
extern const u32 pltf_font[512];

i32 pltf_internal_init()
{
#if PLTF_SHOW_FPS
    PLTF.fps = PLTF_UPS;
#endif
    PLTF.lasttime    = pltf_seconds();
    PLTF.framebuffer = pltf_1bit_buffer();
    i32 res          = app_init();
    return 0;
}

// FRAMESKIP MODE ASSUMES CALLRATE OF THIS FUNCTION = 37.5 PER SECOND
// -> GAME SPEED DEPENDS ON IT
i32 pltf_internal_update()
{
    f32 time      = pltf_seconds();
    f32 timedt    = time - PLTF.lasttime;
    PLTF.lasttime = time;

    i32 n_ticks = 0; // number of logical update ticks this frame

    switch (PLTF.fps_mode) {
    case PLTF_FPS_MODE_UNCAPPED: {
        PLTF.ups_timeacc += timedt;
        if (PLTF_UPS_DT_CAP < PLTF.ups_timeacc) {
            PLTF.ups_timeacc = PLTF_UPS_DT_CAP;
        }
        while (PLTF_UPS_DT_TEST <= PLTF.ups_timeacc) {
            PLTF.ups_timeacc -= PLTF_UPS_DT;
            n_ticks++;
        }
        break;
    }
    case PLTF_FPS_MODE_40: { // 2111 | ... = 50 update ticks over 40 frames
        PLTF.fps_mode_step = (PLTF.fps_mode_step + 1) % 4;
        n_ticks            = 1 + (PLTF.fps_mode_step <= 0);
        break;
    }
    case PLTF_FPS_MODE_30: { // 221 | ... = 50 update ticks over 30 frames
        PLTF.fps_mode_step = (PLTF.fps_mode_step + 1) % 3;
        n_ticks            = 1 + (PLTF.fps_mode_step <= 1);
        break;
    }
    }

    for (i32 n = 0; n < n_ticks; n++) {
        PLTF.tick++;
        app_tick();
    }

#if PLTF_SHOW_FPS
    PLTF.ups_counter += n_ticks;
    PLTF.ups_ft_acc += pltf_seconds() - time;
#endif

    if (n_ticks) {
#if PLTF_SHOW_FPS
        f32 time_pre_draw = pltf_seconds();
        app_draw();
        PLTF.fps_ft_acc += pltf_seconds() - time_pre_draw;
        PLTF.fps_counter++;

        i32  fps_ft = 100 <= PLTF.fps_ft ? 99 : PLTF.fps_ft;
        i32  ups_ft = 100 <= PLTF.ups_ft ? 99 : PLTF.ups_ft;
        char fps[8] = {'F', ' ',
                       '0' + (PLTF.fps / 10), '0' + (PLTF.fps % 10), ' ',
                       10 <= fps_ft ? '0' + (fps_ft / 10) % 10 : ' ',
                       '0' + (fps_ft % 10)};
        char ups[8] = {'U', ' ',
                       '0' + (PLTF.ups / 10), '0' + (PLTF.ups % 10), ' ',
                       10 <= ups_ft ? '0' + (ups_ft / 10) % 10 : ' ',
                       '0' + (ups_ft % 10)};
        pltf_blit_text(fps, 0, 0);
        pltf_blit_text(ups, 0, 1);
#else
        app_draw();
#endif
    }

#ifdef PLTF_PD
    // pltf_pd_keyboard_draw();
#endif

#if PLTF_SHOW_FPS
    PLTF.fps_timeacc += timedt;
    if (1.f <= PLTF.fps_timeacc) {
        PLTF.fps_timeacc -= 1.f;
        PLTF.fps         = PLTF.fps_counter;
        PLTF.ups         = PLTF.ups_counter;
        PLTF.ups_counter = 0;
        PLTF.fps_counter = 0;
        if (0 < PLTF.ups) {
            PLTF.ups_ft = (i32)(PLTF.ups_ft_acc * 1000.5f) / PLTF.ups;
        } else {
            PLTF.ups_ft = U16_MAX;
        }
        if (0 < PLTF.fps) {
            PLTF.fps_ft = (i32)(PLTF.fps_ft_acc * 1000.5f) / PLTF.fps;
        } else {
            PLTF.fps_ft = U16_MAX;
        }
        PLTF.fps_ft_acc = 0.f;
        PLTF.ups_ft_acc = 0.f;
    }
#endif
    return n_ticks;
}

// called to reset the fixed timestep timer so that the game doesn't
// try to catch up in frames after loading something more intense and
// skips a lot of frames
void pltf_sync_timestep()
{
    PLTF.ups_timeacc = 0.f;
    PLTF.lasttime    = pltf_seconds();
}

void pltf_set_fps_mode(i32 fps_mode)
{
    PLTF.fps_mode = fps_mode;

    switch (fps_mode) {
    case PLTF_FPS_MODE_UNCAPPED: {
        pltf_internal_set_fps(0.f); // uncapped
        break;
    }
    case PLTF_FPS_MODE_40: {
        pltf_internal_set_fps(40.0f);
        break;
    }
    case PLTF_FPS_MODE_30: {
        pltf_internal_set_fps(30.0f);
        break;
    }
    }
}

void pltf_blit_text(char *str, i32 tile_x, i32 tile_y)
{
    u8 *fb = (u8 *)PLTF.framebuffer;
    i32 i  = tile_x;
    for (char *c = str; *c != '\0'; c++) {
        i32 cx = ((i32)*c & 31);
        i32 cy = ((i32)*c >> 5) << 3;
        for (i32 n = 0; n < 8; n++) {
            fb[i + ((tile_y << 3) + n) * PLTF_DISPLAY_WBYTES] =
                ((u8 *)pltf_font)[cx + ((cy + n) << 5)];
        }
        i++;
    }
}

void *pltf_file_open(const char *path, i32 pltf_file_mode)
{
    switch (pltf_file_mode) {
    case PLTF_FILE_MODE_R: return pltf_file_open_r(path);
    case PLTF_FILE_MODE_W: return pltf_file_open_w(path);
    case PLTF_FILE_MODE_A: return pltf_file_open_a(path);
    }
    return 0;
}

b32 pltf_file_w_checked(void *f, const void *buf, usize bsize)
{
    return (pltf_file_w(f, buf, bsize) == (i32)bsize);
}

b32 pltf_file_r_checked(void *f, void *buf, usize bsize)
{
    return (pltf_file_r(f, buf, bsize) == (i32)bsize);
}

i32 pltf_cur_tick()
{
    return PLTF.tick;
}

void pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
#if PLTF_SHOW_FPS
    f32 time_pre_audio = pltf_seconds();
    app_audio(lbuf, rbuf, len);
    PLTF.ups_ft_acc += pltf_seconds() - time_pre_audio;
#else
    app_audio(lbuf, rbuf, len);
#endif
}

void pltf_internal_close()
{
    app_close();
}

void pltf_internal_pause()
{
    app_pause();
}

void pltf_internal_resume()
{
    app_resume();
}

void *pltf_mem_alloc_aligned(usize s, usize alignment)
{
    assert(0 <= alignment && alignment <= 64);
    // store a 1 byte offset behind the returned pointer
    void *p             = pltf_mem_alloc(s + alignment);
    void *p_al          = align_ptr((byte *)p + 1, alignment);
    *((byte *)p_al - 1) = (byte)((byte *)p_al - (byte *)p);
    return p_al;
}

void pltf_mem_free_aligned(void *p)
{
    // offset to the original pointer is one byte behind the pointer
    void *p_og = (byte *)p - *((byte *)p - 1);
    pltf_mem_free(p_og);
}

#if PLTF_ENGINE_ONLY
i32 app_init()
{
    return 0;
}

void app_tick()
{
}

void app_draw()
{
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
}

void app_close()
{
}

void app_pause()
{
}

void app_resume()
{
}

#endif

const u32 pltf_font[512] = {
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