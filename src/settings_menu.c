// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings_menu.h"
#include "game.h"

void settings_menu_navigate(settings_menu_s *sm);

void settings_menu_enter(settings_menu_s *sm)
{
    mclr(sm, sizeof(settings_menu_s));
    err32 err      = settings_load(&sm->settings);
    sm->fade_enter = 1;
    sm->active     = 1;
}

void settings_menu_update(settings_menu_s *sm)
{
    settings_s *s  = &sm->settings;
    i32         dx = inp_btn_jp(INP_DL) ? -1 : (inp_btn_jp(INP_DR) ? +1 : 0);
    i32         dy = inp_btn_jp(INP_DU) ? -1 : (inp_btn_jp(INP_DD) ? +1 : 0);

    if (sm->fade_enter) {
        sm->fade_enter = min_i32((i32)sm->fade_enter + 20, 255);
        if (sm->fade_enter == 255) {
            sm->fade_enter = 0;
        }
    } else if (sm->fade_leave) {
        sm->fade_leave = min_i32((i32)sm->fade_leave + 20, 255);
        if (sm->fade_leave == 255) {
            sm->fade_leave = 0;
        }
    } else {
        switch (sm->opt) {
        case SETTINGS_MENU_OPT_EXIT_CONFIRM: {
            if (inp_btn_jp(INP_B)) { // exit, don't save
                sm->fade_leave = 1;
            } else if (inp_btn_jp(INP_A)) {
                switch (sm->exit_confirm) {
                case 0: { // exit, don't save

                    break;
                }
                case 1: { // exit, save
                    err32 err = settings_save(s);
                    break;
                }
                }
                sm->fade_leave = 1;
            } else if (dx) {
                sm->exit_confirm = clamp_i32((i32)sm->exit_confirm + dx, 0, 1);
            }
            break;
        }
        default: {
            if (inp_btn_jp(INP_B)) {
                sm->opt = SETTINGS_MENU_OPT_EXIT_CONFIRM;
            } else {
                settings_menu_navigate(sm);
            }
            break;
        }
        }
    }
}

void settings_menu_navigate(settings_menu_s *sm)
{
    settings_s *s  = &sm->settings;
    i32         dx = inp_btn_jp(INP_DL) ? -1 : (inp_btn_jp(INP_DR) ? +1 : 0);
    i32         dy = inp_btn_jp(INP_DU) ? -1 : (inp_btn_jp(INP_DD) ? +1 : 0);

    switch (sm->opt) {
    case SETTINGS_MENU_OPT_MODE: {
        if (dy) {
            sm->opt = clamp_i32((i32)sm->opt + dy,
                                0, SETTINGS_MENU_OPT_SAVE_EXIT);
        } else {
            s->mode = ((i32)s->mode + dx + 3) % 3;
        }
        break;
    }
    case SETTINGS_MENU_OPT_VOL_MUS: {
        if (dy) {
            sm->opt = clamp_i32((i32)sm->opt + dy,
                                0, SETTINGS_MENU_OPT_SAVE_EXIT);
        } else {
            s->vol_mus = clamp_i32((i32)s->vol_mus + dx, 0, SETTINGS_VOL_MAX);
        }
        break;
    }
    case SETTINGS_MENU_OPT_VOL_SFX: {
        if (dy) {
            sm->opt = clamp_i32((i32)sm->opt + dy,
                                0, SETTINGS_MENU_OPT_SAVE_EXIT);
        } else {
            s->vol_sfx = clamp_i32((i32)s->vol_sfx + dx, 0, SETTINGS_VOL_MAX);
        }
        break;
    }
    case SETTINGS_MENU_OPT_SAVE_EXIT: {
        if (inp_btn_jp(INP_A)) {
            err32 err      = settings_save(s);
            sm->fade_leave = 1;
        } else if (dy < 0) {
            sm->opt = SETTINGS_MENU_OPT_SAVE_EXIT - 1;
        } else if (0 < dx) {
            sm->opt = SETTINGS_MENU_OPT_RESET;
        }
        break;
    }
    case SETTINGS_MENU_OPT_RESET: {
        if (inp_btn_jp(INP_A)) {
            settings_default(s);
        } else if (dy < 0) {
            sm->opt = SETTINGS_MENU_OPT_SAVE_EXIT - 1;
        } else if (dx < 0) {
            sm->opt = SETTINGS_MENU_OPT_SAVE_EXIT;
        }
        break;
    }
    }
}

#define SETTINGS_MENU_TEXT_Y_SPACING 20
#define SETTINGS_MENU_TEXT_X         20
#define SETTINGS_MENU_TEXT_Y         50

void settings_menu_draw(settings_menu_s *sm)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    fnt_s fnt = asset_fnt(FNTID_MEDIUM);

    tex_clr(ctx.dst, GFX_COL_WHITE);

    v2_i32 p1 = {20, 50};

    fnt_draw_str(ctx, fnt,
                 (v2_i32){SETTINGS_MENU_TEXT_X, SETTINGS_MENU_TEXT_Y},
                 "Volume music", 0);
    fnt_draw_str(ctx, fnt,
                 (v2_i32){SETTINGS_MENU_TEXT_X, SETTINGS_MENU_TEXT_Y + 1 * SETTINGS_MENU_TEXT_Y_SPACING},
                 "Volume sound effects", 0);
    fnt_draw_str(ctx, fnt,
                 (v2_i32){SETTINGS_MENU_TEXT_X, SETTINGS_MENU_TEXT_Y + 2 * SETTINGS_MENU_TEXT_Y_SPACING},
                 "Power saving", 0);

    switch (sm->opt) {
    case SETTINGS_MENU_OPT_EXIT_CONFIRM: {

        break;
    }
    case SETTINGS_MENU_OPT_MODE: {
        break;
    }
    case SETTINGS_MENU_OPT_VOL_MUS: {
        break;
    }
    case SETTINGS_MENU_OPT_VOL_SFX: {
        break;
    }
    case SETTINGS_MENU_OPT_SAVE_EXIT: {
        break;
    }
    case SETTINGS_MENU_OPT_RESET: {
        break;
    }
    }
}