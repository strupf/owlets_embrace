// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dialog.h"
#include "game.h"

typedef struct dialog_parser_s {
    u8              *c;
    i32              has_flags_ticks_tmp;
    i32              flags_ticks_tmp;
    i32              flags_ticks;
    i32              next_trigger;
    i32              voice;
    i32              pos;
    dialog_line_s   *line;
    dialog_choice_s *choice;
    dialog_frame_s  *frame;
} dialog_parser_s;

i32    dialog_conv_hex(i32 c1, i32 c2);
bool32 dialog_str_equals(u8 *c, const char *strc);
bool32 dialog_line_try_add_char(dialog_parser_s *p, dialog_char_s c);
void   dialog_parse_cmd(g_s *g, dialog_s *d, dialog_parser_s *p);
void   dialog_set_speed(dialog_parser_s *p, i32 ticks);
void   dialog_newframe_cmd(dialog_s *d, dialog_parser_s *p);
void   dialog_newline_cmd(dialog_s *d, dialog_parser_s *p);
i32    dialog_set_speed_x(i32 f, i32 ticks);

void dialog_newline_cmd(dialog_s *d, dialog_parser_s *p)
{
    if (2 == p->frame->n_lines) {
        dialog_newframe_cmd(d, p);
    } else {
        p->line = &p->frame->lines[p->frame->n_lines++];
    }
}

void dialog_newframe_cmd(dialog_s *d, dialog_parser_s *p)
{
    if (p->frame) {
        p->frame->trigger = p->next_trigger;
        p->next_trigger   = 0;
    }
    p->frame = &d->frames[d->n_frames++];
    mclr(p->frame, sizeof(dialog_frame_s));

    p->frame->pos         = p->pos;
    p->frame->speak_voice = p->voice;
    p->line               = &p->frame->lines[p->frame->n_lines++];
}

void dialog_parse_string(g_s *g, dialog_s *d, const char *str)
{
    mclr(d, sizeof(dialog_s));
    dialog_parser_s  parser = {0};
    dialog_parser_s *p      = &parser;
    p->flags_ticks          = DIALOG_SPEED_DEFAULT;
    p->choice               = &d->choices[0];
    dialog_newframe_cmd(d, p);

    for (p->c = (u8 *)str; *p->c != '\0'; p->c++) {
        switch (*p->c) {
        case '\n': {
            b32 empty_line = 0;
            u8 *cc         = p->c + 1;
            while (1) {
                if (*cc == '\0') goto ENDPARSING;
                if (*cc == '\n') {
                    empty_line = 1;
                    break;
                }
                if (*cc == ' ') {
                    cc++;
                } else {
                    break;
                }
            }

            if (empty_line) {
                p->c = cc;
                dialog_newframe_cmd(d, p);
            } else {
                dialog_newline_cmd(d, p);
            }
            break;
        }
        case '{': {
            dialog_parse_cmd(g, d, p);
            break;
        }
        default: {
            dialog_char_s dc = {0};
            dc.c             = *p->c;
            dc.flags_ticks   = p->flags_ticks;
            dialog_line_try_add_char(p, dc);
            break;
        }
        }
    }
ENDPARSING:;
}

bool32 dialog_line_try_add_char(dialog_parser_s *p, dialog_char_s c)
{
    if (p->has_flags_ticks_tmp) {
        c.flags_ticks          = p->flags_ticks_tmp;
        p->has_flags_ticks_tmp = 0;
    } else {
        c.flags_ticks = p->flags_ticks;
    }
    c.trigger       = p->next_trigger;
    p->next_trigger = 0;
    if (p->line && p->line->n_chars < DIALOG_NUM_CHARS_PER_LINE) {
        p->line->chars[p->line->n_chars++] = c;
        return 1;
    }
    return 0;
}

void dialog_parse_cmd(g_s *g, dialog_s *d, dialog_parser_s *p)
{
    assert(*p->c == '{');
    p->c++;
    switch (*p->c) {
    case '.': // {.E}
        p->flags_ticks_tmp     = dialog_set_speed_x(p->flags_ticks, num_from_hex(p->c[1]));
        p->has_flags_ticks_tmp = 1;
        p->c += 2;
        break;
    case 'n':
        p->c++;
        dialog_newline_cmd(d, p);
        break;
    case '*':
        p->c++;
        p->flags_ticks |= DIALOG_CFLAG_SHAKE;
        break;
    case '~':
        p->c++;
        p->flags_ticks |= DIALOG_CFLAG_WAVE;
        break;
    case '>': { // {>0} {>3} {>E}
        dialog_set_speed(p, num_from_hex(p->c[1]));
        p->c += 2;
        break;
    }
    case '/': {
        p->c++;
        switch (*p->c) {
        case '>':
            dialog_set_speed(p, DIALOG_SPEED_DEFAULT);
            break;
        case '*':
            p->flags_ticks &= ~DIALOG_CFLAG_SHAKE;
            break;
        case '~':
            p->flags_ticks &= ~DIALOG_CFLAG_WAVE;
            break;
        }
        p->c++;
        break;
    }
    }

    if (0) {
    } else if (dialog_str_equals(p->c, "Player")) {
        p->c += 6; // + "Player"
        for (u8 *pc = &g->owl.name[0]; *pc != '\0'; pc++) {
            dialog_char_s namec = {0};
            namec.c             = *pc;
            dialog_line_try_add_char(p, namec);
        }
    } else if (dialog_str_equals(p->c, "Trigger")) { // Trigger:FF
        p->next_trigger = dialog_conv_hex(p->c[8 + 0], p->c[8 + 1]);
        p->c += 10;
    } else if (dialog_str_equals(p->c, "top")) { // Top/bot
        p->pos        = DIALOG_POS_TOP;
        p->frame->pos = p->pos;
        p->c += 3;
    } else if (dialog_str_equals(p->c, "bot")) { // Top/bot
        p->pos        = DIALOG_POS_BOT;
        p->frame->pos = p->pos;
        p->c += 3;
    } else if (dialog_str_equals(p->c, "Voice")) { // Voice:F
        p->voice              = dialog_conv_hex(0, p->c[6]);
        p->frame->speak_voice = p->voice;
        p->c += 7;
    } else if (dialog_str_equals(p->c, "Char")) { // Char:EF
        i32           sID   = dialog_conv_hex(p->c[5 + 0], p->c[5 + 1]);
        dialog_char_s namec = {0};
        namec.c             = sID;
        dialog_line_try_add_char(p, namec);
        p->c += 7;
    } else if (dialog_str_equals(p->c, "Choice")) {
        p->c += 7; // {Choice:Yes, please!:FF:01}
        dialog_choice_s *choice = &d->choices[d->n_choices++];
        u8              *cc     = &choice->chars[0];
        while (*p->c != ':') {
            *cc = *p->c;
            cc++;
            p->c++;
        }
        choice->ID1 = dialog_conv_hex(p->c[1], p->c[2]);
        if (p->c[3] == ':') {
            choice->ID2 = dialog_conv_hex(p->c[4], p->c[5]);
            p->c += 6;
        } else {
            p->c += 3;
        }
    } else {
    }
    while (*p->c != '}') {
        p->c++;
    }
}

i32 dialog_conv_hex(i32 c1, i32 c2)
{
    return ((num_from_hex(c1) << 4) | num_from_hex(c2));
}

i32 dialog_set_speed_x(i32 f, i32 ticks)
{
    return ((f & ~0xF) | (ticks & 0xF));
}

void dialog_set_speed(dialog_parser_s *p, i32 ticks)
{
    p->flags_ticks = dialog_set_speed_x(p->flags_ticks, ticks);
}

bool32 dialog_str_equals(u8 *c, const char *strc)
{
    char *x = (char *)c;
    char *y = (char *)strc;

    while (1) {
        if (*y == '\0' || *y == '}') return 1;
        if (char_lower(*x) != char_lower(*y)) break;
        x++;
        y++;
    }
    return 0;
}
