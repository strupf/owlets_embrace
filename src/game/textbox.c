// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

void textbox_init(textbox_s *tb)
{
        textbox_clr(tb);
}

void textbox_clr(textbox_s *tb)
{
        tb->typewriter_tick = 0;
        tb->shows_all       = 0;
        for (int n = 0; n < TEXTBOX_LINES; n++) {
                textboxline_s *l = &tb->lines[n];
                l->n             = 0;
                l->n_shown       = 0;
        }
}

void textbox_set_text_ascii(textbox_s *tb, const char *txt)
{
        int l = 0;
        for (int i = 0; txt[i] != '\0'; i++) {
                char           c    = txt[i];
                textboxline_s *line = &tb->lines[l];
                if (line->n >= TEXTBOX_CHARS_PER_LINE || c == '\n') {
                        l++;
                        if (l == TEXTBOX_LINES) return;
                        line    = &tb->lines[l];
                        line->n = 0;
                        if (c == '\n') continue;
                }

                fntchar_s fc           = {0};
                fc.glyphID             = c;
                line->chars[line->n++] = fc;
        }
}

bool32 textbox_show_more(textbox_s *tb)
{
        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                if (line->n_shown < line->n) {
                        line->n_shown++;
                        return 1;
                }
        }
        return 0;
}