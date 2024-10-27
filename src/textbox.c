// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

#define DIALOG_CHAR_TICK_Q2_DEFAULT 4
#define TEXTBOX_HEIGHT              64

void textbox_update_wait(textbox_s *tb);
void textbox_update_write(textbox_s *tb, bool32 fast_forward);
void textbox_start_writing(textbox_s *tb, dialog_frame_s *frame);
void textbox_start_exit(textbox_s *tb);
i32  textbox_dialog_line_len(dialog_str_s *line);

bool32 textbox_load_dialog(game_s *g, const char *fname)
{
    FILEPATH_GEN(filepath, FILEPATH_DIALOG, fname);
    str_append(filepath, ".json");

    spm_push();
    spm_align(4);
    char *txt = NULL;
    if (!txt_load(filepath, spm_alloc, &txt)) {
        pltf_log("Couldn't load dialog: %s\n", filepath);
        spm_pop();
        return 0;
    }

    json_s jroot = {0};
    if (!json_root(txt, &jroot)) {
        pltf_log("Dialog json error: %s\n", filepath);
        spm_pop();
        return 0;
    }

    textbox_s *tb  = &g->textbox;
    tb->timer_fade = 0;
    tb->timer      = 0;
    tb->n_frames   = 0;
    fnt_s fnt      = asset_fnt(TEXTBOX_FNTID);

    for (json_each (jroot, "blocks", jblock)) {
        if (tb->n_frames == TEXTBOX_NUM_FRAMES - 1) {
            pltf_log("Can't fit another dialog frame!\n");
            break;
        }

        dialog_frame_s *frame = &tb->frames[tb->n_frames++];
        *frame                = (dialog_frame_s){0};

        i32 fx      = 0;
        i32 tick_q2 = DIALOG_CHAR_TICK_Q2_DEFAULT;

        for (json_each (jblock, "lines", jline)) {
            if (frame->n_lines == DIALOG_NUM_LINES - 1) {
                pltf_log("Can't fit another line!\n");
                break;
            }
            dialog_str_s *line = &frame->lines[frame->n_lines++];
            for (char *c = json_strp(jline, NULL); *c != '\"'; c++) {
                if (*c == '{') { // parse inline command eg {> 4}
                    c++;

                    switch (*c) {
                    case '/': // end effect
                        c++;
                        switch (*c) {
                        case '>': tick_q2 = DIALOG_CHAR_TICK_Q2_DEFAULT; break;
                        case '~':
                        case '*': fx = 0; break;
                        }
                        break;
                    case '>': tick_q2 = u32_from_str(c + 1); break;
                    case '~': fx = DIALOG_CHAR_FX_WAVE; break;
                    case '*': fx = DIALOG_CHAR_FX_SHAKE; break;
                    }

                    while (*c != '}') {
                        c++;
                    }
                } else if (line->len < DIALOG_NUM_CHARS_PER_LINE) { // text character
                    i32 tbc_tick = tick_q2;

                    if (char_is_any(*c, ".!?")) {
                        tbc_tick *= 2;
                    }

                    if (U8_MAX < tbc_tick) {
                        pltf_log("time bigger than range!\n");
                        tbc_tick = U8_MAX;
                    }

                    dialog_char_s tbc        = {0};
                    tbc.fx                   = fx;
                    tbc.tick_q2              = tbc_tick;
                    tbc.c                    = *c;
                    line->chars[line->len++] = tbc;
                } else {
                    pltf_log("Dialog line cropped:\n  ");
                    BAD_PATH
                    c++;
                    while (*c != '\"') {
                        pltf_log("%c", *c);
                        c++;
                    }
                }
            }
        }

        json_s jtag;
        if (json_key(jblock, "tag", &jtag)) {
            frame->tag = jsonk_u32(jblock, "tag");
        }

        // choices
        for (json_each (jblock, "choices", jchoice)) {
            if (frame->n_choices == DIALOG_NUM_CHOICES - 1) {
                pltf_log("Dialog: Can't find another choice!\n");
                break;
            }
            dialog_choice_s *choice = &frame->choices[frame->n_choices++];

            for (char *c = jsonk_strp(jchoice, "text", NULL); *c != '\"'; c++) {
                choice->chars[choice->n_chars++] = *c;
            }

            char action[16] = {0};
            jsonk_strs(jchoice, "action", action);
            if (0) {
            } else if (str_eq(action, "goto")) {
                choice->action    = DIALOG_CHOICE_GOTO;
                choice->frame_tag = jsonk_u32(jchoice, "tag");
            } else if (str_eq(action, "exit")) {
                choice->action = DIALOG_CHOICE_EXIT;
            } else if (str_eq(action, "shop")) {
                choice->action = DIALOG_CHOICE_OPEN_SHOP;
            } else if (str_eq(action, "trigger")) {
                choice->action  = DIALOG_CHOICE_TRIGGER;
                choice->trigger = jsonk_i32(jchoice, "trigger");
            }
        }
    }

    spm_pop();

    textbox_start_writing(tb, &tb->frames[0]);
    tb->state   = TEXTBOX_STATE_FADE_IN;
    g->substate = SUBSTATE_TEXTBOX;
    return 1;
}

void textbox_update(game_s *g)
{
    textbox_s *tb = &g->textbox;
    tb->timer++;

    switch (tb->state) {
    case TEXTBOX_STATE_FADE_IN:
        tb->timer_fade++;
        if (TEXTBOX_FADE_I <= tb->timer_fade) {
            tb->timer_fade = 0;
            tb->state      = TEXTBOX_STATE_WRITE;
        }
        break;
    case TEXTBOX_STATE_FADE_OUT:
        tb->timer_fade--;
        if (tb->timer_fade <= 0) {
            tb->state   = TEXTBOX_STATE_INACTIVE;
            g->substate = 0;
        }
        break;
    case TEXTBOX_STATE_WAIT:
        textbox_update_wait(tb);
        break;
    case TEXTBOX_STATE_WRITE: {
        if (inp_action_jp(INP_A)) {
            tb->fast_forward = 1; // need to toggle again
        }

        bool32 fast_forward = (tb->fast_forward && inp_action(INP_A));
        textbox_update_write(tb, fast_forward);
        break;
    }
    }
}

void textbox_update_wait(textbox_s *tb)
{
    if (tb->frame->n_choices) {
        if (inp_action_jp(INP_DU)) {
            tb->cur_choice--;
        }
        if (inp_action_jp(INP_DD)) {
            tb->cur_choice++;
        }
        tb->cur_choice += tb->frame->n_choices;
        tb->cur_choice %= tb->frame->n_choices;
    }

    if (!inp_action_jp(INP_A)) return;

    if (tb->frame->n_choices) {
        dialog_choice_s choice = tb->frame->choices[tb->cur_choice];
        switch (choice.action) {
        case DIALOG_CHOICE_GOTO: {
            dialog_frame_s *frame = NULL;

            for (i32 n = 0; n < tb->n_frames; n++) {
                dialog_frame_s *f = &tb->frames[n];
                if (f->tag != choice.frame_tag) {
                    frame = f;
                    break;
                }
            }

            if (frame) {
                textbox_start_writing(tb, frame);
                break;
            }
            // FALLTHROUGH
        }
        case DIALOG_CHOICE_EXIT: {
            textbox_start_exit(tb);
            break;
        }
        }
    } else {
        if (tb->cur_frame + 1 < tb->n_frames) {
            textbox_start_writing(tb, tb->frame + 1);
        } else {
            textbox_start_exit(tb);
        }
    }
}

void textbox_update_write(textbox_s *tb, bool32 fast_forward)
{
    tb->tick_q2 += (1 << 2) << (fast_forward ? 1 : 0);

    while (1) {
        dialog_str_s *l = &tb->frame->lines[tb->cur_line];
        dialog_char_s c = l->chars[tb->cur_char];
        if (tb->tick_q2 < c.tick_q2) break;

        // next character
        tb->tick_q2 -= c.tick_q2;
        tb->cur_char++;
        if (!fast_forward) {
            // play sound
        }

        if (tb->cur_char < l->len) continue;

        // next line
        tb->cur_char = 0;
        tb->cur_line++;
        if (tb->cur_line < tb->frame->n_lines) continue;

        // finished
        tb->state = TEXTBOX_STATE_WAIT;
        return;
    }
}

void textbox_start_writing(textbox_s *tb, dialog_frame_s *frame)
{
    tb->frame        = frame;
    tb->timer        = 0;
    tb->tick_q2      = 0;
    tb->cur_choice   = 0;
    tb->cur_char     = 0;
    tb->cur_line     = 0;
    tb->cur_frame    = (i32)(frame - &tb->frames[0]);
    tb->state        = TEXTBOX_STATE_WRITE;
    tb->fast_forward = 0;
}

void textbox_start_exit(textbox_s *tb)
{
    tb->state      = TEXTBOX_STATE_FADE_OUT;
    tb->timer_fade = TEXTBOX_FADE_O;
}

void textbox_draw(game_s *g, v2_i32 camoffset)
{
    textbox_s *tb  = &g->textbox;
    gfx_ctx_s  ctx = gfx_ctx_display();

    i32 offs = TEXTBOX_HEIGHT;

    switch (tb->state) {
    case TEXTBOX_STATE_FADE_IN:
        // ctx.pat = gfx_pattern_interpolate(tb->timer_fade, TEXTBOX_FADE_I);
        offs = lerp_i32(0, TEXTBOX_HEIGHT, tb->timer_fade, TEXTBOX_FADE_I);
        break;
    case TEXTBOX_STATE_FADE_OUT:
        // ctx.pat = gfx_pattern_interpolate(tb->timer_fade, TEXTBOX_FADE_O);
        offs = lerp_i32(0, TEXTBOX_HEIGHT, tb->timer_fade, TEXTBOX_FADE_O);
        break;
    }

    for (i32 y = 0; y < PLTF_DISPLAY_H - offs; y++) {
        for (i32 w = 0; w < PLTF_DISPLAY_WWORDS; w++) {
            ctx.dst.px[w + y * PLTF_DISPLAY_WWORDS] =
                ctx.dst.px[w + (y + offs) * PLTF_DISPLAY_WWORDS];
        }
    }

    rec_i32 tbr = {0, PLTF_DISPLAY_H - offs, PLTF_DISPLAY_W, TEXTBOX_HEIGHT};
    gfx_rec_fill(ctx, tbr, GFX_COL_BLACK);
    v2_i32 pos_og = {10, PLTF_DISPLAY_H - offs};
#define TB_LINE_SPACING 26

    switch (tb->frame->n_lines) {
    case 1: pos_og.y += TB_LINE_SPACING; break;
    case 2: pos_og.y += TB_LINE_SPACING / 2; break;
    }

    fnt_s    fnt        = asset_fnt(TEXTBOX_FNTID);
    texrec_s tr         = {fnt.t, {0, 0, fnt.grid_w, fnt.grid_h}};
    u32      shake_seed = tb->timer >> 1; // shake at half FPS

    for (i32 l = 0; l < tb->frame->n_lines && l <= tb->cur_line; l++) {
        dialog_str_s *line = &tb->frame->lines[l];
        i32           nc   = (l == tb->cur_line ? tb->cur_char : line->len);
        v2_i32        pos  = {pos_og.x, pos_og.y + l * TB_LINE_SPACING};

        for (i32 n = 0; n < nc; n++) {
            dialog_char_s c = line->chars[n];
            tr.r.x          = (c.c & 31) * fnt.grid_w;
            tr.r.y          = (c.c >> 5) * fnt.grid_h;

            v2_i32 posc = pos;
            switch (c.fx) {
            case DIALOG_CHAR_FX_SHAKE:
                posc.x += rngsr_u32(&shake_seed, 0, 1);
                posc.y += rngsr_u32(&shake_seed, 0, 1);
                break;
            case DIALOG_CHAR_FX_WAVE:
                posc.y += sin_q16((tb->timer << 12) + (n << 14)) >> 15;
                break;
            }

            gfx_spr(ctx, tr, posc, 0, SPR_MODE_WHITE);
            pos.x += fnt.widths[c.c];
        }
    }

    if (tb->state == TEXTBOX_STATE_WAIT) {
        if (tb->frame->n_choices) {

        } else {
            // arrow bop animation
#define TB_ANIM_TICKS       15
#define TB_ANIM_TICKS_IDLE  20
#define TB_ANIM_TOTAL_TICKS (TB_ANIM_TICKS + TB_ANIM_TICKS_IDLE)

            i32 atick = tb->timer % TB_ANIM_TOTAL_TICKS;
            i32 fr    = 0;
            if (TB_ANIM_TICKS_IDLE <= atick) {
                i32 ti = atick - TB_ANIM_TICKS_IDLE;
                fr     = tick_to_index_freq(ti, 6, TB_ANIM_TICKS);
            }

            texrec_s tarrow = asset_texrec(TEXID_UI, fr * 32, 176, 32, 32);
            if ((tb->frame - &tb->frames[0]) == tb->n_frames - 1)
                tarrow.r.y += 64 + 16;
            gfx_spr(ctx, tarrow, (v2_i32){360, 200}, 0, 0);
        }
    }
}

i32 textbox_dialog_line_len(dialog_str_s *line)
{
    fnt_s fnt = asset_fnt(TEXTBOX_FNTID);
    i32   l   = 0;
    for (i32 n = 0; n < line->len; n++) {
        l += fnt.widths[line->chars[n].c];
    }
    return l;
}