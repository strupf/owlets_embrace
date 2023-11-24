// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
    TEXTBOX_STATE_INACTIVE,
    TEXTBOX_STATE_WAIT,
    TEXTBOX_STATE_WAIT_CHOICE,
    TEXTBOX_STATE_WRITE,
    TEXTBOX_STATE_FADE_IN,
    TEXTBOX_STATE_FADE_OUT,
};

enum {
    TEXTBOX_EFFECT_NONE,
    TEXTBOX_EFFECT_WAVE,
    TEXTBOX_EFFECT_SHAKE,
};

#define TEXTBOX_MAX_CHARS        256
#define TEXTBOX_NUM_LINES        4
#define TEXTBOX_NUM_BLOCKS       64
#define TEXTBOX_SPEED_DEFAULT_Q2 4
#define TEXTBOX_FADE_TICKS       15

typedef struct {
    u8 glyph;
    u8 effect;
    u8 tick_q2; // ticks per character (q2 -> 4 for 1 tick)
} textbox_char_s;

typedef struct {
    int n;
} textbox_choice_s;

typedef struct {
    int              n_chars;
    int              n_choices;
    int              line_length[TEXTBOX_NUM_LINES];
    textbox_char_s   chars[TEXTBOX_MAX_CHARS];
    textbox_choice_s choices[4];
} textbox_block_s;

typedef struct {
    int             fadetick;
    int             tick; // animation tick
    int             state;
    int             block;
    int             n;       // currently visible characters
    int             tick_q2; // tick accumulator for writing
    int             n_blocks;
    textbox_block_s blocks[TEXTBOX_NUM_BLOCKS]; // the whole dialog tree
} textbox_s;

void textbox_load_dialog(textbox_s *tb, const char *filename);
void textbox_update(textbox_s *tb);
void textbox_draw(textbox_s *tb, v2_i32 camoffset);

#endif