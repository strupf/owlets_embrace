// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys.h"
#include "sys_backend.h"
#include "sys_types.h"

#define SYS_SHOW_CONSOLE       0 // enable or display hardware console
#define SYS_SHOW_FPS           1 // enable fps/ups counter
//
#define SYS_UPS_DT             (1.f / (f32)SYS_UPS)
#define SYS_DT_ACCUMULATOR_CAP (SYS_UPS_DT * 5.f)
#define SYS_NUM_SNDCHANNEL     4
#define SYS_MUSCHUNK_MEM       0x4000 // 16 KB
#define SYS_MUSCHUNK_SAMPLES   (SYS_MUSCHUNK_MEM / sizeof(i16))
#define SYS_MUS_LEN_FILENAME   64

#if SYS_SHOW_CONSOLE || SYS_SHOW_FPS
static const u32 sys_consolefont[512];
#endif
#if SYS_SHOW_CONSOLE
#define SYS_CONSOLE_LINE_CHARS 50
#define SYS_CONSOLE_LINES      30
#define SYS_CONSOLE_TICKS      (SYS_UPS * 6)

static void sys_draw_console();
#endif

enum {
    SNDCHANNEL_PB_SILENT,
    SNDCHANNEL_PB_WAV,
};

typedef struct {
    char   filename[SYS_MUS_LEN_FILENAME];
    //
    void  *stream;
    int    datapos;
    int    streampos; // position in samples
    int    streamlen;
    int    chunkpos; // position in samples in chunk
    int    vol_q8;
    bool32 looping;
    //
    int    vol_q8_fade_out;
    int    fade_out_ticks_og;
    int    fade_out_ticks;
    int    fade_in_ticks;
    int    fade_in_ticks_og;
    //
    alignas(32) i16 chunk[SYS_MUSCHUNK_SAMPLES];
} sys_muschannel_s;

typedef struct {
    int playback_type;
    int vol_q8;
    f32 invpitch; // 1 / pitch

    i16 *wavedata;
    int  wavelen;
    int  wavelen_og;
    int  wavepos;
} sys_sndchannel_s;

static struct {
    f32              lasttime;
    int              fps; // updates per second
    int              ups; // frames per second
    f32              fps_timeacc;
    f32              ups_timeacc;
    int              fps_counter;
    int              ups_counter;
    int              inp;
    f32              crank;
    int              crank_docked;
    sys_muschannel_s muschannel;
    sys_sndchannel_s sndchannel[SYS_NUM_SNDCHANNEL];
#if SYS_SHOW_CONSOLE
    char console_out[SYS_CONSOLE_LINES * SYS_CONSOLE_LINE_CHARS];
    int  console_x;
    int  console_ticks;
#endif
} SYS;

void sys_init()
{
    SYS.lasttime = backend_seconds();
    SYS.fps      = SYS_UPS;
    SYS.ups      = SYS_UPS;
    app_init();
}

int sys_tick(void *arg)
{
    f32 time     = backend_seconds();
    f32 timedt   = time - SYS.lasttime;
    SYS.lasttime = time;
    SYS.fps_timeacc += timedt;
    SYS.ups_timeacc += timedt;
    if (SYS.ups_timeacc > SYS_DT_ACCUMULATOR_CAP) {
        SYS.ups_timeacc = SYS_DT_ACCUMULATOR_CAP;
    }

    int rendered = 0;
    while (SYS.ups_timeacc >= SYS_UPS_DT) {
        rendered = 1;
        SYS.ups_timeacc -= SYS_UPS_DT;
        SYS.ups_counter++;
        SYS.inp          = backend_inp();
        SYS.crank        = backend_crank();
        SYS.crank_docked = backend_crank_docked();
        app_tick();
    }

    if (rendered) {
        SYS.fps_counter++;
        app_draw();
#if SYS_SHOW_CONSOLE
        sys_draw_console();
#endif
#if SYS_SHOW_FPS
        char fps[8] = {0};
        fps[0] = '0' + (SYS.fps / 10), fps[1] = '0' + (SYS.fps % 10);
        fps[3] = '0' + (SYS.ups / 10), fps[4] = '0' + (SYS.ups % 10);

        u8 *fb = backend_framebuffer();
        for (int k = 0; k <= 4; k++) {
            if (k == 2) continue; // printable
            int c  = (int)fps[k];
            int cx = c & 31;
            int cy = c >> 5;
            for (int n = 0; n < 8; n++)
                fb[k + n * 52] = ((u8 *)sys_consolefont)[cx + (((cy << 3) + n) << 5)];
        }
#endif
    }

    if (SYS.fps_timeacc >= 1.f) {
        SYS.fps_timeacc -= 1.f;
        SYS.fps         = SYS.fps_counter;
        SYS.ups         = SYS.ups_counter;
        SYS.ups_counter = 0;
        SYS.fps_counter = 0;
    }

    return rendered;
}

void sys_close()
{
    app_close();
    sys_mus_stop();
}

void sys_pause()
{
    app_pause();
}

void sys_resume()
{
    app_resume();
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

void sys_display_update_rows(int a, int b)
{
    backend_display_row_updated(a, b);
}

int sys_inp()
{
    return backend_inp();
}

f32 sys_crank()
{
    return backend_crank();
}

int sys_crank_docked()
{
    return backend_crank_docked();
}

void sys_set_menu_image(u8 *px, int h, int wbyte)
{
    backend_set_menu_image(px, h, wbyte);
}

// http://soundfile.sapp.org/doc/WaveFormat/
typedef struct {
    u32 chunkID;
    u32 chunksize;
    u32 format;
    u32 subchunk1ID;
    u32 subchunk1size;
    u16 audioformat;
    u16 numchannels;
    u32 samplerate;
    u32 byterate;
    u16 blockalign;
    u16 bitspersample;
    u32 subchunk2ID;
    u32 subchunk2size;
} wavheader_s;

static_assert(sizeof(wavheader_s) == 44, "wav header size");

static void *wavfile_open(const char *filename, wavheader_s *wh);
static void  muschannel_update_chunk(sys_muschannel_s *ch, int samples);
static void  sndchannel_wave(sys_sndchannel_s *ch, i16 *lbuf, int len);
static void  muschannel_fillbuf(sys_muschannel_s *ch, i16 *buf, int len);
static void  muschannel_stream(sys_muschannel_s *ch, i16 *buf, int len);

int sys_audio_cb(void *context, i16 *lbuf, i16 *rbuf, int len)
{
    sys_muschannel_s *mch = &SYS.muschannel;
    muschannel_stream(mch, lbuf, len);

    for (int i = 0; i < SYS_NUM_SNDCHANNEL; i++) {
        sys_sndchannel_s *sch = &SYS.sndchannel[i];
        switch (sch->playback_type) {
        case SNDCHANNEL_PB_WAV:
            sndchannel_wave(sch, lbuf, len);
            break;
        }
    }
    return 1;
}

sys_wavdata_s sys_load_wavdata(const char *filename, void *(*allocf)(usize s))
{
    sys_wavdata_s wav = {0};
    wavheader_s   wheader;
    void         *f = wavfile_open(filename, &wheader);
    if (!f) {
        sys_printf("+++ Can't load wav: %s\n", filename);
        return wav;
    }

    wav.buf = allocf(wheader.subchunk2size);
    if (wav.buf == NULL) {
        sys_printf("+++ Can't alloc mem wav: %s\n", filename);
        sys_file_close(f);
        return wav;
    }
    wav.len = wheader.subchunk2size / sizeof(i16);
    sys_file_read(f, wav.buf, wheader.subchunk2size); // we don't check for errors here...
    sys_file_close(f);
    return wav;
}

void sys_wavdata_play(sys_wavdata_s s, f32 vol, f32 pitch)
{
    for (int i = 0; i < SYS_NUM_SNDCHANNEL; i++) {
        sys_sndchannel_s *ch = &SYS.sndchannel[i];
        if (ch->playback_type != SNDCHANNEL_PB_SILENT) continue;
        ch->playback_type = SNDCHANNEL_PB_WAV;
        ch->wavedata      = s.buf;
        ch->wavelen_og    = s.len;
        ch->wavelen       = (int)((f32)s.len * pitch);
        ch->invpitch      = 1.f / pitch;
        ch->wavepos       = 0;
        ch->vol_q8        = (int)(vol * 256.f);
        break;
    }
}

int sys_mus_play(const char *filename)
{
    sys_muschannel_s *ch = &SYS.muschannel;
    sys_mus_stop();

    wavheader_s wheader;
    void       *f = wavfile_open(filename, &wheader);
    if (!f) return 1;
    strcpy(ch->filename, filename);

    ch->stream    = f;
    ch->datapos   = backend_file_tell(f);
    ch->streamlen = (int)(wheader.subchunk2size / sizeof(i16));
    ch->streampos = 0;
    ch->vol_q8    = 256;
    ch->looping   = 1;
    muschannel_update_chunk(ch, 0);
    return 0;
}

void sys_mus_stop()
{
    sys_muschannel_s *ch = &SYS.muschannel;
    if (ch->stream) {
        backend_file_close(ch->stream);
        ch->stream = NULL;
        memset(ch->filename, 0, sizeof(ch->filename));
    }
}

void sys_set_mus_vol(int vol_q8)
{
    SYS.muschannel.vol_q8 = vol_q8;
}

int sys_mus_vol()
{
    return SYS.muschannel.vol_q8;
}

bool32 sys_mus_playing()
{
    return (SYS.muschannel.stream != NULL);
}

static void *wavfile_open(const char *filename, wavheader_s *wh)
{
    assert(filename);
    if (!filename) return NULL;
    void *f = backend_file_open(filename, SYS_FILE_R);
    if (!f) return NULL;
    backend_file_read(f, wh, sizeof(wavheader_s));
    assert(wh->bitspersample == 16);
    backend_file_seek(f, sizeof(wavheader_s), SYS_FILE_SEEK_SET);
    assert(wh->subchunk2ID == *((u32 *)"data"));
    return f;
}

static void sndchannel_wave(sys_sndchannel_s *ch, i16 *lbuf, int len)
{
    int lmax = ch->wavelen - ch->wavepos;
    int l;
    if (len < lmax) {
        l = len;
    } else { // last part of the wav file, stop after this
        l                 = lmax;
        ch->playback_type = SNDCHANNEL_PB_SILENT;
    }

    i16 *buf = lbuf;
    for (int n = 0; n < l; n++) {
        int i = (int)((f32)ch->wavepos++ * ch->invpitch);
        assert(i < ch->wavelen_og);
        int v = *buf + ((ch->wavedata[i] * ch->vol_q8) >> 8);
        if (v > I16_MAX) v = I16_MAX;
        if (v < I16_MIN) v = I16_MIN;
        *buf++ = v;
    }
}

static void muschannel_stream(sys_muschannel_s *ch, i16 *buf, int len)
{
    if (!ch->stream) {
        memset(buf, 0, sizeof(i16) * len);
        return;
    }

    int l = MIN(len, (int)(ch->streamlen - ch->streampos));
    muschannel_update_chunk(ch, len);
    muschannel_fillbuf(ch, buf, l);

    ch->streampos += l;
    if (ch->streampos >= ch->streamlen) { // at the end of the song
        int samples_left = len - l;
        if (ch->looping) { // fill remainder of buffer and restart
            ch->streampos = samples_left;
            muschannel_update_chunk(ch, 0);
            muschannel_fillbuf(ch, &buf[l], samples_left);
        } else {
            memset(&buf[l], 0, samples_left * sizeof(i16));
            sys_mus_stop();
        }
    }
}

// update loaded music chunk if we are running out of samples
static void muschannel_update_chunk(sys_muschannel_s *ch, int samples)
{
    int samples_chunked = SYS_MUSCHUNK_SAMPLES - ch->chunkpos;
    if (0 < samples && samples <= samples_chunked) return;

    // refill music buffer from file
    // place chunk beginning right at streampos
    backend_file_seek(ch->stream,
                      ch->datapos + ch->streampos * sizeof(i16),
                      SYS_FILE_SEEK_SET);
    int samples_left    = ch->streamlen - ch->streampos;
    int samples_to_read = MIN(SYS_MUSCHUNK_SAMPLES, samples_left);

    backend_file_read(ch->stream, ch->chunk, sizeof(i16) * samples_to_read);
    ch->chunkpos = 0;
}

static void muschannel_fillbuf(sys_muschannel_s *ch, i16 *buf, int len)
{
    i16 *b = buf;
    i16 *c = &ch->chunk[ch->chunkpos];

    ch->chunkpos += len;
    if (ch->vol_q8 == 256) {
        memcpy(b, c, sizeof(i16) * len);
    } else {
        for (int n = 0; n < len; n++) {
            *b++ = (*c++ * ch->vol_q8) >> 8;
        }
    }
}

f32 sys_seconds()
{
    return backend_seconds();
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

    u8 *fb = backend_framebuffer();
    for (int y = 10; y < SYS_CONSOLE_LINES; y++) {
        int yy = SYS_CONSOLE_LINES - y - 1;
        for (int x = 0; x < SYS_CONSOLE_LINE_CHARS; x++) {
            int k = x + yy * SYS_CONSOLE_LINE_CHARS;
            int c = (int)SYS.console_out[k];
            if (!(32 <= c && c < 127)) continue; // printable
            int cx = c % 32;
            int cy = c / 32;
            for (int n = 0; n < 8; n++) {
                fb[x + (y * 8 + n) * 52] =
                    ((u8 *)sys_consolefont)[cx + (cy * 8 + n) * 32];
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