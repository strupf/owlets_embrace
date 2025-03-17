// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dialog.h"
#include "game.h"

typedef struct {
    u8              *c;
    i32              n_char;
    i32              n_line;
    i32              flags_ticks;
    i32              next_sndID;
    i32              next_sndvol;
    dialog_line_s   *line;
    dialog_choice_s *choice;
} dialog_parser_s;

i32    dialog_conv_hex(i32 c1, i32 c2);
bool32 dialog_str_equals(u8 *c, const char *strc);
bool32 dialog_line_try_add_char(dialog_parser_s *p, dialog_char_s c);
void   dialog_parse_cmd(g_s *g, dialog_s *d, dialog_parser_s *p);
void   dialog_set_speed(dialog_parser_s *p, i32 ticks);

void dialog_newline_cmd(dialog_s *d, dialog_parser_s *p)
{
    p->n_char = 0;
    p->n_line++;

    if (p->n_line < DIALOG_NUM_LINES) {
        d->n_lines++;
        p->line++;
    } else {
        p->line = 0;
    }
}

void dialog_parse_string(g_s *g, dialog_s *d, const char *str)
{
    mclr(d, sizeof(dialog_s));
    dialog_parser_s  parser = {0};
    dialog_parser_s *p      = &parser;
    p->flags_ticks          = DIALOG_SPEED_DEFAULT;
    p->line                 = &d->lines[0];
    p->choice               = &d->choices[0];
    d->n_lines              = 1;

    for (p->c = (u8 *)str; *p->c != '\0'; p->c++) {
        switch (*p->c) {
        case '\n': {
            i32 cn = (i32)p->c[1];
            if (cn == '\n' || cn == '[' || cn == '\0') {
                goto ENDPARSING;
            }
            dialog_newline_cmd(d, p);
            break;
        }
        case '{': {
            dialog_parse_cmd(g, d, p);
            break;
        }
        default: {
            dialog_char_s dc = {*p->c, p->flags_ticks};
            dialog_line_try_add_char(p, dc);
            break;
        }
        }
    }
ENDPARSING:;

    for (i32 n = 0; n < d->n_lines; n++) {
        dialog_line_s *line = &d->lines[n];
        for (i32 k = 0; k < line->n_chars; k++) {
            pltf_log("%c", (char)line->chars[k].c);
        }
        pltf_log("\n");
    }
}

bool32 dialog_line_try_add_char(dialog_parser_s *p, dialog_char_s c)
{
    if (p->next_sndID) {
        c.sndID       = p->next_sndID;
        c.sndvol      = p->next_sndvol;
        p->next_sndID = 0;
    }
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
    default: {
        if (0) {
        } else if (dialog_str_equals(p->c, "Player")) {
            p->c += 6; // + "Player"
            for (u8 *pc = &g->hero.name[0]; *pc != '\0'; pc++) {
                dialog_char_s namec = {*pc, p->flags_ticks};
                dialog_line_try_add_char(p, namec);
            }
        } else if (dialog_str_equals(p->c, "Sound")) { // Sound:02:EF
            p->next_sndID  = dialog_conv_hex(p->c[6 + 0], p->c[6 + 1]);
            p->next_sndvol = dialog_conv_hex(p->c[6 + 3], p->c[6 + 4]);
            p->c += 11;
        } else if (dialog_str_equals(p->c, "Symbol")) { // Symbol:EF
            i32           sID   = dialog_conv_hex(p->c[7 + 0], p->c[7 + 1]);
            dialog_char_s namec = {sID, p->flags_ticks};
            dialog_line_try_add_char(p, namec);
            p->c += 9;
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
        break;
    }
    }
    while (*p->c != '}') {
        p->c++;
    }
}

i32 dialog_conv_hex(i32 c1, i32 c2)
{
    return ((num_from_hex(c1) << 4) | num_from_hex(c2));
}

void dialog_set_speed(dialog_parser_s *p, i32 ticks)
{
    p->flags_ticks = (p->flags_ticks & ~0xF) | (ticks & 0xF);
}

bool32 dialog_str_equals(u8 *c, const char *strc)
{
    char *x = (char *)c;
    char *y = (char *)strc;

    while (1) {
        if (*y == '\0' || *y == '}') return 1;
        if (*x != *y) break;
        x++;
        y++;
    }
    return 0;
}
