// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

enum {
        DIALOG_TOK_NULL,
        DIALOG_TOK_PORTRAIT,
        DIALOG_TOK_CMD,
        DIALOG_TOK_TEXT,
        DIALOG_TOK_TEXT_NEW_LINE,
        DIALOG_TOK_TEXT_NEW_PAGE,
        DIALOG_TOK_END,
};

static bool32 str_matches(const char *str, const char *exp)
{
        for (int n = 0; exp[n] != '\0'; n++) {
                if (str[n] != exp[n]) return 0;
        }
        return 1;
}

void dialog_parse(const char *txt, dialog_tok_s *toks)
{
        int ntok = 0;
        for (int i = 0; txt[i] != '\0'; i++) {
                switch (txt[i]) {
                case '\n': // skippable white space
                case '\t':
                case '\r':
                case '\f':
                case '\v': break;
                case '[': {
                        dialog_tok_s *tportrait = &toks[ntok++];
                        tportrait->type         = DIALOG_TOK_PORTRAIT;
                        tportrait->i0           = i;
                        do {
                                i++;
                        } while (txt[i] != ']');
                        tportrait->i1 = i;
                } break;
                case '{': {
                        dialog_tok_s *tcmd = &toks[ntok++];
                        tcmd->type         = DIALOG_TOK_CMD;
                        tcmd->i0           = i;
                        const char *str    = &txt[i + 1];
                        do {
                                i++;
                        } while (txt[i] != '}');
                        tcmd->i1 = i;
                } break;
                default: {
                        dialog_tok_s *tb = &toks[ntok++];
                        tb->type         = DIALOG_TOK_TEXT;
                        tb->i0           = i;
                        while (1) {
                                i++;
                                char cc = txt[i];
                                if (cc == '{') {
                                        tb->i1 = i - 1;
                                        i--;
                                        break;
                                }

                                if (cc == '\0' || (cc == '\n' &&
                                                   (txt[i + 1] == '\0' ||
                                                    txt[i + 1] == '\n'))) {
                                        tb->i1           = i - 1;
                                        dialog_tok_s *tp = &toks[ntok++];
                                        tp->type         = DIALOG_TOK_TEXT_NEW_PAGE;
                                        break;
                                }
                        }
                } break;
                }
        }
        dialog_tok_s *tend = &toks[ntok++];
        tend->type         = DIALOG_TOK_END;
}

void textbox_init(textbox_s *tb)
{
        textbox_clr(tb);
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

void textbox_load_dialog(textbox_s *tb, const char *filename)
{
        txt_read_file(filename, tb->dialogmem, TEXTBOX_FILE_MEM);
        dialog_parse(tb->dialogmem, tb->toks);
        tb->tok    = tb->toks;
        tb->active = 1;
        textbox_next_page(tb);
}

bool32 textbox_next_page(textbox_s *tb)
{
        tb->currspeed = TEXTBOX_TICKS_PER_CHAR;
        textbox_clr(tb);
        textboxline_s *line = &tb->lines[0];
        while (tb->tok->type != DIALOG_TOK_END) {
                const char *s = &tb->dialogmem[tb->tok->i0 + 1];
                switch (tb->tok->type) {
                case DIALOG_TOK_CMD: {
                        if (0) {
                        } else if (str_matches(s, "~")) {
                                tb->curreffect = FNT_EFFECT_WAVE;
                        } else if (str_matches(s, "/~")) {
                                tb->curreffect = FNT_EFFECT_NONE;
                        } else if (str_matches(s, "n")) {
                                if (!textbox_new_line(tb, &line)) return 0;
                        } else if (str_matches(s, ">>")) {
                                if (tb->tok->i1 - tb->tok->i0 > 3) {
                                        tb->currspeed = os_i32_from_str(&s[3]);
                                } else {
                                        tb->currspeed = TEXTBOX_TICKS_PER_CHAR;
                                }
                        } else if (str_matches(s, "trigger")) {
                        } else if (str_matches(s, "*")) {
                                tb->curreffect = FNT_EFFECT_SHAKE;
                        } else if (str_matches(s, "/*")) {
                                tb->curreffect = FNT_EFFECT_NONE;
                        }
                } break;
                case DIALOG_TOK_PORTRAIT: {

                } break;
                case DIALOG_TOK_TEXT: {
                        for (int i = tb->tok->i0; i <= tb->tok->i1; i++) {
                                char ci = tb->dialogmem[i];
                                if (ci == '\n') {
                                        if (!textbox_new_line(tb, &line))
                                                return 0;
                                        continue;
                                }
                                fntchar_s fc = {0};
                                fc.glyphID   = ci;
                                fc.effectID  = tb->curreffect;
                                ASSERT(line->n < TEXTBOX_CHARS_PER_LINE);
                                line->speed[line->n] = tb->currspeed;
                                line->chars[line->n] = fc;
                                line->n++;
                        }
                } break;
                case DIALOG_TOK_TEXT_NEW_PAGE: {
                        tb->tok++;
                        return 1;
                } break;
                }
                tb->tok++;
        }
        tb->active = 0;
        return 0;
}

void textbox_clr(textbox_s *tb)
{
        tb->typewriter_tick = 0;
        tb->shows_all       = 0;
        tb->curr_char       = 0;
        tb->curr_line       = 0;
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
        textboxline_s *line = &tb->lines[tb->curr_line];
        if (tb->typewriter_tick < line->speed[tb->curr_char]) return;
        tb->typewriter_tick = 0;

        if (tb->curr_char < line->n) {
                tb->curr_char++;
                line->n_shown++;
        } else {
                line = &tb->lines[++tb->curr_line];
                if (line >= &tb->lines[TEXTBOX_LINES]) {
                        tb->shows_all            = 1;
                        tb->page_animation_state = 0;
                        return;
                }
                tb->curr_char = 0;
                if (tb->curr_char < line->n) {
                        tb->curr_char++;
                        line->n_shown++;
                }
        }
}