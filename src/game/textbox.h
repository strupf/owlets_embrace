// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
        TEXTBOX_LINES          = 4,
        TEXTBOX_CHARS_PER_LINE = 24,
        TEXTBOX_TICKS_PER_CHAR = 2,
};

enum {
        TEXTBOX_INP_NONE,
        TEXTBOX_INP_ADVANCE,
};

typedef struct {
        int       n_shown;
        int       n;
        fntchar_s chars[TEXTBOX_CHARS_PER_LINE];
} textboxline_s;

struct textbox_s {
        int           inp;
        int           inpp;
        int           typewriter_tick;
        bool32        shows_all;
        bool32        active;
        textboxline_s lines[TEXTBOX_LINES];
};

void textbox_init(textbox_s *tb);
void textbox_clr(textbox_s *tb);
void textbox_update(textbox_s *tb);
void textbox_set_text_ascii(textbox_s *tb, const char *txt);

// returns 1 if another character is shown, and 0 if the whole text
// is visible
bool32 textbox_show_more(textbox_s *tb);

#endif