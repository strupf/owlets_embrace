// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "assets.h"

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
    *tb       = (textbox_s){0};
    tb->state = TEXTBOX_STATE_WRITE;
    fnt_s font;

    char  *txt;
    json_s jroot;
    spm_push();
    txt_load(filename, spm_alloc, &txt);
    json_root(txt, &jroot);

    for (json_each(jroot, "blocks", jblock)) {
        textbox_block_s *block = &tb->blocks[tb->n_blocks++];

        textbox_char_s tbc = {0};
        tbc.effect         = TEXTBOX_EFFECT_NONE;
        tbc.tick_q2        = TEXTBOX_SPEED_DEFAULT_Q2;
        int n_lines        = 0;
        for (json_each(jblock, "lines", jline)) {
            int linelength = 0;
            for (char *c = json_strp(jline, NULL); *c != '\"'; c++) {
                if (*c == '{') { // parse inline command eg {> 4}
                    c++;

                    switch (*c) {
                    case '/': // ====== END EFFECT
                        c++;
                        switch (*c) {
                        case '>':
                            tbc.tick_q2 = TEXTBOX_SPEED_DEFAULT_Q2;
                            break;
                        case '~':
                        case '*':
                            break;
                        default: BAD_PATH
                        }
                        break;
                    case '>': // ====== APPLY NEW EFFECT
                        tbc.tick_q2 = u32_from_str(c + 2);
                        break;
                    case '~':
                        tbc.effect = TEXTBOX_EFFECT_WAVE;
                        break;
                    case '*':
                        tbc.effect = TEXTBOX_EFFECT_SHAKE;
                        break;
                    default: BAD_PATH
                    }

                    while (*c != '}')
                        c++;
                } else {
                    tbc.glyph                      = *c;
                    block->chars[block->n_chars++] = tbc;
                    linelength++;
                }
            }

            block->line_length[n_lines++] = linelength;
        }
    }

    spm_pop();
}

void textbox_update(textbox_s *tb)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;
    tb->tick++;

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {
        if (inp_just_pressed(INP_A)) {
            tb->block++;
            if (tb->block < tb->n_blocks) {
                tb->n       = 0;
                tb->tick_q2 = 0;
                tb->tick    = 0;
                tb->state   = TEXTBOX_STATE_WRITE;
            } else {
                tb->state = TEXTBOX_STATE_INACTIVE;
            }
        }
    } break;
    case TEXTBOX_STATE_WAIT_CHOICE: {
        if (inp_just_pressed(INP_A)) {
        }
    } break;
    case TEXTBOX_STATE_WRITE: {
        textbox_block_s *b = &tb->blocks[tb->block];
        assert(tb->n < b->n_chars);

        tb->tick_q2 += 4;
        int tick = b->chars[tb->n].tick_q2;
        while (tb->tick_q2 >= tick) {
            tb->tick_q2 -= tick;
            tb->n++;
            if (tb->n < b->n_chars) continue;

            if (b->n_choices > 0)
                tb->state = TEXTBOX_STATE_WAIT_CHOICE;
            else
                tb->state = TEXTBOX_STATE_WAIT;

            break;
        }
    } break;
    }
}

void textbox_draw(textbox_s *tb, v2_i32 camoffset)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;

    v2_i32 pos = {10, 150};

    textbox_block_s *b   = &tb->blocks[tb->block];
    fnt_s            fnt = asset_fnt(FNTID_DIALOG);
    gfx_ctx_s        ctx = gfx_ctx_default(asset_tex(0));
    v2_i32           p   = pos;
    texrec_s         t   = {0};
    t.t                  = fnt.t;
    t.r.w                = fnt.grid_w;
    t.r.h                = fnt.grid_h;

    texrec_s tb_frame;
    tb_frame.t = asset_tex(TEXID_UI_TEXTBOX);
    tb_frame.r = (rec_i32){0, 0, 400, 240};
    gfx_spr(ctx, tb_frame, (v2_i32){0}, 0, 0);
    // gfx_rec_fill(ctx, (rec_i32){0, 120, 400, 120}, PRIM_MODE_WHITE);

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

        gfx_spr(ctx, t, pp, 0, 0);

        p.x += fnt.widths[ci.glyph];

        len++;
        if (len >= b->line_length[row]) {
            len = 0;
            row++;
            p.x = pos.x;
            p.y += 30;
        }
    }

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {

    } break;
    case TEXTBOX_STATE_WAIT_CHOICE: {

    } break;
    case TEXTBOX_STATE_WRITE: {

    } break;
    }
}