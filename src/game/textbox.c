// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

enum {
        TEXT_TOKEN_SPACE,
        TEXT_TOKEN_NEWLINE,
        TEXT_TOKEN_WORD,
        TEXT_TOKEN_END,
};

typedef struct {
        int i0;
        int i1;
        int type;
} text_token_s;

// simple pre-parsing of the text to be displayed
// divides text into tokens
void text_parse_tokens(const char *txt, text_token_s *toks)
{
        text_token_s *t = toks;
        for (int i = 0;; i++) {
                char c = txt[i];
                t->i0  = i;
                switch (c) {
                case '\0':
                        t->type = TEXT_TOKEN_END;
                        t->i1   = i;
                        return;
                case ' ':
                        t->type = TEXT_TOKEN_SPACE;
                        t->i1   = i;
                        break;
                case '\n':
                        t->type = TEXT_TOKEN_NEWLINE;
                        t->i1   = i;
                        break;
                default:
                        t->type = TEXT_TOKEN_WORD;
                        for (int k = i + 1;; k++) {
                                char ck = txt[k];
                                if (ck == ' ' || ck == '\n' || ck == '\0') {
                                        i     = k - 1;
                                        t->i1 = i;
                                        break;
                                }
                        }
                        break;
                }

                t++;
        }
}

void textbox_init(textbox_s *tb)
{
        textbox_clr(tb);
#if 0 // testing
        text_token_s tokens[256] = {0};
        const char  *tx          = "This is just some random\n funny text lmaaao.\nAlso new line";
        text_parse_tokens(tx, tokens);
        for (int n = 0; n < 64; n++) {
                text_token_s t = tokens[n];
                switch (t.type) {
                case TEXT_TOKEN_END:
                        PRINTF("END\n");
                        return;
                case TEXT_TOKEN_SPACE:
                        PRINTF("SPACE\n");
                        break;
                case TEXT_TOKEN_NEWLINE:
                        PRINTF("NEWLINE\n");
                        break;
                case TEXT_TOKEN_WORD:
                        for (int i = t.i0; i <= t.i1; i++) {
                                PRINTF("%c", tx[i]);
                        }
                        PRINTF("\n");
                        break;
                }
        }
#endif
}

static inline bool32 textbox_new_line(textbox_s *tb, textboxline_s **l)
{
        textboxline_s *line = *l;
        line++;
        if (line >= &tb->lines[TEXTBOX_LINES]) return 0;
        line->n = 0;
        *l      = line;
        return 1;
}

void textbox_set_text_ascii(textbox_s *tb, const char *txt)
{
        text_token_s tokens[256] = {0};
        text_parse_tokens(txt, tokens);
        textboxline_s *line = &tb->lines[0];
        for (text_token_s *t = &tokens[0];; t++) {
                switch (t->type) {
                case TEXT_TOKEN_END: goto NESTEDBREAK;
                case TEXT_TOKEN_SPACE: {
                        if (line->n != 0 && line->n < TEXTBOX_CHARS_PER_LINE) {
                                fntchar_s fc           = {0};
                                fc.glyphID             = FNT_GLYPH_SPACE;
                                line->chars[line->n++] = fc;
                        }
                } break;
                case TEXT_TOKEN_NEWLINE: {
                        if (!textbox_new_line(tb, &line))
                                goto NESTEDBREAK;
                } break;
                case TEXT_TOKEN_WORD: {
                        int len = t->i1 - t->i0;
                        if (line->n + len >= TEXTBOX_CHARS_PER_LINE) {
                                if (!textbox_new_line(tb, &line))
                                        goto NESTEDBREAK;
                        }

                        for (int i = t->i0; i <= t->i1; i++) {
                                fntchar_s fc           = {0};
                                fc.glyphID             = txt[i];
                                line->chars[line->n++] = fc;
                        }
                } break;
                }
        }
NESTEDBREAK:;
}

void textbox_clr(textbox_s *tb)
{
        tb->typewriter_tick = 0;
        tb->shows_all       = 0;
        tb->active          = 0;
        for (int n = 0; n < TEXTBOX_LINES; n++) {
                textboxline_s *l = &tb->lines[n];
                l->n             = 0;
                l->n_shown       = 0;
        }
}

void textbox_update(textbox_s *tb)
{
        if (tb->shows_all) return;
        tb->typewriter_tick++;
        if (tb->typewriter_tick < TEXTBOX_TICKS_PER_CHAR) return;
        tb->typewriter_tick -= TEXTBOX_TICKS_PER_CHAR;
        if (!textbox_show_more(tb)) {
                tb->shows_all = 1;
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