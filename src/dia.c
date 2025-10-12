// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dia.h"
#include "game.h"

void dia_parse(g_s *g, dia_s *d, void *str);

void dia_next_frame(g_s *g)
{
    dia_s *d = &g->dia;
    if (d->state == DIA_ST_WAITING) {
        d->timer = 0;

        if (d->c_frame == d->n_frames - 1) {
            d->state = DIA_ST_CLOSING;
            game_on_trigger(g, TRIGGER_DIA_END);

            if (d->n_choices) {
                game_on_trigger(g, TRIGGER_DIA_CHOICE_1 + d->c_choice);
            }
        } else {
            d->state            = DIA_ST_WRITING;
            d->last_char_spoken = 0;
            d->speak_counter    = 0;
            d->c_char           = 0;
            d->c_line           = 0;
            d->c_frame++;
            game_on_trigger(g, TRIGGER_DIA_FRAME_NEW);
        }
    }
}

void dia_load_from_wad(g_s *g, void *wadname)
{
    wad_el_s *wad_el = 0;
    dia_s    *d      = &g->dia;
    void     *f      = wad_open_str(wadname, 0, &wad_el);
    if (!f) {
        BAD_PATH();
        return;
    }

    spm_push();
    void *buf = spm_alloc(wad_el->size);
    pltf_file_r(f, buf, wad_el->size);
    dia_parse(g, d, buf);
    spm_pop();

    pltf_log("---\n");
    for (i32 n = 0; n < d->n_frames; n++) {
        dia_frame_s *fr = &d->frames[n];

        for (i32 k = 0; k < DIA_NUM_LINES_PER_FRAME; k++) {
            dia_line_s *li = &fr->lines[k];

            for (i32 i = 0; i < li->n_toks; i++) {
                dia_tok_s t = li->toks[i];

                switch (t.type) {
                case DIA_TOK_CHAR: {
                    pltf_log("%c", t.v);
                    break;
                }
                default: {
                    pltf_log("{cmd %i %i}", t.type, t.v);
                }
                }
            }
            pltf_log("#\n");
        }
        pltf_log(".\n");
    }
    pltf_log("---\n");

    d->script_input     = 0;
    d->c_char           = 0;
    d->c_frame          = 0;
    d->c_line           = 0;
    d->timer            = 0;
    d->last_char_spoken = 0;
    d->speak_counter    = 0;
    d->c_choice         = 0;
    d->c_speed_q4       = DIA_SPEED_Q4_DEFAULT;
    if (d->state == DIA_ST_NULL) {
        d->state = DIA_ST_OPENING;
    } else {
        d->state = DIA_ST_WRITING;
    }
}

void dia_update(g_s *g, inp_s inp)
{
    dia_s *d = &g->dia;

    switch (d->state) {
    case DIA_ST_NULL: {
        break;
    }
    case DIA_ST_OPENING: {
        d->timer++;
        if (DIA_TICKS_CLOSE <= d->timer) {
            d->timer = 0;
            d->state = DIA_ST_WRITING;
        }
        break;
    }
    case DIA_ST_CLOSING: {
        d->timer++;
        if (DIA_TICKS_OPEN <= d->timer) {
            d->timer = 0;
            d->state = DIA_ST_NULL;
        }
        break;
    }
    case DIA_ST_WRITING: {
        dia_frame_s *fr = &d->frames[d->c_frame];
        d->timer += d->c_speed_q4;

        bool32 show_all = 0;
        if (!!d->script_input && (inps_btn_jp(inp, INP_A) || inps_btn_jp(inp, INP_B))) {
            show_all = 1;
        }

        while (show_all || 16 <= d->timer) {
            dia_line_s *li = &fr->lines[d->c_line];
            dia_tok_s   t  = li->toks[d->c_char++];

            switch (t.type) {
            default: break;
            case DIA_TOK_CHAR: {
                i32 c = t.v;
                d->timer -= 16;
                if (char_is_any(t.v, ".,:;!?")) { // extra pause by setting the timer further behind
                    d->timer -= 48;
                }

                if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')) {
                    d->last_char_spoken = 1;
                    d->speak_counter++;
                } else if (d->last_char_spoken) {
                    d->last_char_spoken = 0;
                    d->speak_counter    = DIA_SPEAK_SND_N;
                } else {
                    d->speak_counter = DIA_SPEAK_SND_N - 1;
                }
                break;
            }
            case DIA_TOK_TRIGGER: {
                game_on_trigger(g, t.v);
                break;
            }
            case DIA_TOK_PAUSE: {
                d->timer -= t.v;
                break;
            }
            case DIA_TOK_SFX: {
                break;
            }
            case DIA_TOK_SPEED: {
                d->c_speed_q4 = t.v;
                break;
            }
            case DIA_TOK_SPEED_RESET: {
                d->c_speed_q4 = DIA_SPEED_Q4_DEFAULT;
                break;
            }
            }

            if (li->n_toks <= d->c_char) {
                d->c_line++;
                d->c_char = 0;

                if (fr->n_lines <= d->c_line) {
                    d->state = DIA_ST_WAITING;
                    d->timer = 0;
                    game_on_trigger(g, TRIGGER_DIA_FRAME_END);
                    break;
                }
            }
        }

        if (DIA_SPEAK_SND_N <= d->speak_counter) {
            d->speak_counter = 0;
            // i32 sID          = SFXID_SPEAK1 + frame->speak_voice;
            // snd_play(sID, 0.35f, rngr_f32(0.9f, 1.1f));
        }
        break;
    }
    case DIA_ST_WAITING: {
        d->timer++;

        if (!d->script_input && (inps_btn_jp(inp, INP_A) || inps_btn_jp(inp, INP_B))) {
            dia_next_frame(g);
        } else {
            bool32 choice_window = d->c_frame == d->n_frames - 1 && d->n_choices;
            if (choice_window && inps_btn_jp(inp, INP_DD) && d->c_choice < d->n_choices - 1) {
                d->c_choice++;
            }
            if (choice_window && inps_btn_jp(inp, INP_DU) && 0 < d->c_choice) {
                d->c_choice--;
            }
        }
        break;
    }
    }
}

void dia_draw(g_s *g)
{
    gfx_ctx_s    ctx    = gfx_ctx_display();
    dia_s       *d      = &g->dia;
    dia_frame_s *fr     = &d->frames[d->c_frame];
    fnt_s        f      = asset_fnt(FNTID_MEDIUM);
    i32          offs_y = DIA_H_SLIDE;

    switch (d->state) {
    case DIA_ST_OPENING:
        offs_y = ease_out_quad(0, DIA_H_SLIDE, d->timer, DIA_TICKS_OPEN);
        break;
    case DIA_ST_CLOSING:
        offs_y = ease_out_quad(DIA_H_SLIDE, 0, d->timer, DIA_TICKS_CLOSE);
        break;
    }

    i32       tb_y      = 240 - offs_y;
    i32       textyp    = tb_y + 7;
    rec_i32   rfill     = {0, 0, PLTF_DISPLAY_W, offs_y};
    rec_i32   rborder   = {0, 0, PLTF_DISPLAY_W, 2};
    gfx_ctx_s ctxborder = ctx;

    rfill.y       = PLTF_DISPLAY_H - offs_y;
    rborder.y     = PLTF_DISPLAY_H - offs_y + 2;
    texrec_s tr   = {f.t, 0, 0, f.grid_w, f.grid_h};
    ctxborder.pat = gfx_pattern_50();
    gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);
    gfx_rec_fill(ctxborder, rborder, PRIM_MODE_BLACK_WHITE);

    u32    ptime      = pltf_cur_tick();
    u32    shake_seed = ptime >> 2;
    bool32 fx_shake   = 0;
    bool32 fx_wave    = 0;

    // for each line
    for (i32 n = 0; n <= d->c_line; n++) {
        v2_i32      pos = {10, textyp + n * 20};
        dia_line_s *li  = &fr->lines[n];
        i32         k2  = (n < d->c_line ? li->n_toks : d->c_char);

        // for each character in line
        for (i32 k = 0; k < k2; k++) {
            dia_tok_s t = li->toks[k];

            switch (t.type) {
            case DIA_TOK_CHAR: {
                i32    c    = t.v;
                v2_i32 posc = pos;
                tr.x        = (c & 31) * f.grid_w;
                tr.y        = (c >> 5) * f.grid_h;

                // visual effect
                if (fx_shake) {
                    posc.x += rngsr_i32(&shake_seed, 0, 1);
                    posc.y += rngsr_i32(&shake_seed, 0, 1);
                }
                if (fx_wave) {
                    posc.y += (sin_q16((ptime << 12) + (k << 14)) / 32000);
                }

                gfx_spr(ctx, tr, posc, 0, SPR_MODE_INV);
                pos.x += f.widths[c] + f.tracking;

                // peek next rendered char to apply kerning
                for (i32 i = k + 1; i < li->n_toks; i++) {
                    dia_tok_s ti = li->toks[i];
                    if (ti.type == DIA_TOK_CHAR) {
                        pos.x -= fnt_kerning(f, c, ti.v);
                        break;
                    }
                }
                break;
            }
            case DIA_TOK_FX_SHAKE:
                fx_shake = 1;
                break;
            case DIA_TOK_FX_SHAKE_END:
                fx_shake = 0;
                break;
            case DIA_TOK_FX_WAVE:
                fx_wave = 1;
                break;
            case DIA_TOK_FX_WAVE_END:
                fx_wave = 0;
                break;
            }
        }
    }
}

void dia_set_script_input(g_s *g, b32 script_input)
{
    g->dia.script_input = script_input;
}

// parsing
// -------------------------------------------------------------------------------------------------

static void dia_parse_try_add_tok(dia_line_s *l, dia_tok_s t);

// skips all characters until encountering any of the chars, or the null character
// returns a pointer to that character
static u8 *dia_parse_skip_until_any_char(u8 *p, const void *chars);

// starts from the character '{', parses the command;
// return pointer to the character just after '}'
static u8 *dia_parse_cmd(u8 *p, dia_tok_s *cmd, u8 *cmdstr);

// certain commands get filled in inplace e.g. inserting player defined names
// returns 1 if the command was executed in place, returns 0 if it must be
// executed during dialog writing
static void dia_parse_tok_insert(g_s *g, dia_s *d, dia_frame_s *fr, dia_tok_s cmd, u8 *cmdstr);

static void dia_parse_line_remove_trailing_ws(dia_line_s *li);

void dia_parse(g_s *g, dia_s *d, void *str)
{
    u8          *c      = (u8 *)str;
    dia_frame_s *frames = d->frames;
    dia_frame_s *fr     = &frames[0];
    mclr(d->frames, sizeof(d->frames));
    fr->n_lines  = 1;
    d->n_choices = 0;
    d->n_frames  = 1;

    while (*c != '\0') {
        switch (*c) {
        case '\t':
        case '\v':
        case '\f':
        case '\r':
            c++;
            break;
        case '{': {
            dia_tok_s cmd        = {0};
            u8        cmdstr[32] = {0};
            c                    = dia_parse_cmd(c, &cmd, cmdstr) + 1;
            dia_parse_tok_insert(g, d, fr, cmd, cmdstr);
            break;
        }
        case '\n': {
            dia_parse_line_remove_trailing_ws(&fr->lines[fr->n_lines - 1]);

            c++;
            bool32    is_empty_line  = 0;
            i32       n_tok_queue    = 0;
            dia_tok_s tok_queue[12]  = {0};
            u8        cmdstr[12][32] = {0};

            while (1) {
                switch (*c) {
                case '\0':
                    goto FOUND_EOF;
                default:
                    goto FOUND_CHAR;
                case '\n':
                    is_empty_line = 1;
                    c++;
                    break;
                case ' ':
                case '\t':
                case '\r':
                case '\v':
                case '\f':
                    c++;
                    break;
                case '#':
                    c = dia_parse_skip_until_any_char(c, "\n");
                    break;
                case '{':
                    c = dia_parse_cmd(c, &tok_queue[n_tok_queue], cmdstr[n_tok_queue]) + 1;
                    n_tok_queue++;
                    break;
                }
            }

        FOUND_CHAR:;
            if (fr->n_lines == DIA_NUM_LINES_PER_FRAME || is_empty_line) {
                if (d->n_frames == ARRLEN(d->frames)) {
                    goto END_PARSING;
                }
                fr = &frames[d->n_frames++];
            }
            fr->n_lines++;

            // commands apply to potentially new frame so they had to
            // be stored temporarely
            for (i32 n = 0; n < n_tok_queue; n++) {
                dia_tok_s t = tok_queue[n];
                dia_parse_tok_insert(g, d, fr, t, cmdstr[n]);
            }
        FOUND_EOF:;
            break;
        }
        case '#': {
            c = dia_parse_skip_until_any_char(c, "\n");
            break;
        }
        default: {
            dia_tok_s t = {DIA_TOK_CHAR, (u8)*c};
            dia_parse_tok_insert(g, d, fr, t, 0);
            c++;
            break;
        }
        }
    }
END_PARSING:;
    dia_parse_line_remove_trailing_ws(&fr->lines[fr->n_lines - 1]);
}

static void dia_parse_try_add_tok(dia_line_s *l, dia_tok_s t)
{
    if (l->n_toks < DIA_NUM_CHARS_PER_LINE) {
        l->toks[l->n_toks++] = t;
    }
}

static u8 *dia_parse_skip_until_any_char(u8 *p, const void *chars)
{
    u8 *c = p;
    while (*c != '\0' && !char_is_any(*c, chars)) {
        c++;
    }
    return c;
}

static u8 *dia_parse_cmd(u8 *p, dia_tok_s *cmd, u8 *strbuf)
{
    assert(*p == '{');
    mclr(cmd, sizeof(dia_tok_s));
    u8 *c = p + 1;

    if (0) {
    } else if (str_eq_nc(c, "char:")) {
        cmd->type = DIA_TOK_CHAR;
        c += 5;
        cmd->v = i32_from_str(c);
    } else if (str_eq_nc(c, "trigger:")) {
        cmd->type = DIA_TOK_TRIGGER;
        c += 8;
        cmd->v = i32_from_str(c);
    } else if (str_eq_nc(c, "pause:")) {
        cmd->type = DIA_TOK_PAUSE;
        c += 6;
        cmd->v = i32_from_str(c);
    } else if (*c == '>') {
        cmd->type = DIA_TOK_SPEED;
        c += 1;
        cmd->v = i32_from_str(c);
    } else if (str_eq_nc(c, "choice:")) {
        cmd->type = DIA_TOK_CHOICE;
        c += 7;
        if (strbuf) {
            while (*c != '\0' && *c != '}') {
                *strbuf++ = *c++;
            }
        }
    } else if (*c == '~') {
        cmd->type = DIA_TOK_FX_WAVE;
    } else if (*c == '*') {
        cmd->type = DIA_TOK_FX_SHAKE;
    } else if (*c == '/') {
        c++;
        switch (*c) {
        case '~':
            cmd->type = DIA_TOK_FX_WAVE_END;
            break;
        case '*':
            cmd->type = DIA_TOK_FX_SHAKE_END;
            break;
        case '>':
            cmd->type = DIA_TOK_SPEED_RESET;
            break;
        }
    }
    return dia_parse_skip_until_any_char(p, "}");
}

static void dia_parse_tok_copy_str(dia_frame_s *fr, const void *str)
{
    dia_line_s *li = &fr->lines[fr->n_lines - 1];
    const u8   *c  = (const u8 *)str;
    while (*c) {
        dia_tok_s t = {DIA_TOK_CHAR, *c};
        dia_parse_try_add_tok(li, t);
        c++;
    }
}

static void dia_parse_tok_insert(g_s *g, dia_s *d, dia_frame_s *fr, dia_tok_s t, u8 *strbuf)
{
    dia_line_s *li = &fr->lines[fr->n_lines - 1];
    switch (t.type) {
    default: {
        dia_parse_try_add_tok(li, t);
        break;
    }
    case DIA_TOK_CHOICE: {
        dia_choice_s *ch = &d->choices[d->n_choices++];
        str_cpy(ch->txt, strbuf);
        break;
    }
    case DIA_TOK_NAME_PLAYER: {
        // TODO: get actual name from game
        u8 name[16] = "Owlet";
        dia_parse_tok_copy_str(fr, (const void *)name);
        break;
    }
    }
}

static void dia_parse_line_remove_trailing_ws(dia_line_s *li)
{
    for (i32 n = li->n_toks - 1; li->n_toks && 0 <= n; n--) {
        dia_tok_s *t = &li->toks[n];
        if (t->type != DIA_TOK_CHAR) continue;

        // remove this character if trailing whitespace
        if (char_is_ws(t->v)) {
            mmov(t, t + 1, sizeof(dia_tok_s) * ((i32)li->n_toks - n));
            li->n_toks--;
        } else {
            break;
        }
    }
}