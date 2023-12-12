// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SYS_H
#define SYS_H

#include "sys_backend.h"
#include "sys_types.h"

#define SYS_UPS            50 // ticks per second
#define SYS_DISPLAY_W      400
#define SYS_DISPLAY_H      240
#define SYS_DISPLAY_WBYTES 52
#define SYS_DISPLAY_WWORDS 13

typedef struct {
    i16 *data;
    int  len;
    f32  pitch;
    f32  vol;
} sys_snddata_s;

typedef struct {
    u8 *px;
    int w;
    int h;
    int wword;
    int wbyte;
} sys_display_s;

typedef struct {
    i16 *buf;
    int  len;
} sys_wavdata_s;

// implemented by user
void app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();

enum {                  // pd_api.h:
    SYS_FILE_R = 1 | 2, // kFileRead | kFileReadData
    SYS_FILE_W = 4,     // kFileWrite
};

enum {
    SYS_FILE_SEEK_SET, // SEEK_SET
    SYS_FILE_SEEK_CUR, // SEEK_CUR
    SYS_FILE_SEEK_END, // SEEK_END
};

enum {
    SYS_SEEK_SET = SYS_FILE_SEEK_SET,
    SYS_SEEK_CUR = SYS_FILE_SEEK_CUR,
    SYS_SEEK_END = SYS_FILE_SEEK_END,
};

enum {                     // pd_api.h:
    SYS_INP_DPAD_L = 0x01, // kButtonLeft
    SYS_INP_DPAD_R = 0x02, // kButtonRight
    SYS_INP_DPAD_U = 0x04, // kButtonUp
    SYS_INP_DPAD_D = 0x08, // kButtonDown
    SYS_INP_B      = 0x10, // kButtonB
    SYS_INP_A      = 0x20, // kButtonA
};

#define sys_file_open   backend_file_open
#define sys_file_close  backend_file_close
#define sys_file_read   backend_file_read
#define sys_file_write  backend_file_write
#define sys_file_tell   backend_file_tell
#define sys_file_seek   backend_file_seek
#define sys_file_remove backend_file_remove
//
sys_display_s sys_display();
void          sys_set_menu_image(u8 *px, int h, int wbyte);
void          sys_display_update_rows(int a, int b);
sys_wavdata_s sys_load_wavdata(const char *filename, void *(*allocf)(usize s));
void          sys_wavdata_play(sys_wavdata_s s, f32 vol, f32 pitch);
int           sys_mus_play(const char *filename);
void          sys_mus_stop();
void          sys_set_mus_vol(int vol_q8);
int           sys_mus_vol();
bool32        sys_mus_playing();
void          sys_log(const char *str);
int           sys_inp();   // bitmask
f32           sys_crank(); // [0, 1]
int           sys_crank_docked();
f32           sys_seconds();

typedef void *sys_file_s;
sys_file_s   *sys_fopen(const char *path, const char *mode);
int           sys_fclose(sys_file_s *f);
size_t        sys_fread(void *buf, size_t size, size_t count, sys_file_s *f);
size_t        sys_fwrite(const void *buf, size_t size, size_t count, sys_file_s *f);
int           sys_ftell(sys_file_s *f);
int           sys_fseek(sys_file_s *f, int pos, int origin);

#endif