// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

#define NUM_TEXTBOX_DIALOG_FRAMES 64
#define NUM_TEXTBOX_CHOICES       4
#define NUM_TEXTBOX_FRAME_CHARS   256
#define NUM_TEXTBOX_CHOICE_CHARS  64

typedef_struct(textbox_dialog_char_s);
typedef_struct(textbox_dialog_s);
typedef_struct(textbox_dialog_frame_s);
typedef_struct(textbox_dialog_choice_s);

struct textbox_dialog_char_s {
    u8 c;
    u8 fx;
    u8 tick_q2; // ticks per character (q2 -> 4 for 1 tick)
};

struct textbox_dialog_choice_s {
    i32 trigger;
    i32 ID_frame;
    u16 ID_choice;
    u16 n_chars;
    u8  chars[NUM_TEXTBOX_CHOICE_CHARS];
};

struct textbox_dialog_frame_s {
    i32                     ID;
    u16                     n_choices;
    u16                     n_chars;
    textbox_dialog_char_s   chars[NUM_TEXTBOX_FRAME_CHARS];
    textbox_dialog_choice_s choices[NUM_TEXTBOX_CHOICES];
};

struct textbox_dialog_s {
    i32                    n_frames;
    textbox_dialog_frame_s frames[NUM_TEXTBOX_DIALOG_FRAMES];
};

enum {
    TEXTBOX_STATE_INACTIVE,
    TEXTBOX_STATE_WAIT,
    TEXTBOX_STATE_WRITE,
};

enum {
    TEXTBOX_EFFECT_NONE,
    TEXTBOX_EFFECT_WAVE,
    TEXTBOX_EFFECT_SHAKE,
};

enum {
    TEXTBOX_CHOICE_NULL,
    TEXTBOX_CHOICE_EXIT,
    TEXTBOX_CHOICE_GOTO,
    TEXTBOX_CHOICE_OPEN_SHOP,
    TEXTBOX_CHOICE_TRIGGER,
};

#define TEXTBOX_MAX_CHARS        192
#define TEXTBOX_NUM_LINES        3
#define TEXTBOX_NUM_BLOCKS       64
#define TEXTBOX_SPEED_DEFAULT_Q2 4
#define TEXTBOX_FADE_TICKS       10

typedef struct {
    u8 glyph;
    u8 effect;
    u8 tick_q2; // ticks per character (q2 -> 4 for 1 tick)
} textbox_char_s;

typedef struct {
    i32 type;
    i32 n_chars;
    i32 gototag;
    u8  chars[32];
    i32 trigger;
} textbox_choice_s;

typedef struct {
    i32              tag;
    i32              n_chars;
    i32              n_choices;
    i32              n_lines;
    i32              line_length[TEXTBOX_NUM_LINES];
    textbox_char_s   chars[TEXTBOX_MAX_CHARS];
    textbox_choice_s choices[4];
} textbox_block_s;

typedef struct {
    i32             animation;
    i32             fade_in;
    i32             fade_out;
    i32             tick; // animation tick
    i32             state;
    i32             block;
    i32             n; // currently visible characters
    i32             curchoice;
    i32             tick_q2; // tick accumulator for writing
    i32             n_blocks;
    textbox_block_s blocks[TEXTBOX_NUM_BLOCKS]; // the whole dialog tree
} textbox_s;

void textbox_load_dialog(game_s *g, textbox_s *tb, const char *filename);
void textbox_update(game_s *g);
void textbox_draw(game_s *g, v2_i32 camoffset);

#endif