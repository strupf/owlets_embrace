// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

#define TEXTBOX_FADE_I            10
#define TEXTBOX_FADE_O            10
#define TEXTBOX_FNTID             FNTID_LARGE
#define TEXTBOX_NUM_FRAMES        64
#define DIALOG_NUM_LINES          4
#define DIALOG_NUM_CHARS_PER_LINE 32
#define DIALOG_NUM_CHOICES        4

typedef struct {
    u8 c;
    u8 fx;
    u8 tick_q2; // ticks per character (q2 -> 4 for 1 tick)
} dialog_char_s;

typedef struct {
    u16           len; // num of characters
    dialog_char_s chars[DIALOG_NUM_CHARS_PER_LINE];
} dialog_str_s;

typedef struct {
    u16 action;
    u16 trigger;
    u8  frame_tag;
    u8  n_chars;
    u8  chars[32];
} dialog_choice_s;

typedef struct {
    u16             tag;
    u8              n_choices;
    u8              n_lines;
    dialog_str_s    lines[DIALOG_NUM_LINES];
    dialog_choice_s choices[DIALOG_NUM_CHOICES];
} dialog_frame_s;

typedef struct {
    u16 n_frames;
} dialog_header_s;

enum {
    DIALOG_CHAR_FX_NONE,
    DIALOG_CHAR_FX_WAVE,
    DIALOG_CHAR_FX_SHAKE,
};

enum {
    DIALOG_CHOICE_NULL,
    DIALOG_CHOICE_EXIT,
    DIALOG_CHOICE_GOTO,
    DIALOG_CHOICE_OPEN_SHOP,
    DIALOG_CHOICE_TRIGGER,
};

enum {
    TEXTBOX_STATE_INACTIVE,
    TEXTBOX_STATE_WAIT,
    TEXTBOX_STATE_WRITE,
    TEXTBOX_STATE_FADE_IN,
    TEXTBOX_STATE_FADE_OUT,
};

typedef struct {
    i16             state;
    // for drawing
    i16             timer_fade;
    i16             timer;
    // for writing
    i16             tick_q2; // tick accumulator for writing
    i16             cur_frame;
    i16             cur_line;
    i16             cur_char;
    i16             cur_choice;
    bool16          fast_forward;
    dialog_frame_s *frame;
    // data
    i32             n_frames;
    dialog_frame_s  frames[TEXTBOX_NUM_FRAMES];
} textbox_s;

bool32 textbox_load_dialog(game_s *g, const char *fname);
void   textbox_update(game_s *g);
void   textbox_draw(game_s *g, v2_i32 camoffset);

#endif