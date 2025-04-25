// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dialog.h"
#include "game.h"

#define DIALOG_TICKS_FADE_OPEN  10
#define DIALOG_TICKS_FADE_CLOSE 10
#define DIALOG_H_SLIDE          56

void dialog_action_exe(g_s *g, dialog_s *d, i32 ID1, i32 ID2);
void dialog_parse_string(g_s *g, dialog_s *d, const char *str);

void dialog_open_wad(g_s *g, const void *name)
{
    dialog_open_wad_hash(g, wad_hash(name));
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
        bool32 next = !d->script_input &&
                      (inp_btn_jp(INP_A) || inp_btn_jp(INP_B));

        if (next) {
            game_on_trigger(g, TRIGGER_DIALOG_NEW_FRAME);
            dialog_continue_next_frame(g);
        }
        break;
    }
    case DIALOG_ST_WAITING_END: {
        d->tick++;
        bool32 next = !d->script_input &&
                      inp_btn_jp(INP_A);

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

        while (show_all || d->tick_char <= d->tick) {
            d->tick -= d->tick_char;

            dialog_frame_s *frame = &d->frames[d->c_frame];
            dialog_line_s  *line  = &frame->lines[d->c_line];
            dialog_char_s   ch    = line->chars[line->n_visible++];
            d->tick_char          = ch.flags_ticks & 0xF;

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
        break;
    }
    }
}

void dialog_draw(g_s *g)
{
    dialog_s *d = &g->dialog;
    if (d->state == 0) return;

    fnt_s     f   = asset_fnt(FNTID_LARGE);
    gfx_ctx_s ctx = gfx_ctx_display();

    i32 offs_y = DIALOG_H_SLIDE;
    if (1) {
        // move current drawn scene upwards via memmove

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
    }

    i32      tb_y  = 240 - offs_y;
    i32      dt_y  = DIALOG_H_SLIDE - offs_y;
    texrec_s tr    = {f.t, 0, 0, f.grid_w, f.grid_h};
    rec_i32  rfill = {0, PLTF_DISPLAY_H - offs_y, PLTF_DISPLAY_W, offs_y};

    gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);
    if (d->state == DIALOG_ST_WAITING_END && d->n_choices) {
        rec_i32 rchoices = {240, 110, PLTF_DISPLAY_W, PLTF_DISPLAY_H};
        gfx_rec_fill(ctx, rchoices, GFX_COL_BLACK);
    }

    {
        // textbox border top
        rec_i32   rr3  = rfill;
        rec_i32   rr2  = rfill;
        gfx_ctx_s ctx1 = ctx;
        ctx1.pat       = gfx_pattern_2x2(B2(01), B2(10));

        rr3.h = 1;
        rr3.y += 3;
        rr2.h = 3;
        rr2.y += 2;
        gfx_rec_fill(ctx1, rr2, GFX_COL_WHITE);
        gfx_rec_fill(ctx, rr3, GFX_COL_WHITE);
    }

    // draw dialog lines
    u32             ptime      = pltf_cur_tick();
    u32             shake_seed = ptime >> 2;
    dialog_frame_s *frame      = &d->frames[d->c_frame];

    for (i32 n = 0; n < frame->n_lines; n++) {
        dialog_line_s *line = &frame->lines[n];
        v2_i32         pos  = {10, tb_y + 10 + n * 22};

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

            gfx_spr(ctx, tr, posc, 0, SPR_MODE_WHITE);
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

        gfx_spr(ctx, tr_arrow, CINIT(v2_i32){400 - 32, 240 - 50}, 0, 0);
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
#

void dialog_continue_next_frame(g_s *g)
{
    dialog_s *d = &g->dialog;
    switch (d->state) {
    case DIALOG_ST_WAITING_NEXT: {
        d->tick   = 0;
        d->state  = DIALOG_ST_WRITING;
        d->c_line = 0;
        d->c_frame++;
        break;
    }
    case DIALOG_ST_WAITING_END: {
        dialog_close(g);
        break;
    }
    }
}