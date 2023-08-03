// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

enum {
        TEXTBOX_ROWS          = 4,
        TEXTBOX_CHARS_PER_ROW = 32,
};

struct textbox_s {
        bool32    active;
        fntchar_s chars[TEXTBOX_ROWS][TEXTBOX_CHARS_PER_ROW];
        fntstr_s  str[TEXTBOX_ROWS];
        int       n_row;
        int       n_chars_shown;
        int       n_total_characters;
};

void textbox_clr(textbox_s *tb);
void textbox_set_text_ascii(textbox_s *tb, const char *txt);
void textbox_show_more(textbox_s *tb);

#endif