// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dialog.h"
#include "game.h"

#define DIALOG_TICKS_FADE_OPEN  10
#define DIALOG_TICKS_FADE_CLOSE 10
#define DIALOG_H_SLIDE          52
#define DIALOG_SPEAK_SND_N      3 // speak snd every n chars

void dialog_action_exe(g_s *g, dialog_s *d, i32 ID1, i32 ID2);
void dialog_parse_string(g_s *g, dialog_s *d, const char *str);

void dialog_open_wad(g_s *g, const void *name)
{
    dialog_open_wad_hash(g, wad_hash(name));
}

void dialog_open_wad_pos(g_s *g, const void *name, i32 pos)
{
    dialog_open_wad(g, name);
    g->dialog.pos = pos;
}

void dialog_open_wad_hash(g_s *g, u32 hash)
{
    void     *f;
    wad_el_s *w;
    if (!wad_open(hash, &f, &w)) {
        return;
    }

    spm_push();
    dialog_s *d = &g->dialog;
    mclr(d, sizeof(dialog_s));

    void *txt = wad_r_spm(f, w, hash);
    dialog_parse_string(g, d, txt);
    d->state = DIALOG_ST_OPENING;
    d->tick  = 0;
    d->pos   = DIALOG_POS_BOT;
    spm_pop();
}

void dialog_update(g_s *g)
{
    dialog_s *d = &g->dialog;
    if (d->state == DIALOG_ST_NULL) {
        return;
    }

    switch (d->state) {
    case DIALOG_ST_OPENING: {
        d->tick++;
        if (DIALOG_TICKS_FADE_OPEN <= d->tick) {
            d->tick  = 0;
            d->state = DIALOG_ST_WRITING;
        }
        break;
    }
    case DIALOG_ST_CLOSING: {
        d->tick++;
        if (DIALOG_TICKS_FADE_CLOSE <= d->tick) {
            d->tick  = 0;
            d->state = DIALOG_ST_NULL;
        }
        break;
    }
    case DIALOG_ST_WAITING_NEXT: {
        d->tick++;
        bool32 next = (d->script_input && d->tick == 60) ||
                      (!d->script_input &&
                       (inp_btn_jp(INP_A) || inp_btn_jp(INP_B)));

        if (next) {
            game_on_trigger(g, TRIGGER_DIALOG_NEW_FRAME);
            dialog_continue_next_frame(g);
        }
        break;
    }
    case DIALOG_ST_WAITING_END: {
        d->tick++;
        bool32 next = (d->script_input && d->tick == 60) ||
                      (!d->script_input &&
                       (inp_btn_jp(INP_A) || inp_btn_jp(INP_B)));

        if (d->n_choices) {
            d->cur_choice += inp_btn_jp(INP_DD) - inp_btn_jp(INP_DU);
            d->cur_choice = clamp_i32(d->cur_choice, 0, d->n_choices - 1);

            if (next) {
                dialog_choice_s *choice = &d->choices[d->cur_choice];
                dialog_action_exe(g, d, choice->ID1, choice->ID2);
                d->tick  = 0;
                d->state = DIALOG_ST_NULL;
            }
        } else if (next) {
            dialog_continue_next_frame(g);
            game_on_trigger(g, TRIGGER_DIALOG_END);
        }
        break;
    }
    case DIALOG_ST_WRITING: {
        d->tick += 8;
        bool32 show_all = !d->script_input &&
                          (inp_btn_jp(INP_B) || inp_btn_jp(INP_A));
        dialog_frame_s *frame = &d->frames[d->c_frame];

        while (show_all || d->tick_char <= d->tick) {
            d->tick -= d->tick_char;

            dialog_line_s *line = &frame->lines[d->c_line];
            dialog_char_s  ch   = line->chars[line->n_visible++];
            d->tick_char        = ch.flags_ticks & 0xF;

            if (('a' <= ch.c && ch.c <= 'z') ||
                ('A' <= ch.c && ch.c <= 'Z') ||
                ('0' <= ch.c && ch.c <= '9')) {
                d->last_char_spoken = 1;
                d->speak_counter++;
            } else if (d->last_char_spoken) {
                d->last_char_spoken = 0;
                d->speak_counter    = DIALOG_SPEAK_SND_N;
            } else {
                d->speak_counter = DIALOG_SPEAK_SND_N - 1;
            }

            switch (ch.c) {
            case '.':
            case '!':
            case '?': d->tick_char *= 6; break;
            case ';':
            case ':':
            case ',': d->tick_char *= 4; break;
            default: break;
            }

            if (ch.trigger) {
                game_on_trigger(g, TRIGGER_DIALOG + ch.trigger);
            }

            if (line->n_visible == line->n_chars) {
                d->c_line++;

                if (d->c_line == frame->n_lines) { // rendered all lines
                    if (d->c_frame == d->n_frames - 1) {
                        d->state = DIALOG_ST_WAITING_END;
                    } else {
                        d->state = DIALOG_ST_WAITING_NEXT;
                        game_on_trigger(g, TRIGGER_DIALOG_END_FRAME);
                        game_on_trigger(g, TRIGGER_DIALOG + frame->trigger);
                    }
                    d->tick = 0;
                    break;
                }
            }
        }

        if (show_all) {
            d->speak_counter = 0;
        }

        if (DIALOG_SPEAK_SND_N <= d->speak_counter) {
            d->speak_counter = 0;
            i32 sID          = SNDID_SPEAK1 + frame->speak_voice;
            snd_play(sID, 0.35f, rngr_f32(0.9f, 1.1f));
        }
        break;
    }
    }
}

void dialog_draw(g_s *g)
{
    dialog_s *d = &g->dialog;
    if (d->state == 0) return;

    fnt_s     f   = asset_fnt(FNTID_MEDIUM);
    gfx_ctx_s ctx = gfx_ctx_display();

    i32 offs_y = DIALOG_H_SLIDE;
    switch (d->state) {
    case DIALOG_ST_OPENING:
        offs_y = ease_out_quad(0, DIALOG_H_SLIDE,
                               d->tick, DIALOG_TICKS_FADE_OPEN);
        break;
    case DIALOG_ST_CLOSING:
        offs_y = ease_out_quad(DIALOG_H_SLIDE, 0,
                               d->tick, DIALOG_TICKS_FADE_CLOSE);
        break;
    }

    i32             tb_y      = 0;
    i32             textyp    = 0;
    rec_i32         rfill     = {0, 0, PLTF_DISPLAY_W, offs_y};
    rec_i32         rborder   = {0, 0, PLTF_DISPLAY_W, 2};
    gfx_ctx_s       ctxborder = ctx;
    dialog_frame_s *frame     = &d->frames[d->c_frame];
    texrec_s        tr        = {f.t, 0, 0, f.grid_w, f.grid_h};
    ctxborder.pat             = gfx_pattern_50();

    switch (frame->pos) {
    case DIALOG_POS_BOT: {
        tb_y      = 240 - offs_y;
        textyp    = tb_y + 7;
        rfill.y   = PLTF_DISPLAY_H - offs_y;
        rborder.y = PLTF_DISPLAY_H - offs_y + 2;
        break;
    }
    case DIALOG_POS_TOP: {
        tb_y      = 0;
        textyp    = 4 - (DIALOG_H_SLIDE - offs_y);
        rfill.y   = 0;
        rborder.y = offs_y - rborder.h - 2;
        break;
    }
    }

    gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);
    gfx_rec_fill(ctxborder, rborder, PRIM_MODE_BLACK_WHITE);

    if (d->state == DIALOG_ST_WAITING_END && d->n_choices) {
        rec_i32 rchoices = {240, 110, PLTF_DISPLAY_W, PLTF_DISPLAY_H};
        gfx_rec_fill(ctx, rchoices, GFX_COL_BLACK);
    }

    // draw dialog lines
    u32 ptime      = pltf_cur_tick();
    u32 shake_seed = ptime >> 2;

    for (i32 n = 0; n < frame->n_lines; n++) {
        dialog_line_s *line = &frame->lines[n];
        v2_i32         pos  = {10, textyp + n * 20};

        for (i32 k = 0; k < line->n_visible; k++) {
            dialog_char_s ch   = line->chars[k];
            v2_i32        posc = pos;
            tr.x               = (ch.c & 31) * f.grid_w;
            tr.y               = (ch.c >> 5) * f.grid_h;

            if (ch.flags_ticks & DIALOG_CFLAG_SHAKE) {
                posc.x += rngsr_i32(&shake_seed, 0, 1);
                posc.y += rngsr_i32(&shake_seed, 0, 1);
            }
            if (ch.flags_ticks & DIALOG_CFLAG_WAVE) {
                posc.y += (sin_q16((ptime << 12) + (k << 14)) / 32000);
            }

            gfx_spr(ctx, tr, posc, 0, SPR_MODE_INV);
            pos.x += f.widths[ch.c] + f.tracking;
            if (k < line->n_visible - 1) {
                pos.x -= fnt_kerning(f, ch.c, line->chars[k + 1].c);
            }
        }
    }

    // choices
    if (d->state == DIALOG_ST_WAITING_END) {
        for (i32 k = 0; k < d->n_choices; k++) {
            dialog_choice_s *choice = &d->choices[k];
            v2_i32           pos    = {250, 120 + k * 20};

            for (u8 *ch = choice->chars; *ch != '\0'; ch++) {
                i32 c = *ch;
                tr.x  = (c & 31) * f.grid_w;
                tr.y  = (c >> 5) * f.grid_h;
                gfx_spr(ctx, tr, pos, 0, SPR_MODE_WHITE);
                pos.x += f.widths[c] + f.tracking - fnt_kerning(f, c, ch[1]);
            }
        }
    }

    // next page animation
    i32 state_next_page = d->script_input ? 0 : d->state;

    switch (state_next_page) {
    case DIALOG_ST_WAITING_END:
    case DIALOG_ST_WAITING_NEXT: {
        if (d->n_choices) break;

        i32 fr_x_arrow = 0;
        i32 fr_y_arrow = fr_y_arrow = (d->state == DIALOG_ST_WAITING_END);

#define DIALOG_ANIM_TICKS_HOLD   40
#define DIALOG_ANIM_TICKS_BOUNCE 10

        i32 t = d->tick % (DIALOG_ANIM_TICKS_HOLD + DIALOG_ANIM_TICKS_BOUNCE);
        if (DIALOG_ANIM_TICKS_HOLD <= t) {
            t -= DIALOG_ANIM_TICKS_HOLD;
            fr_x_arrow = lerp_i32(1, 6, t, DIALOG_ANIM_TICKS_BOUNCE);
        }

        texrec_s tr_arrow = asset_texrec(TEXID_UI,
                                         fr_x_arrow * 32,
                                         176 + fr_y_arrow * 32,
                                         32, 32);

        gfx_spr(ctx, tr_arrow, CINIT(v2_i32){400 - 32, textyp + 8}, 0, 0);
        break;
    }
    }
}

void dialog_close(g_s *g)
{
    dialog_s *d = &g->dialog;
    d->state    = DIALOG_ST_CLOSING;
    d->tick     = 0;
}

void dialog_continue_next_frame(g_s *g)
{
    dialog_s *d = &g->dialog;
    switch (d->state) {
    case DIALOG_ST_WAITING_NEXT: {
        d->speak_counter = DIALOG_SPEAK_SND_N - 1;
        d->tick          = 0;
        d->state         = DIALOG_ST_WRITING;
        d->c_line        = 0;
        d->c_frame++;
        break;
    }
    case DIALOG_ST_WAITING_END: {
        dialog_close(g);
        break;
    }
    }
}