// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

static int textbox_text_length(fnt_s f, textbox_char_s *chars, int n_chars)
{
    int len = 0;
    for (int i = 0; i < n_chars; i++) {
        textbox_char_s ci = chars[i];
        len += f.widths[ci.glyph];
    }
    return len;
}

void textbox_load_dialog(textbox_s *tb, const char *filename)
{
    *tb          = (textbox_s){0};
    tb->state    = TEXTBOX_STATE_FADE_IN;
    tb->fadetick = TEXTBOX_FADE_TICKS;

    char  *txt;
    json_s jroot;
    spm_push();

    FILEPATH_GEN(filepath, FILEPATH_DIALOG, filename);

    txt_load(filepath, spm_alloc, &txt);
    json_root(txt, &jroot);
    for (json_each (jroot, "blocks", jblock)) {
        textbox_block_s *block = &tb->blocks[tb->n_blocks++];
        block->tag             = jsonk_u32(jblock, "tag");

        int effect  = TEXTBOX_EFFECT_NONE;
        int tick_q2 = TEXTBOX_SPEED_DEFAULT_Q2;
        int n_lines = 0;

        for (json_each (jblock, "lines", jline)) {
            int linelength = 0;

            for (char *c = json_strp(jline, NULL); *c != '\"'; c++) {

                // INLINE COMMMANDS
                if (*c == '{') { // parse inline command eg {> 4}
                    c++;

                    switch (*c) {
                    case '/': // ====== END EFFECT
                        c++;
                        switch (*c) {
                        case '>': tick_q2 = TEXTBOX_SPEED_DEFAULT_Q2; break;
                        case '~':
                        case '*': effect = TEXTBOX_EFFECT_NONE; break;
                        default: BAD_PATH
                        }
                        break;
                    case '>': tick_q2 = u32_from_str(c + 2); break;
                    case '~': effect = TEXTBOX_EFFECT_WAVE; break;
                    case '*': effect = TEXTBOX_EFFECT_SHAKE; break;
                    default: BAD_PATH
                    }

                    while (*c != '}')
                        c++;
                } else {
                    // TEXT CHARACTER
                    textbox_char_s tbc = {0};
                    tbc.effect         = effect;
                    tbc.tick_q2        = tick_q2;
                    tbc.glyph          = *c;

                    if (char_is_any(*c, ".!?")) {
                        tbc.tick_q2 *= 2;
                    }

                    block->chars[block->n_chars++] = tbc;
                    linelength++;
                }
            }

            block->line_length[n_lines++] = linelength;
        }

        // CHOICES
        for (json_each (jblock, "choices", jchoice)) {
            textbox_choice_s *choice = &block->choices[block->n_choices++];
            for (char *c = jsonk_strp(jchoice, "text", NULL); *c != '\"'; c++) {
                choice->chars[choice->n_chars++] = *c;
            }

            char action[16] = {0};
            jsonk_strs(jchoice, "action", action);
            if (0) {
            } else if (str_eq(action, "goto")) {
                choice->type    = TEXTBOX_CHOICE_GOTO;
                choice->gototag = jsonk_u32(jchoice, "tag");
            } else if (str_eq(action, "exit")) {
                choice->type = TEXTBOX_CHOICE_EXIT;
            }
        }
    }

    spm_pop();
}

static void select_tbblock(textbox_s *tb)
{
    if (tb->block < tb->n_blocks) {
        tb->n       = 0;
        tb->tick_q2 = 0;
        tb->tick    = 0;
        tb->state   = TEXTBOX_STATE_WRITE;
    } else {
        tb->state    = TEXTBOX_STATE_FADE_OUT;
        tb->fadetick = TEXTBOX_FADE_TICKS;
    }
}

void textbox_update(game_s *g, textbox_s *tb)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;
    tb->tick++;

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {
        textbox_block_s *b = &tb->blocks[tb->block];
        if (inp_just_pressed(INP_A)) {
            if (b->n_choices <= 0) {
                tb->block++;
                select_tbblock(tb);
                break;
            }

            textbox_choice_s *choice = &b->choices[tb->curchoice];

            switch (choice->type) {
            case TEXTBOX_CHOICE_GOTO: {
                int found = 0;
                for (int i = 0; i < tb->n_blocks; i++) {
                    if (tb->blocks[i].tag == choice->gototag) {
                        found     = 1;
                        tb->block = i;
                        select_tbblock(tb);
                        break;
                    }
                }
                if (!found) {
                    BAD_PATH
                }

            } break;
            case TEXTBOX_CHOICE_EXIT: {
                tb->state    = TEXTBOX_STATE_FADE_OUT;
                tb->fadetick = TEXTBOX_FADE_TICKS;
            } break;
            }
            break;
        }

        if (b->n_choices <= 0) break;

        if (inp_just_pressed(INP_DPAD_D))
            tb->curchoice = min_i(tb->curchoice + 1, b->n_choices - 1);
        if (inp_just_pressed(INP_DPAD_U))
            tb->curchoice = max_i(tb->curchoice - 1, 0);
    } break;
    case TEXTBOX_STATE_WRITE: {
        textbox_block_s *b = &tb->blocks[tb->block];
        assert(tb->n < b->n_chars);

        tb->tick_q2 += 4;
        int tick = b->chars[tb->n].tick_q2;
        while (tb->tick_q2 >= tick) {
            tb->tick_q2 -= tick;
            tb->n++;

            if (tb->n >= b->n_chars) {
                tb->state     = TEXTBOX_STATE_WAIT;
                tb->curchoice = 0;
                break;
            }

            int glyph = b->chars[tb->n].glyph;

            if (glyph != ' ') {
                snd_play_ext(asset_snd(SNDID_SPEAK), 0.2f, rngr_f32(.5f, .7f));
            }
        }
    } break;
    case TEXTBOX_STATE_FADE_IN:
        tb->fadetick--;
        if (tb->fadetick <= 0) {
            tb->state = TEXTBOX_STATE_WRITE;
        }
        break;
    case TEXTBOX_STATE_FADE_OUT:
        tb->fadetick--;
        if (tb->fadetick <= 0) {
            tb->state = TEXTBOX_STATE_INACTIVE;
        }
        break;
    }
}

void textbox_draw(textbox_s *tb, v2_i32 camoffset)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;

    textbox_block_s *b   = &tb->blocks[tb->block];
    gfx_ctx_s        ctx = gfx_ctx_default(asset_tex(0));

#define TB_LINE_SPACING 26

    switch (tb->state) {
    case TEXTBOX_STATE_FADE_IN:
        ctx.pat = gfx_pattern_interpolate(TEXTBOX_FADE_TICKS - tb->fadetick, TEXTBOX_FADE_TICKS);
        break;
    case TEXTBOX_STATE_FADE_OUT:
        ctx.pat = gfx_pattern_interpolate(tb->fadetick, TEXTBOX_FADE_TICKS);
        break;
    }

    gfx_ctx_s ctx_bg = ctx;
    gfx_rec_fill(ctx_bg, (rec_i32){0, 150, 400, 90}, PRIM_MODE_BLACK);

    v2_i32   pos = {10, 160};
    fnt_s    fnt = asset_fnt(FNTID_LARGE);
    v2_i32   p   = pos;
    texrec_s t   = {0};
    t.t          = fnt.t;
    t.r.w        = fnt.grid_w;
    t.r.h        = fnt.grid_h;

    for (int i = 0, len = 0, row = 0; i < tb->n; i++) {
        textbox_char_s ci = b->chars[i];
        t.r.x             = fnt.grid_w * (ci.glyph & 31);
        t.r.y             = fnt.grid_h * (ci.glyph >> 5);

        v2_i32 pp = p;
        switch (ci.effect) {
        case TEXTBOX_EFFECT_SHAKE:
            pp.x += (i32)(rng_u32() & 1);
            pp.y += (i32)(rng_u32() & 1);
            break;
        case TEXTBOX_EFFECT_WAVE:
            pp.y += (sin_q16(i * 20000 + tb->tick * 4000) * 2) >> 16;
            break;
        }

        gfx_spr(ctx, t, pp, 0, SPR_MODE_WHITE);

        p.x += fnt.widths[ci.glyph];

        len++;
        if (len >= b->line_length[row]) {
            len = 0;
            row++;
            p.x = pos.x;
            p.y += TB_LINE_SPACING;
        }
    }

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {
        if (b->n_choices <= 0) break;

        for (int n = 0; n < b->n_choices; n++) {
            textbox_choice_s *choice = &b->choices[n];
            v2_i32            pp     = {250, 80 + n * 20};
            for (int i = 0; i < choice->n_chars; i++) {
                int ci = choice->chars[i];
                t.r.x  = fnt.grid_w * (ci & 31);
                t.r.y  = fnt.grid_h * (ci >> 5);
                gfx_spr(ctx, t, pp, 0, 0);
                pp.x += fnt.widths[ci];
            }
        }

        gfx_rec_fill(ctx, (rec_i32){220, 80 + tb->curchoice * 20, 8, 8}, PRIM_MODE_BLACK);
    } break;
    case TEXTBOX_STATE_WRITE: {

    } break;
    }
}