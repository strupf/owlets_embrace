// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
        TEXTBOX_STATE_INACTIVE,
        TEXTBOX_STATE_OPENING,
        TEXTBOX_STATE_WRITING,
        TEXTBOX_STATE_WAITING,
        TEXTBOX_STATE_CLOSING,
};

enum {
        TEXTBOX_LINES                 = 4,
        TEXTBOX_CHARS_PER_LINE        = 64,
        TEXTBOX_NUM_CHOICES           = 4,
        TEXTBOX_TICKS_PER_CHAR_Q4     = 20,
        TEXTBOX_TICK_PUNCTUATION_MARK = 2, // factor!
        TEXTBOX_FILE_MEM              = 0x10000,
        TEXTBOX_NUM_TOKS              = 256,
        TEXTBOX_ANIMATION_TICKS       = 6,
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
        int       trigger[TEXTBOX_CHARS_PER_LINE];
} textboxline_s;

typedef struct {
        fntchar_s label[16];
        int       labellen;

        // location in text
        char *txtptr;
} textboxchoice_s;

struct textbox_s {
        int             animationticks;
        int             page_animation_state;
        int             typewriter_tick_q4;
        int             curreffect;
        int             currspeed;
        int             curr_line;
        int             curr_char;
        int             n_chars;
        int             n_chars_shown;
        bool32          shows_all;
        int             state;
        textboxline_s   lines[TEXTBOX_LINES];
        textboxchoice_s choices[TEXTBOX_NUM_CHOICES];
        int             n_choices;
        int             cur_choice;
        dialog_tok_s   *tok;
        dialog_tok_s    toks[TEXTBOX_NUM_TOKS];
        char            dialogmem[TEXTBOX_FILE_MEM];
};

int    textbox_state(textbox_s *tb);
void   textbox_init(textbox_s *tb);
void   textbox_update(game_s *g, textbox_s *tb);
bool32 textbox_blocking(textbox_s *tb);
void   textbox_select_choice(game_s *g, textbox_s *tb, int choiceID);
void   textbox_load_dialog(textbox_s *tb, char *text);
void   textbox_input(game_s *g, textbox_s *tb);

#endif