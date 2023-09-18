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

int textbox_state(textbox_s *tb)
{
        return tb->state;
}

bool32 textbox_blocking(textbox_s *tb)
{
        return (tb->state == TEXTBOX_STATE_WRITING ||
                tb->state == TEXTBOX_STATE_WAITING);
}

// returns true if str contains the same characters of exp, not
// comparing the terminating \0 of exp
static bool32 str_matches(const char *str, const char *exp)
{
        for (const char *s = str, *e = exp; *e != '\0'; e++, s++) {
                if (*s != *e) return 0;
        }
        return 1;
}

static void dialog_parse(const char *txt, dialog_tok_s *toks)
{
        int ntok = 0;
        for (int i = 0; txt[i] != '\0'; i++) {
                ASSERT(ntok + 1 < TEXTBOX_NUM_TOKS);
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
                        tcmd->i0           = i + 1;
                        const char *str    = &txt[i + 1];
                        do {
                                i++;
                        } while (txt[i] != '}');
                        tcmd->i1 = i - 1;
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

                                // newlines Raylib \n, Playdate sim \r\n
                                if (cc == '\0' ||
#ifdef TARGET_DESKTOP
                                    ((cc == '\n') &&
                                     (txt[i + 1] == '\0' || txt[i + 1] == '\n')
#else
                                    ((cc == '\r' && txt[i + 1] == '\n') &&
                                     (txt[i + 2] == '\0' || txt[i + 2] == '\r')
#endif
                                         )) {
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

static void textbox_clr(textbox_s *tb)
{
        tb->animationticks     = 0;
        tb->typewriter_tick_q4 = 0;
        tb->curr_char          = 0;
        tb->curr_line          = 0;
        tb->n_choices          = 0;
        tb->magic              = 0xDEADBEEF;
        for (int n = 0; n < TEXTBOX_LINES; n++) {
                textboxline_s *l = &tb->lines[n];
                l->n             = 0;
                l->n_shown       = 0;
                os_memclr(l->trigger, sizeof(l->trigger));
                for (int n = 0; n < TEXTBOX_CHARS_PER_LINE; n++) {
                        l->speed[n] = TEXTBOX_TICKS_PER_CHAR_Q4;
                }
        }
}

static inline bool32 textbox_new_line(textbox_s *tb, textboxline_s **l)
{
        textboxline_s *line = *l;
        if (++line >= &tb->lines[TEXTBOX_LINES]) return 0;
        line->n = 0;
        *l      = line;
        return 1;
}

static void textbox_cmd_choice(textbox_s *tb, dialog_tok_s *tok)
{
        char *txt = tb->dialogmem;
        for (int k = 0; k < TEXTBOX_NUM_CHOICES; k++) {
                tb->choices[k].labellen = 0;
        }

        // parse choice actions
        for (int i = tok->i0; i <= tok->i1; i++) {
                char c = txt[i];
                if (c != ' ') continue;
                textboxchoice_s *tc = &tb->choices[tb->n_choices++];

                for (i++; txt[i] != '['; i++) {
                        char ck = txt[i];
                        PRINTF("%c\n", ck);
                        fntchar_s fc              = {0};
                        fc.glyphID                = (int)ck;
                        tc->label[tc->labellen++] = fc;
                }

                tc->txtptr = &txt[i + 1];
                for (i++; txt[i] != ']'; i++)
                        ;
        }
        PRINTF("labels\n");
        for (int k = 0; k < TEXTBOX_NUM_CHOICES; k++) {
                textboxchoice_s *tc = &tb->choices[k];
                for (int i = 0; i < tc->labellen; i++) {
                        PRINTF("%c\n", tb->choices[k].label[i].glyphID);
                }
        }
}

static bool32 textbox_next_page(textbox_s *tb)
{
        tb->currspeed = TEXTBOX_TICKS_PER_CHAR_Q4;
        textbox_clr(tb);
        textboxline_s *line = &tb->lines[0];
        while (tb->tok->type != DIALOG_TOK_END) {
                switch (tb->tok->type) {
                case DIALOG_TOK_CMD: {
                        const char *s = &tb->dialogmem[tb->tok->i0];
                        if (0) {
                        } else if (str_matches(s, "~")) {
                                tb->curreffect = FNT_EFFECT_WAVE;
                        } else if (str_matches(s, "/~")) {
                                tb->curreffect = FNT_EFFECT_NONE;
                        } else if (str_matches(s, "n")) {
                                if (!textbox_new_line(tb, &line)) return 0;
                        } else if (str_matches(s, "-")) {
                                line->speed[line->n] = TEXTBOX_TICKS_PER_CHAR_Q4 * 8;
                        } else if (str_matches(s, ">>")) {
                                if (tb->tok->i1 - tb->tok->i0 > 3) {
                                        tb->currspeed = os_i32_from_str(&s[3]);
                                } else {
                                        tb->currspeed = TEXTBOX_TICKS_PER_CHAR_Q4;
                                }
                        } else if (str_matches(s, "trigger")) {
                                int triggerID          = os_i32_from_str(&s[8]);
                                line->trigger[line->n] = triggerID;
                                // game_trigger(&g_gamestate, triggerID);
                        } else if (str_matches(s, "*")) {
                                tb->curreffect = FNT_EFFECT_SHAKE;
                        } else if (str_matches(s, "/*")) {
                                tb->curreffect = FNT_EFFECT_NONE;
                        } else if (str_matches(s, "choice")) {
                                textbox_cmd_choice(tb, tb->tok);
                                tb->tok++;
                                return 1;
                        }
                } break;
                case DIALOG_TOK_PORTRAIT: {

                } break;
                case DIALOG_TOK_TEXT: {
                        for (int i = tb->tok->i0; i <= tb->tok->i1; i++) {
                                char ci = tb->dialogmem[i];
                                if (ci == '\n' || ci == '\r') {
                                        if (!textbox_new_line(tb, &line))
                                                return 0;
#ifdef TARGET_PD
                                        i++; // skip two characters \r\n
#endif
                                        continue;
                                }
                                fntchar_s fc = {0};
                                fc.glyphID   = ci;
                                fc.effectID  = tb->curreffect;
                                ASSERT(line->n < TEXTBOX_CHARS_PER_LINE);
                                int speed = tb->currspeed;
                                if (char_matches_any(ci, ".,!?"))
                                        speed *= TEXTBOX_TICK_PUNCTUATION_MARK;
                                line->speed[line->n] = speed;
                                line->chars[line->n] = fc;
                                line->n++;
                        }
                } break;
                case DIALOG_TOK_TEXT_NEW_PAGE:
                        tb->tok++;
                        return 1;
                }
                tb->tok++;
        }
        tb->state = TEXTBOX_STATE_CLOSING;
        return 0;
}

void textbox_init(textbox_s *tb)
{
        textbox_clr(tb);
}

void textbox_load_dialog(textbox_s *tb, char *filename)
{
        textbox_clr(tb);
#if 1
        txt_read_file(filename, tb->dialogmem, TEXTBOX_FILE_MEM);
#else
        os_strcpy(tb->dialogmem, text);
#endif
        dialog_parse(tb->dialogmem, tb->toks);
        tb->tok   = tb->toks;
        tb->state = TEXTBOX_STATE_OPENING;
        textbox_next_page(tb);
}

void textbox_input(game_s *g, textbox_s *tb)
{
        if (tb->state != TEXTBOX_STATE_WAITING) return;
        if (os_inp_just_pressed(INP_A)) {
                os_inp_set_pressedp(INP_A); // disable "just pressed A" for this frame
                if (tb->n_choices) {
                        textbox_select_choice(g, tb, tb->cur_choice);
                } else {
                        if (textbox_next_page(tb)) {
                                tb->state = TEXTBOX_STATE_WRITING;
                        }
                }
        } else if (tb->n_choices) {
                if (os_inp_just_pressed(INP_DOWN) &&
                    ++tb->cur_choice >= tb->n_choices) {
                        tb->cur_choice = 0;
                }
                if (os_inp_just_pressed(INP_UP) &&
                    --tb->cur_choice < 0) {
                        tb->cur_choice = tb->n_choices - 1;
                }
        }
}

void textbox_update(game_s *g, textbox_s *tb)
{
        switch (tb->state) {
        case TEXTBOX_STATE_OPENING:
                if (++tb->animationticks == TEXTBOX_ANIMATION_TICKS) {
                        tb->state          = TEXTBOX_STATE_WRITING;
                        tb->animationticks = 0;
                }
                return;
        case TEXTBOX_STATE_CLOSING:
                if (++tb->animationticks == TEXTBOX_ANIMATION_TICKS) {
                        tb->state          = TEXTBOX_STATE_INACTIVE;
                        tb->animationticks = 0;
                }
                return;
        case TEXTBOX_STATE_WAITING: return;
        }

        tb->typewriter_tick_q4 -= 16;
        textboxline_s *line        = &tb->lines[tb->curr_line];
        bool32         playedsound = 0;
        while (tb->typewriter_tick_q4 <= 0) {
                tb->typewriter_tick_q4 += line->speed[tb->curr_char];

                while (tb->curr_char == line->n) {
                        line          = &tb->lines[++tb->curr_line];
                        tb->curr_char = 0;
                        if (line >= &tb->lines[TEXTBOX_LINES]) {
                                tb->state                = TEXTBOX_STATE_WAITING;
                                tb->page_animation_state = 0;
                                return;
                        }
                }

                int i = line->chars[tb->curr_char].glyphID;
                if (line->trigger[tb->curr_char] != 0) {
                        game_trigger(g, line->trigger[tb->curr_char++]);
                }
                tb->curr_char++;
                line->n_shown++;
                if (playedsound) continue;

                if (('A' <= i && i <= 'Z') || ('a' <= i && i <= 'z')) {
                        playedsound = 1;
                        snd_play_ext(snd_get(SNDID_TYPEWRITE),
                                     0.25f,
                                     rngf_range(0.7f, 1.f));
                }
        }
}

void textbox_select_choice(game_s *g, textbox_s *tb, int choiceID)
{
        ASSERT(tb->state == TEXTBOX_STATE_WAITING && tb->n_choices > 0);
        tb->state            = TEXTBOX_STATE_CLOSING;
        textboxchoice_s *tc  = &tb->choices[choiceID];
        const char      *txt = tc->txtptr;
        if (0) {
        } else if (str_matches(txt, "dialog")) {
                char filename[64] = {0};
                for (int i = 7, k = 0; txt[i] != ']'; i++, k++) {
                        filename[k] = txt[i];
                }
                textbox_load_dialog(tb, filename);
                tb->state = TEXTBOX_STATE_WRITING;
        } else if (str_matches(txt, "trigger")) {
                int triggerID = os_i32_from_str(&txt[8]);
                game_trigger(g, triggerID);
        }
}