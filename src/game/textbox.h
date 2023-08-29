// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
        TEXTBOX_LINES                 = 4,
        TEXTBOX_CHARS_PER_LINE        = 64,
        TEXTBOX_NUM_CHOICES           = 4,
        TEXTBOX_TICKS_PER_CHAR_Q4     = 24,
        TEXTBOX_TICK_PUNCTUATION_MARK = 3, // factor!
        TEXTBOX_FILE_MEM              = 0x10000,
        TEXTBOX_NUM_TOKS              = 256,
        TEXTBOX_CLOSE_TICKS           = 10,
        TEXTBOX_CLOSE_MAXFRAME        = 3,
        TEXBOX_CLOSE_DIV              = TEXTBOX_CLOSE_TICKS / TEXTBOX_CLOSE_MAXFRAME,
};

typedef struct {
        int type;
        int i0;
        int i1;
} dialog_tok_s;

typedef struct {
        int n_shown;
        int n;

        fntchar_s chars[TEXTBOX_CHARS_PER_LINE];
        int       speed[TEXTBOX_CHARS_PER_LINE];
} textboxline_s;

typedef struct {
        fntchar_s label[16];
        int       labellen;

        // location in text
        char *txtptr;
} textboxchoice_s;

struct textbox_s {
        i32             closeticks;
        int             page_animation_state;
        int             typewriter_tick_q4;
        int             curreffect;
        int             currspeed;
        int             curr_line;
        int             curr_char;
        int             n_chars;
        int             n_chars_shown;
        bool32          shows_all;
        bool32          active;
        textboxline_s   lines[TEXTBOX_LINES];
        textboxchoice_s choices[TEXTBOX_NUM_CHOICES];
        int             n_choices;
        int             cur_choice;
        dialog_tok_s   *tok;
        dialog_tok_s    toks[TEXTBOX_NUM_TOKS];
        char            dialogmem[TEXTBOX_FILE_MEM];
};

void   textbox_select_choice(game_s *g, textbox_s *tb, int choiceID);
void   textbox_init(textbox_s *tb);
void   textbox_clr(textbox_s *tb);
void   textbox_update(textbox_s *tb);
void   textbox_load_dialog(textbox_s *tb, char *text);
bool32 textbox_next_page(textbox_s *tb);

#endif