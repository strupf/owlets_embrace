// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
        TEXTBOX_LINES          = 4,
        TEXTBOX_CHARS_PER_LINE = 32,
        TEXTBOX_TICKS_PER_CHAR = 1,
        TEXTBOX_FILE_MEM       = 0x10000,
        TEXTBOX_NUM_TOKS       = 256,
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

struct textbox_s {
        int           page_animation_state;
        int           typewriter_tick;
        int           curreffect;
        int           currspeed;
        int           curr_line;
        int           curr_char;
        bool32        shows_all;
        bool32        active;
        textboxline_s lines[TEXTBOX_LINES];

        dialog_tok_s *tok;
        dialog_tok_s  toks[TEXTBOX_NUM_TOKS];
        char          dialogmem[TEXTBOX_FILE_MEM];
};

void   textbox_init(textbox_s *tb);
void   textbox_clr(textbox_s *tb);
void   textbox_update(textbox_s *tb);
void   textbox_load_dialog(textbox_s *tb, const char *filename);
bool32 textbox_next_page(textbox_s *tb);

#endif