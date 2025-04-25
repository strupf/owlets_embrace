// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DIALOG_H
#define DIALOG_H

#include "gamedef.h"

#define DIALOG_NUM_CHARS_PER_LINE 48
#define DIALOG_SPEED_DEFAULT      8
#define DIALOG_CHARS_PER_CHOICE   16
#define DIALOG_NUM_CHOICES        4

enum {
    DIALOG_ID1_SHOP  = 0x10,
    DIALOG_ID1_CLOSE = 0xFF,
};

enum {
    DIALOG_CFLAG_WAVE  = 1 << 6,
    DIALOG_CFLAG_SHAKE = 1 << 7
};

enum {
    DIALOG_ST_NULL,
    DIALOG_ST_OPENING,
    DIALOG_ST_CLOSING,
    DIALOG_ST_WRITING,
    DIALOG_ST_WAITING_NEXT,
    DIALOG_ST_WAITING_END,
};

typedef struct {
    ALIGNAS(4)
    u8  c;
    u8  flags_ticks; // 4 hi bits = flags, 4 lo bits = speed
    u16 trigger;
} dialog_char_s;

typedef struct {
    u8            n_chars;
    u8            n_visible;
    dialog_char_s chars[DIALOG_NUM_CHARS_PER_LINE];
} dialog_line_s;

typedef struct {
    u8 ID1;
    u8 ID2;
    u8 chars[DIALOG_CHARS_PER_CHOICE];
} dialog_choice_s;

typedef struct dialog_frame_s {
    u8            n_lines;
    u8            trigger;
    dialog_line_s lines[2];
} dialog_frame_s;

typedef struct dialog_s {
    i32 state;
    i32 tick;
    i32 tick_char;
    b8  script_input;
    u8  cur_choice;
    u8  n_choices;
    u8  n_frames;
    u8  c_frame;
    u8  c_line;

    dialog_choice_s choices[DIALOG_NUM_CHOICES];
    dialog_frame_s  frames[32];
} dialog_s;

void dialog_open_wad(g_s *g, const void *name);
void dialog_open_wad_hash(g_s *g, u32 hash);
void dialog_update(g_s *g);
void dialog_close(g_s *g);
void dialog_draw(g_s *g);
void dialog_continue_next_frame(g_s *g);

#endif