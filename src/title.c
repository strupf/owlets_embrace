// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "app.h"
#include "game.h"

#define TITLE_BTN_Y          181
#define TITLE_BTN_X_1        (50 - 48)
#define TITLE_BTN_X_2        (255 - 48)
#define TITLE_BTN_X_3        (350 - 48)
#define TITLE_SLOT_Y         30
#define TITLE_SLOT_Y_SPACING 50
#define TITLE_MSG_TICK       150

void title_start_game(g_s *g, i32 slot);
void title_to_state(title_s *t, i32 state, i32 fade);
void title_pressed_A(g_s *g, title_s *t);
void title_pressed_B(title_s *t);
void title_navigate(g_s *g, title_s *t, i32 dx, i32 dy);
void title_load(title_s *t);
void title_log(title_s *t, const char *msg);
void title_render_savefile(title_s *t, i32 slot);
void title_render_button(gfx_ctx_s ctx, const char *txt, i32 x, i32 y, i32 frame);
void title_render_file_small_preview(title_s *t, i32 slot, f32 a);
void title_render_file_big_preview(title_s *t, i32 slot, f32 a);
void title_render_copy_arrow(title_s *t, i32 slot_from, i32 slot_to);
void title_render_copy(title_s *t, i32 slot_from, i32 slot_to);
void title_render_textfield(i32 x, i32 y, i32 w, i32 h, const char *txt, bool32 selected, bool32 editing);
void title_render_file_selected(title_s *t, bool32 fade_out);

void title_to_state(title_s *t, i32 state, i32 fade)
{
    t->state_prev = t->state;
    t->state      = state;
    if (fade) {
        t->fade   = 1;
        t->fade_0 = fade;
    }
}

void title_log(title_s *t, const char *msg)
{
    mset(t->msg, 0, sizeof(t->msg));
    str_cpy(t->msg, msg);
    t->msg_tick = TITLE_MSG_TICK;
}

void title_init(title_s *t)
{
#if TITLE_SKIP_TO_GAME || 0
    spm_push();
    save_s *s = spm_alloct(save_s, 1);
    savefile_empty(s);
    str_cpy(s->name, "Lukas");
    str_cpy(s->hero_mapfile, "START");
    s->hero_pos.x       = 200;
    s->hero_pos.y       = 200;
    s->stamina_upgrades = 1;
#if 1
    s->upgrades =
        ((flags32)1 << HERO_UPGRADE_HOOK) |
        ((flags32)1 << HERO_UPGRADE_STOMP) |
        ((flags32)1 << HERO_UPGRADE_DIVE) |
        ((flags32)1 << HERO_UPGRADE_SPRINT) |
        ((flags32)1 << HERO_UPGRADE_SWIM) |
        ((flags32)1 << HERO_UPGRADE_FLY) |
        ((flags32)1 << HERO_UPGRADE_CLIMB) |
        0;
#endif
    savefile_write(0, s);
    spm_pop();
#endif
    title_load(t);
}

void title_load(title_s *t)
{
    spm_push();
    save_s *s = spm_alloct(save_s, 1);

    for (i32 n = 0; n < 3; n++) {
        save_preview_s pr = {0};

        if (savefile_read(n, s)) {
            pr.tick = s->tick;
            str_cpy(pr.name, s->name);
        }
        t->saves[n] = pr;
    }
    spm_pop();
}

void title_start_game(g_s *g, i32 slot)
{
    g->save_slot = slot;
    g->state     = APP_STATE_GAME;
    savefile_read(slot, &g->save);
    game_load_savefile(g);
}

void title_update(g_s *g, title_s *t)
{
#if TITLE_SKIP_TO_GAME
    title_start_game(g, 0);
    return;

#endif
    t->timer++;
    if (t->msg_tick) {
        t->msg_tick--;
    }

    if (t->fade) {
        t->fade++;

        if (t->fade_0 <= t->fade) {
            t->fade = 0;
        } else {
            return;
        }
    }

    switch (t->state) {
    case TITLE_ST_FILE_START: {
        if (t->fade != 0) return;
        title_start_game(g, t->selected);
        return;
    }
    default: break;
    }

    i32 optionp = t->option;
    i32 statep  = t->state;

    if (inp_action_jp(INP_B)) { // back or abort
        title_pressed_B(t);
    } else if (inp_action_jp(INP_A)) { // select or confirm
        title_pressed_A(g, t);
    } else {
        i32 dx = 0;
        i32 dy = 0;
        if (inp_action_jp(INP_DL)) dx = -1;
        if (inp_action_jp(INP_DR)) dx = +1;
        if (inp_action_jp(INP_DU)) dy = -1;
        if (inp_action_jp(INP_DD)) dy = +1;

        if (dx | dy) {
            title_navigate(g, t, dx, dy);
        }
    }

    if (t->state == statep) {
        t->state_tick++;
    } else {
        t->state_tick = 0;
    }

    if (t->option != optionp || t->state != statep) {
        t->timer = 0;
    }
}

void title_pressed_A(g_s *g, title_s *t)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START: {
        t->state  = TITLE_ST_FILE_SELECT;
        t->option = 0;
        break;
    }
    case TITLE_ST_FILE_SELECT:
        switch (t->option) {
        case 0:
        case 1:
        case 2:
            title_to_state(t, TITLE_ST_FILE_SELECTED, 4);
            t->selected = t->option;
            t->option   = 0;
            break;
        }
        break;
    case TITLE_ST_FILE_SELECTED: {
        switch (t->option) {
        case TITLE_F_START:
            if (t->saves[t->selected].health) {
                title_to_state(t, TITLE_ST_FILE_START, 60);
            } else {
                title_to_state(t, TITLE_ST_FILE_NEW, 0);
                t->option = 0;
                t->tinput = (textinput_s){0};
            }
            break;
        case TITLE_F_CPY:
            title_to_state(t, TITLE_ST_FILE_CPY, 0);
            t->option  = (t->selected + 1) % 3;
            t->copy_to = t->option;
            break;
        case TITLE_F_DEL:
            title_to_state(t, TITLE_ST_FILE_DEL_CONFIRM, 0);
            t->option = 0;
            break;
        }
        break;
    }
    case TITLE_ST_FILE_CPY: {
        t->copy_to = t->option;
        if (t->saves[t->copy_to].health) {
            title_to_state(t, TITLE_ST_FILE_CPY_CONFIRM, 0);
            t->option = 0;
        } else {
            if (!savefile_cpy(t->selected, t->copy_to)) {
                title_log(t, "Couldn't copy savefile!");
            }
            title_load(t);
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            t->option = 0;
        }
        break;
    }
    case TITLE_ST_FILE_CPY_CONFIRM: {
        switch (t->option) {
        case 0: // no
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
            t->option = TITLE_F_CPY;
            break;
        case 1: // yes
            if (!savefile_cpy(t->selected, t->copy_to)) {
                title_log(t, "Couldn't copy savefile!");
            }
            title_load(t);
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            t->option = 0;
            break;
        }
        break;
    }
    case TITLE_ST_FILE_DEL_CONFIRM: {
        switch (t->option) {
        case 0: // no
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
            t->option = TITLE_F_DEL;
            break;
        case 1: // yes
            if (!savefile_del(t->selected)) {
                title_log(t, "Couldn't delete savefile!");
            }
            title_load(t);
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            t->option = 0;
            break;
        }
        break;
    }
    case TITLE_ST_OPTIONS: {
        break;
    }
    case TITLE_ST_FILE_NEW: {
        switch (t->option) {
        case 0: // edit text
            t->tinput.cap = 16;
            textinput_activate(&t->tinput, 1);
            break;
        case 1: // "ok"
            if (t->tinput.n == 0) {
                break;
            }
            spm_push();
            save_s *s = spm_alloctz(save_s, 1);
            savefile_empty(s);

            mcpy(s->name, t->tinput.c, t->tinput.n + 1);

            str_cpy(s->hero_mapfile, "L_0");
            s->hero_pos.x = 100;
            s->hero_pos.y = 500;
            s->upgrades =
                ((flags32)1 << HERO_UPGRADE_SPRINT) |
                ((flags32)1 << HERO_UPGRADE_FLY) |
                ((flags32)1 << HERO_UPGRADE_HOOK) |
                ((flags32)1 << HERO_UPGRADE_SWIM) |
                ((flags32)1 << HERO_UPGRADE_DIVE);

            savefile_write(t->selected, s);
            spm_pop();     // fallthrough
            title_load(t); // update savefiles
        case 2:            // "cancel"
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            t->option = t->selected;
            break;
        }
        break;
    }
    default: break;
    }
}

void title_pressed_B(title_s *t)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START:
        break;
    case TITLE_ST_FILE_SELECT:
        t->state  = TITLE_ST_PRESS_START;
        t->option = 0;
        break;
    case TITLE_ST_FILE_SELECTED:
        t->state  = TITLE_ST_FILE_SELECT;
        t->option = t->selected;
        t->fade   = 0;
        break;
    case TITLE_ST_FILE_DEL_CONFIRM:
        switch (t->option) {
        case 0: // no
            title_to_state(t, TITLE_ST_FILE_SELECTED, 10);
            t->option = TITLE_F_DEL;
            break;
        case 1: // yes
            t->option = 0;
            break;
        }
        break;
    case TITLE_ST_FILE_CPY_CONFIRM:
        switch (t->option) {
        case 0: // no
            title_to_state(t, TITLE_ST_FILE_SELECTED, 10);
            t->option = TITLE_F_CPY;
            break;
        case 1: // yes
            t->option = 0;
            break;
        }
        break;
    case TITLE_ST_FILE_CPY:
        title_to_state(t, TITLE_ST_FILE_SELECTED, 10);
        t->option = TITLE_F_CPY;
        break;
    case TITLE_ST_FILE_NEW: {
        switch (t->option) {
        case 0:
        case 1:
            t->option = 2;
            break;
        case 2:
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            t->option = t->selected;
            break;
        }
        break;
    }
    default: break;
    }
}

void title_navigate(g_s *g, title_s *t, i32 dx, i32 dy)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START: {
        break;
    }
    case TITLE_ST_FILE_SELECT:
        t->option = clamp_i32(t->option + dy, 0, 3);
        break;
    case TITLE_ST_FILE_SELECTED:
        if (t->saves[t->selected].health) {
            t->option = clamp_i32(t->option + dx, 0, 2);
        } else {
            t->option = 0;
        }
        break;
    case TITLE_ST_FILE_CPY:
        t->option = clamp_i32(t->option + dy, 0, 2);
        switch (t->selected) {
        case 0: t->option = max_i32(t->option, 1); break;
        case 2: t->option = min_i32(t->option, 1); break;
        case 1:
            if (t->option == 1) {
                t->option += dy;
            }
            break;
        }
        t->copy_to = t->option;
        break;
    case TITLE_ST_FILE_CPY_CONFIRM:
    case TITLE_ST_FILE_DEL_CONFIRM:
        t->option = clamp_i32(t->option - dx, 0, 1);
        break;
    case TITLE_ST_OPTIONS: {
        break;
    }
    case TITLE_ST_FILE_NEW: {
        switch (t->option) {
        case 0:
            t->option = clamp_i32(t->option + dy, 0, 1);
            break;
        case 1:
        case 2:
            if (dy == -1) {
                t->option = 0;
            } else {
                t->option = clamp_i32(t->option + dx, 1, 2);
            }
            break;
        }
        break;
    }
    default: break;
    }
}

void title_render(title_s *t)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));
    tex_clr(asset_tex(0), GFX_COL_WHITE);
    fnt_s   font  = asset_fnt(FNTID_LARGE);
    rec_i32 rfull = {0, 0, 400, 240};

    switch (t->state) {
    case TITLE_ST_PRESS_START: {
#define PRESS_START_TXT   "- Press A -"
#define PRESS_START_TICKS 40

        gfx_ctx_s ctx_start = ctx;

        i32 b = t->state_tick % (PRESS_START_TICKS * 2);
        if (b > PRESS_START_TICKS) {
            b = 2 * PRESS_START_TICKS - b;
        }

        i32 fade_i    = 100;
        i32 fade_j    = min_i32((100 * b + (PRESS_START_TICKS / 2)) / PRESS_START_TICKS, fade_i);
        fade_j        = min_i32(fade_j * 2, 100);
        ctx_start.pat = gfx_pattern_interpolate(pow2_i32(fade_j), pow2_i32(100));

        i32 strl = fnt_length_px(font, PRESS_START_TXT);
        fnt_draw_ascii(ctx_start, font,
                       (v2_i32){200 - strl / 2, 200},
                       PRESS_START_TXT, SPR_MODE_BLACK);
        break;
    }
    case TITLE_ST_FILE_CPY:
    case TITLE_ST_FILE_CPY_CONFIRM: {
        title_render_copy(t, t->selected, t->copy_to);
        if (t->state == TITLE_ST_FILE_CPY) break;

        gfx_ctx_s ctxr = ctx;
        ctxr.pat       = gfx_pattern_interpolate(1, 4);
        gfx_rec_fill(ctxr, rfull, GFX_COL_WHITE);

        i32 btn_y = TITLE_BTN_Y + (t->fade ? lerp_i32(100, 0, t->fade, t->fade_0) : 0);

        title_render_button(ctx,
                            "Copy",
                            TITLE_BTN_X_1, btn_y,
                            t->option == 1 ? t->timer : 0); // yes
        title_render_button(ctx,
                            "Cancel",
                            TITLE_BTN_X_2, btn_y,
                            t->option == 0 ? t->timer : 0); // no
        break;
    }
    case TITLE_ST_FILE_DEL_CONFIRM: {
        for (i32 n = 0; n < 3; n++) {
            f32 a = (n == t->selected ? 1.0f : 0.5f);
            title_render_file_small_preview(t, n, a);
        }

        i32 btn_y = TITLE_BTN_Y + (t->fade ? lerp_i32(100, 0, t->fade, t->fade_0) : 0);

        title_render_button(ctx,
                            "Delete",
                            TITLE_BTN_X_1, btn_y,
                            t->option == 1 ? t->timer : 0); // yes
        title_render_button(ctx,
                            "Cancel",
                            TITLE_BTN_X_2, btn_y,
                            t->option == 0 ? t->timer : 0); // no
        break;
    }
    case TITLE_ST_FILE_SELECT: {
        for (i32 n = 0; n < 3; n++) {
            title_render_file_small_preview(t, n, 1.0f);
        }
        rec_i32 rselection = {
            10,
            TITLE_SLOT_Y + t->option * TITLE_SLOT_Y_SPACING,
            10,
            10};
        gfx_rec_fill(ctx, rselection, GFX_COL_BLACK);
        break;
    }
    case TITLE_ST_FILE_SELECTED: {
        title_render_file_selected(t, 0);
        break;
    }
    case TITLE_ST_FILE_NEW: {
        fnt_draw_ascii(ctx, font, (v2_i32){50, 50}, "What is your name?", GFX_COL_BLACK);
        i32    offs    = textinput_draw_offs();
        i32    wi      = 400 - offs;
        i32    tw      = 160;
        bool32 editing = textinput_active();
        title_render_textfield((wi - tw) / 2, 100, tw, 28, t->tinput.c, t->option == 0, editing);
        title_render_button(ctx,
                            "OK",
                            TITLE_BTN_X_1, TITLE_BTN_Y,
                            t->option == 1 ? t->timer : 0);
        title_render_button(ctx,
                            "Cancel",
                            TITLE_BTN_X_3, TITLE_BTN_Y,
                            t->option == 2 ? t->timer : 0);
        break;
    }
    case TITLE_ST_FILE_START: {
        title_render_file_selected(t, 1);
        gfx_ctx_s ctxf = ctx;
        i32       f    = min_i32(t->fade * 2, t->fade_0);
        ctxf.pat       = gfx_pattern_interpolate(f, t->fade_0);
        gfx_rec_fill(ctxf, rfull, GFX_COL_BLACK);
        break;
    }
    }

    if (t->msg_tick) {
        fnt_s  fnt    = asset_fnt(FNTID_LARGE);
        v2_i32 msgpos = {10, 200};
        fnt_draw_ascii(ctx, fnt, msgpos, t->msg, SPR_MODE_BLACK);
    }
}

void title_render_savefile(title_s *t, i32 slot)
{
    save_preview_s *pr = &t->saves[slot];
}

void title_render_button(gfx_ctx_s ctx, const char *txt, i32 x, i32 y, i32 frame)
{
    texrec_s trbut = asset_texrec(TEXID_TITLE_SCREEN, 0, 64, 96, 64);
    fnt_s    fnt   = asset_fnt(FNTID_LARGE);

    rec_i32 rr   = {x + 10, y + 18, 78, 26};
    i32     len  = fnt_length_px(fnt, txt);
    v2_i32  txtp = {x + trbut.r.w / 2 - len / 2, y + 23};

    if (frame) {
        gfx_ctx_s ctxr = ctx;
        ctxr.pat       = gfx_pattern_interpolate(1, 2);
        gfx_rec_fill(ctxr, rr, PRIM_MODE_BLACK_WHITE);
        if (frame < 30) { // blink twice
            gfx_ctx_s ctxl = gfx_ctx_clipr(ctx, rr);
            i32       xm   = frame * 14;
            v2_i32    p1   = {x + xm - 128, y - 64};
            v2_i32    p2   = {x + xm, y + 64};
            v2_i32    p3   = p1;
            v2_i32    p4   = p2;
            p3.x -= 20;
            p4.x -= 20;
            gfx_lin_thick(ctxl, p1, p2, GFX_COL_WHITE, 12);
            gfx_lin_thick(ctxl, p3, p4, GFX_COL_WHITE, 4);
        }

    } else {
        gfx_rec_fill(ctx, rr, GFX_COL_WHITE);
    }

    gfx_spr(ctx, trbut, (v2_i32){x, y}, 0, 0);

    if (frame) {
        for (i32 yy = -2; yy <= +2; yy++) {
            for (i32 xx = -2; xx <= +2; xx++) {
                v2_i32 pp = {txtp.x + xx, txtp.y + yy};
                fnt_draw_ascii(ctx, fnt, pp, txt, SPR_MODE_WHITE);
            }
        }
    }

    fnt_draw_ascii(ctx, fnt, txtp, txt, SPR_MODE_BLACK);
}

void title_render_copy(title_s *t, i32 slot_from, i32 slot_to)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    for (i32 n = 0; n < 3; n++) {
        f32 a = (n == slot_from ? 1.0f : 0.5f);
        title_render_file_small_preview(t, n, a);
    }
    title_render_copy_arrow(t, slot_from, slot_to);
}

void title_render_file_small_preview(title_s *t, i32 slot, f32 a)
{
    assert(0 <= slot && slot <= 2);
    save_preview_s *sp = &t->saves[slot];

    fnt_s     fnt  = asset_fnt(FNTID_LARGE);
    fnt_s     fnts = asset_fnt(FNTID_SMALL);
    gfx_ctx_s ctx  = gfx_ctx_display();
    ctx.pat        = gfx_pattern_interpolate((i32)(a * 256.5f), 256);

    i32 y_anchor = TITLE_SLOT_Y + slot * TITLE_SLOT_Y_SPACING;

    texrec_s trframe  = asset_texrec(TEXID_TITLE_SCREEN, 256, 16, 16 * 7, 16 * 4);
    v2_i32   framepos = {60, y_anchor};
    gfx_spr(ctx, trframe, framepos, 0, 0);

    v2_i32 namepos = {70, y_anchor + 10};
    v2_i32 timep   = {90, y_anchor + 30};

    time_real_s timer       = time_real_from_ticks(sp->tick);
    char        timestr[16] = {0};

    timestr[0] = 99 < timer.h ? '0' + (timer.h / 100) % 10 : ' ';
    timestr[1] = 9 < timer.h ? '0' + (timer.h / 10) % 10 : ' ';
    timestr[2] = timer.h ? '0' + timer.h % 10 : ' ';
    timestr[3] = timer.h ? 'h' : ' ';
    timestr[4] = ' ';
    timestr[5] = 0 < timer.h || 9 < timer.m ? '0' + (timer.m / 10) % 10 : ' ';
    timestr[6] = '0' + timer.m % 10;
    timestr[7] = 'm';

    fnt_draw_ascii(ctx, fnt, namepos, sp->name, SPR_MODE_BLACK);
    fnt_draw_ascii_mono(ctx, fnts, timep, timestr, SPR_MODE_BLACK, 9);
}

void title_render_file_big_preview(title_s *t, i32 slot, f32 a)
{
    assert(0 <= slot && slot <= 2);
    save_preview_s *sp  = &t->saves[slot];
    fnt_s           fnt = asset_fnt(FNTID_LARGE);
    gfx_ctx_s       ctx = gfx_ctx_display();
    ctx.pat             = gfx_pattern_interpolate((i32)(a * 256.5f), 256);
}

void title_render_copy_arrow(title_s *t, i32 slot_from, i32 slot_to)
{
#define TITLE_ARROW_THICK   8  // arrow thickness
#define TITLE_ARROW_X       10 // position X
#define TITLE_ARROW_STRIPE1 12 // length of black dots
#define TITLE_ARROW_STRIPE2 16 // distance of black dots (begin to begin)

    gfx_ctx_s ctx      = gfx_ctx_display();
    gfx_ctx_s ctx_clip = ctx;

    i32 arr_dir = sgn_i32(slot_to - slot_from); // vertical arrow sign
    i32 roff    = (t->state_tick & 15);         // animation dot offset
    i32 clip_y1 = TITLE_SLOT_Y + slot_from * TITLE_SLOT_Y_SPACING;
    i32 clip_y2 = TITLE_SLOT_Y + slot_to * TITLE_SLOT_Y_SPACING;

    if (clip_y2 < clip_y1) {
        SWAP(i32, clip_y1, clip_y2);
    }

    ctx_clip.clip_x1 = TITLE_ARROW_X;
    ctx_clip.clip_x2 = TITLE_ARROW_X + 25; // horizontal arrow length
    ctx_clip.clip_y1 = clip_y1 + 2;
    ctx_clip.clip_y2 = clip_y2 + 2 + TITLE_ARROW_THICK - 1;

    i32 rypos1 = ctx_clip.clip_y1;
    i32 rypos2 = ctx_clip.clip_y2 + 1 - TITLE_ARROW_THICK;

    // static arrow corner dots
    rec_i32 rhor1 = {TITLE_ARROW_X,
                     rypos1,
                     TITLE_ARROW_THICK, TITLE_ARROW_THICK};
    rec_i32 rhor2 = {TITLE_ARROW_X,
                     rypos2,
                     TITLE_ARROW_THICK, TITLE_ARROW_THICK};

    gfx_rec_rounded_fill(ctx_clip, rhor1, 3, GFX_COL_BLACK);
    gfx_rec_rounded_fill(ctx_clip, rhor2, 3, GFX_COL_BLACK);

    // vertical dynamic arrow (going up or down)
    gfx_ctx_s ctx_clip_ver = ctx_clip; // clip some more to avoid visual glitches
    ctx_clip_ver.clip_y1 += 3;
    ctx_clip_ver.clip_y2 -= 3;
    for (i32 yy = -10; yy < 10; yy++) {
        rec_i32 rver = {TITLE_ARROW_X,
                        arr_dir * (yy * TITLE_ARROW_STRIPE2 + roff),
                        TITLE_ARROW_THICK, TITLE_ARROW_STRIPE1};
        gfx_rec_rounded_fill(ctx_clip_ver, rver, 3, GFX_COL_BLACK);
    }

    // horizontal dynamic arrows (one going left, other going right)
    for (i32 hord = -1; hord <= +1; hord += 2) {
        for (i32 xx = -3; xx < +3; xx++) {
            rec_i32 rver = {TITLE_ARROW_X + hord * (xx * TITLE_ARROW_STRIPE2 + roff),
                            arr_dir * hord < 0 ? rypos1 : rypos2,
                            TITLE_ARROW_STRIPE1, TITLE_ARROW_THICK};
            rver.x       = max_i32(rver.x, TITLE_ARROW_X);
            gfx_rec_rounded_fill(ctx_clip, rver, 3, GFX_COL_BLACK);
        }
    }

    // "copy to" sprite end of arrow
    texrec_s trarrow  = asset_texrec(TEXID_TITLE_SCREEN, 160, 128, 32, 32);
    v2_i32   arrowpos = {TITLE_ARROW_X + 15 - (t->state_tick >> 3) % 3,
                         (arr_dir < 0 ? ctx_clip.clip_y1 : ctx_clip.clip_y2 + 1) - 16 -
                             arr_dir * TITLE_ARROW_THICK / 2};
    gfx_spr(ctx, trarrow, arrowpos, 0, 0);
}

void title_render_textfield(i32 x, i32 y, i32 w, i32 h, const char *txt, bool32 selected, bool32 editing)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    rec_i32   r   = {x, y, w, h + 1};

    gfx_rec_rounded_fill(ctx, r, -1, GFX_COL_BLACK);

    fnt_s  fnt  = asset_fnt(FNTID_LARGE);
    v2_i32 spos = {x + 12, y + 6};

    fnt_draw_ascii(ctx, fnt, spos, txt, SPR_MODE_WHITE);
    static u32 tick = 0;
    tick++;
    if (editing && ((tick >> 3) & 1)) {
        i32     strl    = fnt_length_px(fnt, txt);
        rec_i32 rcursor = {spos.x + strl + 2, spos.y, 4, 18};
        gfx_rec_fill(ctx, rcursor, GFX_COL_WHITE);
    }

    if (selected && !editing) {
        rec_i32   rsmall = {r.x + 3, r.y + 3, r.w - 6, r.h - 6};
        gfx_ctx_s ctxr   = ctx;
        i32       fr     = frame_from_ticks_pingpong(tick >> 1, 256);
        ctxr.pat         = gfx_pattern_interpolate(fr, 512);
        gfx_rec_rounded_fill(ctxr, rsmall, -1, GFX_COL_WHITE);
    }
}

void title_render_file_selected(title_s *t, bool32 fade_out)
{
    gfx_ctx_s ctx          = gfx_ctx_display();
    gfx_ctx_s ctx_start    = ctx;
    gfx_ctx_s ctx_copy_del = ctx;

    i32 blerp_q8     = (t->fade && fade_out ? lerp_i32(0, 256, t->fade, t->fade_0) : 256);
    ctx_start.pat    = gfx_pattern_interpolate(blerp_q8, 256);
    ctx_copy_del.pat = gfx_pattern_interpolate(blerp_q8, 256);

    if (!t->saves[t->selected].health) {
        ctx_copy_del.pat = gfx_pattern_interpolate(min_i32(128, blerp_q8), 256);
    }

    title_render_button(ctx_start,
                        "Start",
                        TITLE_BTN_X_1, TITLE_BTN_Y,
                        t->option == 0 ? t->timer : 0);
    title_render_button(ctx_copy_del,
                        "Copy",
                        TITLE_BTN_X_2, TITLE_BTN_Y,
                        t->option == 1 ? t->timer : 0);
    title_render_button(ctx_copy_del,
                        "Delete",
                        TITLE_BTN_X_3, TITLE_BTN_Y,
                        t->option == 2 ? t->timer : 0);

    for (i32 n = 0; n < 3; n++) {
        f32 a = (n == t->selected ? 1.0f : 0.5f);
        title_render_file_small_preview(t, n, a);
    }
}