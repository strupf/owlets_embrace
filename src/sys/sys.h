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

// For possible porting way way in the future:
// Dynamic aspect ratio, resolution and scale to prevent black bars
//  -> Affects also game design
// Find the best fitting resolution between these X/Y values:
//
// Minimum resolution: 400 x 240 (25 x 15 tiles)
// Maximum resolution: 480 x 284 (30 x 18 tiles)

typedef struct {
    u32 *px;
    i32  w;
    i32  h;
    i32  wword;
    i32  wbyte;
} sys_display_s;

// implemented by user
void app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
void app_audio(i16 *buf, int len); // mono

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
sys_display_s sys_display_buffer();
u32           sys_tick();
void          sys_set_menu_image(void *px, int h, int wbyte);
void          sys_display_flush();
void          sys_display_update_rows(int a, int b);
void          sys_display_inv(int i);
void          sys_log(const char *str);
int           sys_inp();   // bitmask
f32           sys_crank(); // [0, 1]
int           sys_crank_docked();
int           sys_key(int k);
void          sys_set_FPS(int fps);
void          sys_menu_item_add(int ID, const char *title, void (*cb)(void *arg), void *arg);
void          sys_menu_checkmark_add(int ID, const char *title, int val, void (*cb)(void *arg), void *arg);
int           sys_menu_value(int ID);
void          sys_menu_options_add(int ID, const char *title, const char **options,
                                   int count, void (*cb)(void *arg), void *arg);
void          sys_menu_clr();
void          sys_set_volume(f32 vol); // only works in SDL

typedef void *sys_file_s;
sys_file_s   *sys_fopen(const char *path, const char *mode);
int           sys_fclose(sys_file_s *f);
size_t        sys_fread(void *buf, size_t size, size_t count, sys_file_s *f);
size_t        sys_fwrite(const void *buf, size_t size, size_t count, sys_file_s *f);
int           sys_ftell(sys_file_s *f);
int           sys_fseek(sys_file_s *f, int pos, int origin);

// copied SDL_SCANCODE_X
enum {
    SYS_KEY_A              = 4,
    SYS_KEY_B              = 5,
    SYS_KEY_C              = 6,
    SYS_KEY_D              = 7,
    SYS_KEY_E              = 8,
    SYS_KEY_F              = 9,
    SYS_KEY_G              = 10,
    SYS_KEY_H              = 11,
    SYS_KEY_I              = 12,
    SYS_KEY_J              = 13,
    SYS_KEY_K              = 14,
    SYS_KEY_L              = 15,
    SYS_KEY_M              = 16,
    SYS_KEY_N              = 17,
    SYS_KEY_O              = 18,
    SYS_KEY_P              = 19,
    SYS_KEY_Q              = 20,
    SYS_KEY_R              = 21,
    SYS_KEY_S              = 22,
    SYS_KEY_T              = 23,
    SYS_KEY_U              = 24,
    SYS_KEY_V              = 25,
    SYS_KEY_W              = 26,
    SYS_KEY_X              = 27,
    SYS_KEY_Y              = 28,
    SYS_KEY_Z              = 29,
    SYS_KEY_1              = 30,
    SYS_KEY_2              = 31,
    SYS_KEY_3              = 32,
    SYS_KEY_4              = 33,
    SYS_KEY_5              = 34,
    SYS_KEY_6              = 35,
    SYS_KEY_7              = 36,
    SYS_KEY_8              = 37,
    SYS_KEY_9              = 38,
    SYS_KEY_0              = 39,
    SYS_KEY_RETURN         = 40,
    SYS_KEY_ESCAPE         = 41,
    SYS_KEY_BACKSPACE      = 42,
    SYS_KEY_TAB            = 43,
    SYS_KEY_SPACE          = 44,
    SYS_KEY_MINUS          = 45,
    SYS_KEY_EQUALS         = 46,
    SYS_KEY_LEFTBRACKET    = 47,
    SYS_KEY_RIGHTBRACKET   = 48,
    SYS_KEY_BACKSLASH      = 49,
    SYS_KEY_NONUSHASH      = 50,
    SYS_KEY_SEMICOLON      = 51,
    SYS_KEY_APOSTROPHE     = 52,
    SYS_KEY_GRAVE          = 53,
    SYS_KEY_COMMA          = 54,
    SYS_KEY_PERIOD         = 55,
    SYS_KEY_SLASH          = 56,
    SYS_KEY_CAPSLOCK       = 57,
    SYS_KEY_F1             = 58,
    SYS_KEY_F2             = 59,
    SYS_KEY_F3             = 60,
    SYS_KEY_F4             = 61,
    SYS_KEY_F5             = 62,
    SYS_KEY_F6             = 63,
    SYS_KEY_F7             = 64,
    SYS_KEY_F8             = 65,
    SYS_KEY_F9             = 66,
    SYS_KEY_F10            = 67,
    SYS_KEY_F11            = 68,
    SYS_KEY_F12            = 69,
    SYS_KEY_RIGHT          = 79,
    SYS_KEY_LEFT           = 80,
    SYS_KEY_DOWN           = 81,
    SYS_KEY_UP             = 82,
    SYS_KEY_NUMLOCKCLEAR   = 83,
    SYS_KEY_KP_DIVIDE      = 84,
    SYS_KEY_KP_MULTIPLY    = 85,
    SYS_KEY_KP_MINUS       = 86,
    SYS_KEY_KP_PLUS        = 87,
    SYS_KEY_KP_ENTER       = 88,
    SYS_KEY_KP_1           = 89,
    SYS_KEY_KP_2           = 90,
    SYS_KEY_KP_3           = 91,
    SYS_KEY_KP_4           = 92,
    SYS_KEY_KP_5           = 93,
    SYS_KEY_KP_6           = 94,
    SYS_KEY_KP_7           = 95,
    SYS_KEY_KP_8           = 96,
    SYS_KEY_KP_9           = 97,
    SYS_KEY_KP_0           = 98,
    SYS_KEY_KP_PERIOD      = 99,
    SYS_KEY_NONUSBACKSLASH = 100,
    SYS_KEY_KP_EQUALS      = 103,
    SYS_KEY_KP_COMMA       = 133,
    SYS_KEY_KP_EQUALSAS400 = 134,
    SYS_KEY_LCTRL          = 224,
    SYS_KEY_LSHIFT         = 225,
    SYS_KEY_LALT           = 226,
    SYS_KEY_LGUI           = 227,
    SYS_KEY_RCTRL          = 228,
    SYS_KEY_RSHIFT         = 229,
    SYS_KEY_RALT           = 230,
    SYS_KEY_RGUI           = 231,
};

#endif