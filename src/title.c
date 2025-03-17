// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "app.h"
#include "game.h"

#define TITLE_CONFIRM_TICKS 100

// mainmenu state machine
enum {
    TITLE_ST_OVERVIEW, // title screen
    TITLE_ST_SETTINGS,
    TITLE_ST_CREDITS,
    //
    TITLE_ST_FILE_SELECT,
    TITLE_ST_FILE_SELECTED,
    TITLE_ST_FILE_CPY,
    TITLE_ST_FILE_CPY_CONFIRM,
    TITLE_ST_FILE_DEL_CONFIRM,
};

enum {
    TITLE_OV_0_START,
    TITLE_OV_1_SETTINGS,
    TITLE_OV_2_CREDITS
};

enum {
    TITLE_FP_0_SLOT,
    TITLE_FP_1_COPY,
    TITLE_FP_2_DELETE
};

void title_to_state(title_s *t, i32 state, i32 fade);
void title_load(title_s *t);
void title_load_previews(title_s *t);
void title_draw_preview(title_s *t, i32 slot);
void title_draw_companion(title_s *t);
void title_start_game(app_s *app, i32 slot);
void title_draw_confirm(title_s *t);
void title_draw_version();

void title_to_state(title_s *t, i32 state, i32 fade)
{
    t->state_prev = t->state;
    t->state      = state;
    if (fade) {
    }
}

void title_init(title_s *t)
{
    title_load(t);
}

void title_load(title_s *t)
{
    title_load_previews(t);
}

void title_start_game(app_s *app, i32 slot)
{
    app->game.save_slot = slot;
    app->state          = APP_ST_GAME;
    game_load_savefile(&app->game);
}

void title_update(app_s *app, title_s *t)
{
#if TITLE_SKIP_TO_GAME
    title_start_game(app, 0);
    return;
#endif
    t->select_anim++;
    i32 dx = inp_btn_jp(INP_DL) ? -1 : (inp_btn_jp(INP_DR) ? +1 : 0);
    i32 dy = inp_btn_jp(INP_DU) ? -1 : (inp_btn_jp(INP_DD) ? +1 : 0);

    v2_i32 target_q8 = v2_i32_shl(t->pos_comp_target, 8);
    if (v2_i32_distancesq(target_q8, t->pos_comp_q8) < 100000) {
        t->pos_comp_q8 = target_q8;
        t->v_comp_q8.x = 0;
        t->v_comp_q8.y = 0;
    } else {
        t->pos_comp_q8 = v2_i32_add(t->pos_comp_q8, t->v_comp_q8);
        v2_i32 steer   = steer_arrival(t->pos_comp_q8,
                                       t->v_comp_q8,
                                       target_q8,
                                       4000,
                                       32 << 8);
        t->v_comp_q8   = v2_i32_add(t->v_comp_q8, steer);
    }

    switch (t->state) {
    case TITLE_ST_OVERVIEW: {
        if (inp_btn_jp(INP_A)) {
            switch (t->option) {
            case TITLE_OV_0_START: {
                title_to_state(t, TITLE_ST_FILE_SELECT, 0);
                t->option = 0;
                break;
            }
            case TITLE_OV_1_SETTINGS: {
                t->option = 0;
                title_to_state(t, TITLE_ST_SETTINGS, 0);
                break;
            }
            case TITLE_OV_2_CREDITS:
                t->option = 0;
                title_to_state(t, TITLE_ST_CREDITS, 0);
                break;
            }
        } else {
            t->option = clamp_i32((i32)t->option + dy, 0, 2);
        }
        break;
    }
    case TITLE_ST_SETTINGS: {
        if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_OVERVIEW, 0);
        }
        break;
    }
    case TITLE_ST_CREDITS: {
        if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_OVERVIEW, 0);
        }
        break;
    }
    case TITLE_ST_FILE_SELECT: {
        if (inp_btn_jp(INP_A)) {
            t->selected = t->option;
            t->option   = 0;
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
        } else if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_OVERVIEW, 0);
        } else {
            t->option = clamp_i32((i32)t->option + dy, 0, 2);
        }
        break;
    }
    case TITLE_ST_FILE_SELECTED: {
        if (inp_btn_jp(INP_A)) {
            switch (t->option) {
            case TITLE_FP_0_SLOT: {
                title_start_game(app, t->selected);
                break;
            }
            case TITLE_FP_1_COPY: {
                t->option = t->selected == 0 ? 1 : 0;
                title_to_state(t, TITLE_ST_FILE_CPY, 0);
                break;
            }
            case TITLE_FP_2_DELETE: {
                t->option       = 0;
                t->confirm_tick = 0;
                title_to_state(t, TITLE_ST_FILE_DEL_CONFIRM, 0);
                break;
            }
            }
        } else if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
        } else {
            switch (t->option) {
            case TITLE_FP_0_SLOT: {
                if (t->saves[t->selected].exists && 0 < dy) {
                    t->option = TITLE_FP_1_COPY;
                }
                break;
            }
            case TITLE_FP_1_COPY: {
                if (dy < 0) {
                    t->option = TITLE_FP_0_SLOT;
                } else if (0 < dx) {
                    t->option = TITLE_FP_2_DELETE;
                }
                break;
            }
            case TITLE_FP_2_DELETE: {
                if (dy < 0) {
                    t->option = TITLE_FP_0_SLOT;
                } else if (dx < 0) {
                    t->option = TITLE_FP_1_COPY;
                }
                break;
            }
            }
        }
        break;
    }
    case TITLE_ST_FILE_CPY: {
        if (inp_btn_jp(INP_A)) {
            t->copy_to      = t->option;
            t->confirm_tick = 0;
            title_to_state(t, TITLE_ST_FILE_CPY_CONFIRM, 0);
        } else if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
        } else {
            i32 n_option = clamp_i32((i32)t->option + dy, 0, 2);

            // skip middle save if middle save selected
            if (n_option == t->selected) {
                if (t->selected == 1) {
                    n_option += dy;
                    t->option = n_option;
                }
            } else {
                t->option = n_option;
            }
        }
        break;
    }
    case TITLE_ST_FILE_CPY_CONFIRM: {
        if (t->confirm_tick) {
            if (inp_btn(INP_A)) {
                t->confirm_tick++;
                if (TITLE_CONFIRM_TICKS <= t->confirm_tick) {
                    spm_push();
                    savefile_s *s     = spm_alloct(savefile_s);
                    err32       err_r = savefile_r(t->selected, s);
                    if (err_r == 0) {
                        err32 err_w = savefile_w(t->copy_to, s);
                        if (err_w) {

                        } else {
                        }
                    }
                    spm_pop();
                    title_load_previews(t);
                    t->option = 0;
                    title_to_state(t, TITLE_ST_FILE_SELECT, 0);
                }
            } else {
                t->confirm_tick = 0;
            }
        } else if (inp_btn_jp(INP_A)) {
            t->confirm_tick = 1;
        } else if (inp_btn_jp(INP_B)) {
            t->option = t->copy_to;
            title_to_state(t, TITLE_ST_FILE_CPY, 0);
        }

        break;
    }
    case TITLE_ST_FILE_DEL_CONFIRM: {
        if (t->confirm_tick) {
            if (inp_btn(INP_A)) {
                t->confirm_tick++;
                if (TITLE_CONFIRM_TICKS <= t->confirm_tick) {
                    bool32 res = savefile_del(t->selected);
                    if (res) {

                    } else {
                    }
                    title_load_previews(t);
                    t->option = 0;
                    title_to_state(t, TITLE_ST_FILE_SELECT, 0);
                }
            } else {
                t->confirm_tick = 0;
            }
        } else if (inp_btn_jp(INP_A)) {
            t->confirm_tick = 1;
        } else if (inp_btn_jp(INP_B)) {
            t->option = 0;
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
        }
        break;
    }
    default: break;
    }
}

void title_render(title_s *t)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);
    tex_clr(tdisplay, GFX_COL_WHITE);
    fnt_s    font    = asset_fnt(FNTID_LARGE);
    rec_i32  rfull   = {0, 0, 400, 240};
    texrec_s trcover = asset_texrec(TEXID_COVER, 0, 0, 400, 240);

    switch (t->state) {
    case TITLE_ST_OVERVIEW: {
        // gfx_spr(ctx, trcover, (v2_i32){0, 0}, 0, 0);
        fnt_draw_ascii(ctx, font, (v2_i32){20, 40}, "Start", SPR_MODE_BLACK);
        fnt_draw_ascii(ctx, font, (v2_i32){20, 40 + 1 * 40}, "Settings", SPR_MODE_BLACK);
        fnt_draw_ascii(ctx, font, (v2_i32){20, 40 + 2 * 40}, "Credits", SPR_MODE_BLACK);
        title_draw_companion(t);
        t->pos_comp_target.x = 5;
        t->pos_comp_target.y = 40 + t->option * 40;
        break;
    }
    case TITLE_ST_SETTINGS: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 40}, "Settings", SPR_MODE_BLACK);
        break;
    }
    case TITLE_ST_CREDITS: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 40}, "Credits", SPR_MODE_BLACK);
        break;
    }
    case TITLE_ST_FILE_SELECT: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 20}, "Select file", SPR_MODE_BLACK);

        title_draw_companion(t);
        t->pos_comp_target.x = 5;
        t->pos_comp_target.y = 40 + t->option * 40;
        for (i32 n = 0; n < 3; n++) {
            title_draw_preview(t, n);
        }
        break;
    }
    case TITLE_ST_FILE_SELECTED: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 20}, "Start file?", SPR_MODE_BLACK);
        title_draw_preview(t, t->selected);
        if (t->option == 0) {
            title_draw_companion(t);
            t->pos_comp_target.x = 5;
            t->pos_comp_target.y = 40;
        }
        if (t->saves[t->selected].exists) {
            fnt_draw_ascii(ctx, font, (v2_i32){20, 200}, "Copy", SPR_MODE_BLACK);
            fnt_draw_ascii(ctx, font, (v2_i32){100, 200}, "Delete", SPR_MODE_BLACK);
            if (t->option > 0) {
                title_draw_companion(t);
                t->pos_comp_target.x = 5 + (t->option - 1) * 100;
                t->pos_comp_target.y = 10 + 200;
            }
        } else {
            fnt_draw_ascii(ctx, font, (v2_i32){20, 200}, "Start", SPR_MODE_BLACK);
        }
        break;
    }
    case TITLE_ST_FILE_CPY: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 20}, "Copy to?", SPR_MODE_BLACK);

        title_draw_companion(t);
        t->pos_comp_target.x = 5;
        t->pos_comp_target.y = 40 + t->option * 40;
        for (i32 n = 0; n < 3; n++) {
            title_draw_preview(t, n);
        }
        break;
    }
    case TITLE_ST_FILE_CPY_CONFIRM: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 20}, "Confirm copy", SPR_MODE_BLACK);

        title_draw_companion(t);
        t->pos_comp_target.x = 5;
        t->pos_comp_target.y = 40 + t->option * 40;
        for (i32 n = 0; n < 3; n++) {
            title_draw_preview(t, n);
        }
        title_draw_confirm(t);
        break;
    }
    case TITLE_ST_FILE_DEL_CONFIRM: {
        fnt_draw_ascii(ctx, font, (v2_i32){20, 20}, "Confirm delete", SPR_MODE_BLACK);
        title_draw_preview(t, t->selected);
        title_draw_confirm(t);
        break;
    }
    }

    title_draw_version();
}

void title_load_previews(title_s *t)
{
    spm_push();
    savefile_s *s = spm_alloct(savefile_s);

    for (i32 n = 0; n < 3; n++) {
        save_preview_s pr = {0};
        err32          e  = savefile_r(n, s);
        if (e) {
        } else {
            pr.tick   = s->tick;
            pr.exists = 1;
            str_cpys(pr.name, sizeof(pr.name), s->name);
        }
        t->saves[n] = pr;
    }
    spm_pop();
}

void title_draw_preview(title_s *t, i32 slot)
{
    gfx_ctx_s      ctx = gfx_ctx_display();
    fnt_s          f   = asset_fnt(FNTID_20);
    v2_i32         pos = {20, 40 + slot * 40};
    save_preview_s pr  = t->saves[slot];

    if (pr.exists) {
        time_real_s time = time_real_from_ticks(pr.tick);
    } else {
    }

    fnt_draw_ascii(ctx, f, pos, pr.name, SPR_MODE_BLACK);
}

void title_draw_companion(title_s *t)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    i32 frx = ani_frame(ANIID_COMPANION_FLY, t->select_anim);
    i32 fry = 0;
    i32 x   = t->pos_comp_q8.x >> 8;
    i32 y   = t->pos_comp_q8.y >> 8;

    texrec_s tr  = asset_texrec(TEXID_COMPANION, frx * 96, fry * 64, 96, 64);
    v2_i32   pos = {x - 48 + 6, y - 32};
    gfx_spr(ctx, tr, pos, SPR_FLIP_X, 0);
}

void title_draw_confirm(title_s *t)
{

    texrec_s  tr  = asset_texrec(TEXID_BUTTONS, 160, 32, 32, 32);
    gfx_ctx_s ctx = gfx_ctx_display();

    if (inp_btn(INP_A)) {
        tr.x += 32;
    }
    gfx_ctx_s ctxr = ctx;
    ctxr.pat       = gfx_pattern_8x8(B8(10000011),
                                     B8(00000111),
                                     B8(00001110),
                                     B8(00011100),
                                     B8(00111000),
                                     B8(01110000),
                                     B8(11100000),
                                     B8(11000001));
    rec_i32 rr     = {100, 50, 200, 80};
    gfx_rec_fill(ctxr, rr, PRIM_MODE_BLACK_WHITE);
    gfx_spr(ctx, tr, (v2_i32){300, 100}, 0, 0);
}

void title_draw_version()
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    v2_i32    pnum = {4, 240 - 16};
    texrec_s  tnum = asset_texrec(TEXID_BUTTONS, 0, 224, 8, 16);

    // major
    tnum.x = 8 * (GAME_V_MAJ);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 9;
    // dot
    tnum.x = 80;
    gfx_spr_tile_32x32(ctx, tnum, pnum); // "."
    pnum.x += 5;
    // minor
    tnum.x = 8 * (GAME_V_MIN);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 9;
    // dot
    tnum.x = 80;
    gfx_spr_tile_32x32(ctx, tnum, pnum); // "."
    pnum.x += 5;

    // patch
    if (100 <= GAME_V_PAT) {
        tnum.x = 8 * (GAME_V_PAT / 100);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 9;
    }
    if (10 <= GAME_V_PAT) {
        tnum.x = 8 * ((GAME_V_PAT / 10) % 10);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 9;
    }
    tnum.x = 8 * (GAME_V_PAT % 10);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 9;

    if (GAME_V_DEV) {
        // slash
        tnum.x = 88;
        gfx_spr_tile_32x32(ctx, tnum, pnum); // "."
        pnum.x += 9;

        if (100 <= GAME_V_DEV) {
            tnum.x = 8 * (GAME_V_DEV / 100);
            gfx_spr_tile_32x32(ctx, tnum, pnum);
            pnum.x += 9;
        }
        if (10 <= GAME_V_DEV) {
            tnum.x = 8 * ((GAME_V_DEV / 10) % 10);
            gfx_spr_tile_32x32(ctx, tnum, pnum);
            pnum.x += 9;
        }
        tnum.x = 8 * (GAME_V_DEV % 10);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 9;
    }
}