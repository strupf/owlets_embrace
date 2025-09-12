// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DIA_H
#define DIA_H

#include "gamedef.h"

#define DIA_TICKS_CLOSE         10
#define DIA_TICKS_OPEN          10
#define DIA_NUM_LINES_PER_FRAME 2
#define DIA_NUM_CHARS_PER_LINE  (48 - 1) // -1 for number of characters
#define DIA_SPEED_Q4_DEFAULT    12
#define DIA_SPEAK_SND_N         3
#define DIA_H_SLIDE             52
#define DIA_NUM_CHARS_CHOICE    32

enum {
    DIA_ST_NULL,
    DIA_ST_OPENING,
    DIA_ST_CLOSING,
    DIA_ST_WRITING,
    DIA_ST_WAITING,
};

enum {
    DIA_TOK_NULL,
    DIA_TOK_CHAR,
    DIA_TOK_CHOICE,
    DIA_TOK_FX_WAVE,
    DIA_TOK_FX_WAVE_END,
    DIA_TOK_FX_SHAKE,
    DIA_TOK_FX_SHAKE_END,
    DIA_TOK_TRIGGER,
    DIA_TOK_SFX,
    DIA_TOK_SPEED,
    DIA_TOK_SPEED_RESET,
    DIA_TOK_NAME_PLAYER,
    DIA_TOK_PAUSE,
};

typedef struct {
    ALIGNAS(4)
    u16 type;
    u16 v;
} dia_tok_s;

typedef struct {
    i32       n_toks;
    dia_tok_s toks[DIA_NUM_CHARS_PER_LINE];
} dia_line_s;

typedef struct {
    ALIGNAS(2)
    u8         tag;
    u8         n_lines;
    dia_line_s lines[DIA_NUM_LINES_PER_FRAME];
} dia_frame_s;

typedef struct {
    ALIGNAS(4)
    u16 txt[DIA_NUM_CHARS_CHOICE];
} dia_choice_s;

typedef struct dia_s {
    ALIGNAS(32)
    i32          timer;
    u8           last_char_spoken;
    u8           c_speed_q4;
    u8           state;
    u8           speak_counter;
    u8           c_frame;
    u8           c_line; // current line in frame
    u8           c_char; // current char in line
    u8           c_choice;
    u8           n_frames;
    u8           n_choices;
    u8           script_input;
    dia_frame_s  frames[32];
    dia_choice_s choices[4];
} dia_s;

void dia_load_from_wad(g_s *g, void *wadname);
void dia_update(g_s *g, inp_s inp);
void dia_draw(g_s *g);
void dia_set_script_input(g_s *g, b32 script_input); // advance dialog via "dia_next_frame in code"
void dia_next_frame(g_s *g);

#endif