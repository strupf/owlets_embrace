// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textbox.h"
#include "game.h"

bool32 textbox_finished(textbox_s *tb)
{
    return (tb->state == TEXTBOX_STATE_INACTIVE);
}

static int textbox_text_length(fnt_s f, textbox_char_s *chars, int n_chars)
{
    int len = 0;
    for (int i = 0; i < n_chars; i++) {
        textbox_char_s ci = chars[i];
        len += f.widths[ci.glyph];
    }
    return len;
}

void textbox_load_dialog(game_s *g, textbox_s *tb, const char *filename)
{
    *tb = (textbox_s){0};
    FILEPATH_GEN(filepath, FILEPATH_DIALOG, filename);
    str_append(filepath, ".json");

    spm_push();
    char  *txt;
    json_s jroot;
    if (txt_load(filepath, spm_alloc, &txt) != TXT_SUCCESS) {
        sys_printf("could not load dialog: %s\n", filepath);
        goto DIALOG_ERR;
    }

    if (json_root(txt, &jroot) != JSON_SUCCESS) {
        sys_printf("could not find dialog json root: %s\n", filepath);
        goto DIALOG_ERR;
    }

    tb->state    = TEXTBOX_STATE_WRITE;
    tb->fade_out = 0;
    tb->fade_in  = 1;

    for (json_each (jroot, "blocks", jblock)) {
        textbox_block_s *block = &tb->blocks[tb->n_blocks++];
        block->tag             = jsonk_u32(jblock, "tag");

        int effect     = TEXTBOX_EFFECT_NONE;
        int tick_q2    = TEXTBOX_SPEED_DEFAULT_Q2;
        block->n_lines = 0;

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
                    case '>': tick_q2 = u32_from_str(c + 1); break;
                    case '~': effect = TEXTBOX_EFFECT_WAVE; break;
                    case '*': effect = TEXTBOX_EFFECT_SHAKE; break;
                    default: BAD_PATH
                    }

                    while (*c != '}') {
                        c++;
                    }
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

            block->line_length[block->n_lines++] = linelength;
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
            } else if (str_eq(action, "shop")) {
                choice->type = TEXTBOX_CHOICE_OPEN_SHOP;
            } else if (str_eq(action, "trigger")) {
                choice->type    = TEXTBOX_CHOICE_TRIGGER;
                choice->trigger = jsonk_i32(jchoice, "trigger");
            }
        }
    }
DIALOG_ERR:
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
        tb->fade_out = 1;
    }
}

void textbox_update(game_s *g, textbox_s *tb, inp_s inp)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;
    tb->tick++;

    if (tb->fade_in) {
        tb->fade_in++;
        if (TEXTBOX_FADE_TICKS <= tb->fade_in) {
            tb->fade_in = 0;
        }
        return;
    }

    if (tb->fade_out) {
        tb->fade_out++;
        if (TEXTBOX_FADE_TICKS <= tb->fade_out) {
            tb->fade_out = 0;
            tb->state    = TEXTBOX_STATE_INACTIVE;
        }
        return;
    }

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {
        textbox_block_s *b = &tb->blocks[tb->block];
        if (inps_just_pressed(inp, INP_A)) {
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
            case TEXTBOX_CHOICE_TRIGGER: {
                tb->fade_out = 1;
                game_on_trigger(g, choice->trigger);
            } break;
            case TEXTBOX_CHOICE_EXIT: {
                tb->fade_out = 1;
            } break;
            case TEXTBOX_CHOICE_OPEN_SHOP: {
                tb->fade_out = 1;
                shop_open(g);
            } break;
            }
            break;
        }

        if (b->n_choices <= 0) break;

        if (inps_just_pressed(inp, INP_DPAD_D) || inps_just_pressed(inp, INP_DPAD_U)) {
            tb->curchoice += inps_dpad_y(inp);
            tb->curchoice = clamp_i(tb->curchoice, 0, b->n_choices - 1);
            snd_play_ext(SNDID_MENU_NEXT_ITEM, 1.f, 1.f);
        }
    } break;
    case TEXTBOX_STATE_WRITE: {
        textbox_block_s *b = &tb->blocks[tb->block];
        assert(tb->n < b->n_chars);

        tb->tick_q2 += inps_pressed(inp, INP_A) || inps_pressed(inp, INP_B) ? 12 : 4;
        int tick = b->chars[tb->n].tick_q2;
        while (tick <= tb->tick_q2) {
            tb->tick_q2 -= tick;
            tb->n++;

            if (b->n_chars <= tb->n) {
                tb->state     = TEXTBOX_STATE_WAIT;
                tb->animation = 0;
                tb->curchoice = 0;
                break;
            }

            int glyph = b->chars[tb->n].glyph;

            if (glyph != ' ') {
                snd_play_ext(SNDID_SPEAK, 0.2f, rngr_f32(.5f, .7f));
            }
        }
    } break;
    }
}

void textbox_draw(textbox_s *tb, v2_i32 camoffset)
{
    if (tb->state == TEXTBOX_STATE_INACTIVE) return;

    textbox_block_s *b   = &tb->blocks[tb->block];
    gfx_ctx_s        ctx = gfx_ctx_display();

#define TB_LINE_SPACING 26
    int num = TEXTBOX_FADE_TICKS;
    if (tb->fade_in) {
        num = tb->fade_in;
    }
    if (tb->fade_out) {
        num = TEXTBOX_FADE_TICKS - tb->fade_out;
    }
    ctx.pat = gfx_pattern_interpolate(num, TEXTBOX_FADE_TICKS);

    gfx_rec_fill(ctx, (rec_i32){0, 150, 400, 90}, PRIM_MODE_BLACK);
    if (tb->state == TEXTBOX_STATE_WAIT && 0 < b->n_choices) {
        gfx_rec_fill(ctx, (rec_i32){220, 70, 200, 100}, PRIM_MODE_BLACK);
    }

    v2_i32 pos = {10, 160};
    switch (b->n_lines) {
    case 1: pos.y += TB_LINE_SPACING; break;
    case 2: pos.y += TB_LINE_SPACING / 2; break;
    }
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
        if (b->line_length[row] <= len) {
            row++;
            len = 0;
            p.x = pos.x;
            p.y += TB_LINE_SPACING;
        }
    }

    switch (tb->state) {
    case TEXTBOX_STATE_WAIT: {
        // arrow bop animation
#define TB_ANIM_TICKS       15
#define TB_ANIM_TICKS_IDLE  20
#define TB_ANIM_TOTAL_TICKS (TB_ANIM_TICKS + TB_ANIM_TICKS_IDLE)

        if (!tb->fade_out) {
            tb->animation = (tb->animation + 1) % TB_ANIM_TOTAL_TICKS;
            int fr        = 0;
            if (TB_ANIM_TICKS_IDLE <= tb->animation) {
                int ti = tb->animation - TB_ANIM_TICKS_IDLE;
                fr     = tick_to_index_freq(ti, 6, TB_ANIM_TICKS);
            }

            texrec_s tarrow = asset_texrec(TEXID_UI, fr * 32, 176, 32, 32);
            if (tb->block == tb->n_blocks - 1)
                tarrow.r.y += 64 + 16;
            gfx_spr(ctx, tarrow, (v2_i32){360, 200}, 0, 0);
        }

        if (b->n_choices <= 0) break;

        for (int n = 0; n < b->n_choices; n++) {
            textbox_choice_s *choice = &b->choices[n];
            v2_i32            pp     = {250, 80 + n * 20};
            for (int i = 0; i < choice->n_chars; i++) {
                int ci = choice->chars[i];
                t.r.x  = fnt.grid_w * (ci & 31);
                t.r.y  = fnt.grid_h * (ci >> 5);
                gfx_spr(ctx, t, pp, 0, SPR_MODE_WHITE);
                pp.x += fnt.widths[ci];
            }
        }

        gfx_rec_fill(ctx, (rec_i32){230, 80 + tb->curchoice * 20, 8, 8}, PRIM_MODE_WHITE);
    } break;
    case TEXTBOX_STATE_WRITE: {

    } break;
    }
}