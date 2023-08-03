// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

void textbox_init(textbox_s *tb)
{
        for (int n = 0; n < TEXTBOX_ROWS; n++) {
                tb->str[n].chars = tb->chars[n];
                tb->str[n].c     = TEXTBOX_CHARS_PER_ROW;
                tb->str[n].n     = 0;
        }
}

void textbox_clr(textbox_s *tb)
{
        tb->n_total_characters = 0;
        tb->n_chars_shown      = 0;
        tb->n_row              = 0;
        for (int n = 0; n < TEXTBOX_ROWS; n++) {
                tb->str[n].n = 0;
        }
}

void textbox_set_text_ascii(textbox_s *tb, const char *txt)
{
}

void textbox_show_more(textbox_s *tb)
{
}