// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "mainmenu.h"
#include "game.h"
#include "sys/sys.h"

enum {
    MAINMENU_SOUND_BUTTON,
    MAINMENU_SOUND_START,
    MAINMENU_SOUND_PROMPT,
    MAINMENU_SOUND_DELETE,
    MAINMENU_SOUND_COPY,
    MAINMENU_SOUND_ABORT,
    MAINMENU_SOUND_NAVIGATE,
};

static void mainmenu_pressed_A(game_s *g, mainmenu_s *t);
static void mainmenu_pressed_B(mainmenu_s *t);
static void mainmenu_navigate(game_s *g, mainmenu_s *t, int dx, int dy);
static void mainmenu_op_start_file(game_s *g, mainmenu_s *t, int index);
static void mainmenu_play_sound(int soundID);
static void mainmenu_update_savefiles(mainmenu_s *t);
static void mainmenu_render_save_slots(mainmenu_s *t, gfx_ctx_s ctx);
static void mainmenu_render_yes_no(mainmenu_s *t, gfx_ctx_s ctx);

void mainmenu_init(mainmenu_s *t)
{
#if 1
    // write some files to debug copy and delete
    savefile_s *sf = &t->savefiles[0].sf;
#if 0
    sf->upgrades[HERO_UPGRADE_HOOK]      = 1;
    sf->upgrades[HERO_UPGRADE_WHIP]      = 1;
    sf->upgrades[HERO_UPGRADE_HIGH_JUMP] = 1;
    sf->upgrades[HERO_UPGRADE_LONG_HOOK] = 1;
    sf->upgrades[HERO_UPGRADE_WALLJUMP]  = 1;
#endif
    sf->upgrades[HERO_UPGRADE_LONG_HOOK] = 1;
    sf->upgrades[HERO_UPGRADE_WHIP]      = 1;
    sf->health                           = 5;
    str_cpys(t->savefiles[0].sf.hero_name, sizeof(t->savefiles[0].sf.hero_name), "Link");
    str_cpys(t->savefiles[1].sf.hero_name, sizeof(t->savefiles[1].sf.hero_name), "Mario");
    str_cpys(t->savefiles[2].sf.hero_name, sizeof(t->savefiles[2].sf.hero_name), "Samus");

    strcpy(t->savefiles[0].sf.area_filename, "Level_0");
    savefile_write(0, &t->savefiles[0].sf);
    savefile_write(1, &t->savefiles[1].sf);
    savefile_write(2, &t->savefiles[2].sf);
#endif
    mainmenu_update_savefiles(t);
}

void mainmenu_update(game_s *g, mainmenu_s *t)
{
#if MAINMENU_SKIP_TO_GAME
    mainmenu_op_start_file(g, t, 0);
    return;
#endif

    t->title_blink++;

    // update feather animation
    t->feather_time += 0.05f;
    float s1 = sin_f(t->feather_time);
    t->feather_time += (1.f - ABS(s1)) * 0.05f;
    t->feather_y += 0.6f;

    if (t->feather_y >= 500.f) t->feather_y = 0;

    if (t->fade_to_game) {
        t->fade_to_game++;
        if (t->fade_to_game < (FADETICKS_MM_GAME + FADETICKS_MM_GAME_BLACK)) return;
        t->fade_to_game = 0;
        mainmenu_op_start_file(g, t, t->option);
        g->mainmenu_fade_in = FADETICKS_GAME_IN;
        return;
    }

    if (inp_just_pressed(INP_B)) { // back or abort
        mainmenu_pressed_B(t);
        return;
    }

    if (inp_just_pressed(INP_A)) { // select or confirm
        mainmenu_pressed_A(g, t);
        return;
    }

    int dx = 0;
    int dy = 0;
    if (inp_just_pressed(INP_DPAD_L)) dx = -1;
    if (inp_just_pressed(INP_DPAD_R)) dx = +1;
    if (inp_just_pressed(INP_DPAD_U)) dy = -1;
    if (inp_just_pressed(INP_DPAD_D)) dy = +1;

    if (dx != 0 || dy != 0) {
        mainmenu_navigate(g, t, dx, dy);
    }
}

static void cb_mainmenu_to_fileselect(void *arg)
{
    mainmenu_s *t = (mainmenu_s *)arg;
    t->state      = MAINMENU_ST_FILESELECT;
    t->option     = 0;
}

static void cb_mainmenu_to_press_start(void *arg)
{
    mainmenu_s *t  = (mainmenu_s *)arg;
    t->state       = MAINMENU_ST_PRESS_START;
    t->title_blink = 0;
}

static void cb_load_game_debug(void *arg)
{
    game_s     *g = (game_s *)arg;
    mainmenu_s *t = &g->mainmenu;
    mainmenu_op_start_file(g, t, 0);
}

static void mainmenu_pressed_A(game_s *g, mainmenu_s *t)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
#if 0
        cb_load_game_debug(g);
#else
        mainmenu_op_start_file(g, t, 0);
        // cb_mainmenu_to_fileselect(g);
#endif
        break;
        //
    case MAINMENU_ST_FILESELECT:
        switch (t->option) {
        case 0:
        case 1:
        case 2: // selected file to play
            t->fade_to_game = 1;
            mainmenu_play_sound(MAINMENU_SOUND_START);
            // mainmenu_op_start_file(g, t, t->option);
            break;
        case MAINMENU_OPTION_COPY: // copy file
            mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
            t->state  = MAINMENU_ST_COPY_SELECT_FROM;
            t->option = 0;
            break;
        case MAINMENU_OPTION_DELETE: // delete file
            mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
            t->state  = MAINMENU_ST_DELETE_SELECT;
            t->option = 0;
            break;
        }
        break;
        //
    case MAINMENU_ST_COPY_SELECT_FROM:
        mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
        t->state             = MAINMENU_ST_COPY_SELECT_TO;
        t->file_to_copy_from = t->option;
        t->file_to_copy_to   = -1;
        switch (t->file_to_copy_from) {
        case 0: t->option = 1; break;
        case 1:
        case 2: t->option = 0; break;
        }
        break;
        //
    case MAINMENU_ST_COPY_SELECT_TO:
        mainmenu_play_sound(MAINMENU_SOUND_PROMPT);
        t->state           = MAINMENU_ST_COPY_NO_OR_YES;
        t->file_to_copy_to = t->option;
        t->option          = MAINMENU_OPTION_NO;
        break;
        //
    case MAINMENU_ST_COPY_NO_OR_YES:
        switch (t->option) {
        case MAINMENU_OPTION_NO:
            mainmenu_play_sound(MAINMENU_SOUND_ABORT);
            break;
        case MAINMENU_OPTION_YES:
            mainmenu_play_sound(MAINMENU_SOUND_COPY);
            savefile_copy(t->file_to_copy_from, t->file_to_copy_to);
            mainmenu_update_savefiles(t);
            break;
        }
        t->state  = MAINMENU_ST_FILESELECT;
        t->option = 0;
        break;
        //
    case MAINMENU_ST_DELETE_SELECT:
        mainmenu_play_sound(MAINMENU_SOUND_PROMPT);
        t->state          = MAINMENU_ST_DELETE_NO_OR_YES;
        t->file_to_delete = t->option;
        t->option         = MAINMENU_OPTION_NO;
        break;
        //
    case MAINMENU_ST_DELETE_NO_OR_YES:
        switch (t->option) {
        case MAINMENU_OPTION_NO:
            mainmenu_play_sound(MAINMENU_SOUND_ABORT);
            break;
        case MAINMENU_OPTION_YES:
            mainmenu_play_sound(MAINMENU_SOUND_DELETE);
            savefile_delete(t->file_to_delete);
            mainmenu_update_savefiles(t);
            break;
        }
        t->state  = MAINMENU_ST_FILESELECT;
        t->option = 0;
        break;
    }
}

static void mainmenu_pressed_B(mainmenu_s *t)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        break;
    case MAINMENU_ST_FILESELECT:
        cb_mainmenu_to_press_start(t);
        break;
    default:
        mainmenu_play_sound(MAINMENU_SOUND_ABORT);
        t->state  = MAINMENU_ST_FILESELECT;
        t->option = 0;
        break;
    }
}

static void mainmenu_navigate(game_s *g, mainmenu_s *t, int dx, int dy)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        break;
        //
    case MAINMENU_ST_FILESELECT:
        switch (t->option) {
        case 0:
        case 1:
        case 2:
            t->option += dy;
            t->option = max_i(t->option, 0);
            if (t->option > 2)
                t->option = MAINMENU_OPTION_COPY;
            break;
        case MAINMENU_OPTION_COPY:
            if (dx > 0) t->option = MAINMENU_OPTION_DELETE;
            if (dy < 0) t->option = 2;
        case MAINMENU_OPTION_DELETE:
            if (dx < 0) t->option = MAINMENU_OPTION_COPY;
            if (dy < 0) t->option = 2;
            break;
        }
        break;
        //
    case MAINMENU_ST_COPY_SELECT_FROM:
        //
    case MAINMENU_ST_DELETE_SELECT:
        t->option = clamp_i(t->option + dy, 0, 2);
        break;
        //
    case MAINMENU_ST_COPY_SELECT_TO:
        t->option += dy;
        t->option = clamp_i(t->option, 0, 2);
        switch (t->file_to_copy_from) {
        case 0:
        case 2:
            if (t->option == t->file_to_copy_from) t->option = 1;
            break;
        case 1:
            if (t->option == 1) {
                if (dy > 0) t->option = 2;
                if (dy < 0) t->option = 0;
            }
            break;
        }
        break;
    case MAINMENU_ST_COPY_NO_OR_YES: // yes/no prompt for copy/delete
                                     //
    case MAINMENU_ST_DELETE_NO_OR_YES:
        switch (t->option) {
        case MAINMENU_OPTION_NO:
            if (dx > 0) t->option = MAINMENU_OPTION_YES;
            break;
        case MAINMENU_OPTION_YES:
            if (dx < 0) t->option = MAINMENU_OPTION_NO;
            break;
        }
        break;
        //
    default:
        BAD_PATH
        break;
    }
}

static void mainmenu_op_start_file(game_s *g, mainmenu_s *t, int index)
{
    mainmenu_play_sound(MAINMENU_SOUND_START);
    if (t->savefiles[index].exists) {
        game_load_savefile(g, g->mainmenu.savefiles[index].sf, index);
    } else {
        game_new_savefile(g, index);
    }

    g->state = GAMESTATE_GAMEPLAY;
}

static void mainmenu_play_sound(int soundID)
{
}

static void mainmenu_update_savefiles(mainmenu_s *t)
{
    for (int i = 0; i < 3; i++) {
        t->savefiles[i].exists = savefile_read(i, &t->savefiles[i].sf);
    }
}

void mainmenu_render(mainmenu_s *t)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));
    tex_clr(asset_tex(0), TEX_CLR_WHITE);
    int fade_i = 100;
    ctx.pat    = gfx_pattern_interpolate(fade_i, 100);

    { // feather falling animation and logo
        float s1  = sin_f(t->feather_time);
        float xx  = s1 * 20.f + 315.f;
        float yy  = t->feather_y + cos_f(t->feather_time * 2.f) * 8.f - 100.f;
        float ang = s1 * PI_FLOAT * 0.1f + 0.3f;

        int fade_anim = fade_i;
        fade_anim     = max_i(fade_anim, 10);
        if (t->state != MAINMENU_ST_PRESS_START) {
            fade_anim = min_i(fade_anim, 10);
        }

        gfx_ctx_s ctx_logo = ctx;
        ctx_logo.pat       = gfx_pattern_interpolate(fade_anim, 100);

        texrec_s tfeather = asset_texrec(TEXID_TITLE, 256, 0, 64, 64);
        texrec_s tlogo    = asset_texrec(TEXID_TITLE, 0, 0, 256, 128);

        gfx_spr_rotated(ctx_logo, tfeather, (v2_i32){(int)xx, (int)yy}, (v2_i32){32, 32}, ang);
        gfx_spr(ctx_logo, tlogo, (v2_i32){70, 10}, 0, 0);
    }

    fnt_s font = asset_fnt(FNTID_LARGE);
    if (t->state != MAINMENU_ST_PRESS_START) {
        gfx_rec_fill(ctx, (rec_i32){5, t->option * 20 + 25, 10, 10}, PRIM_MODE_BLACK);
    }

    texrec_s tbutton = asset_texrec(TEXID_MAINMENU, 0, 0, 64, 64);
    int      mode    = SPR_MODE_BLACK;

    switch (t->state) {
    case MAINMENU_ST_PRESS_START: {
#define PRESS_START_TICKS 40

        int b = t->title_blink % (PRESS_START_TICKS * 2);
        if (b > PRESS_START_TICKS)
            b = 2 * PRESS_START_TICKS - b;

        int fade_j = min_i((100 * b + (PRESS_START_TICKS / 2)) / PRESS_START_TICKS, fade_i);
        fade_j     = min_i(fade_j * 2, 100);
        ctx.pat    = gfx_pattern_interpolate(pow2_i32(fade_j), pow2_i32(100));

        fnt_draw_ascii(ctx, font, (v2_i32){140, 200}, "Press Start", mode);
    } break;
    case MAINMENU_ST_FILESELECT:
        mainmenu_render_save_slots(t, ctx);
        //
        fnt_draw_ascii(ctx, font, (v2_i32){40, 100}, "Copy", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){200, 100}, "Delete", mode);
        break;
    case MAINMENU_ST_COPY_SELECT_FROM:
        mainmenu_render_save_slots(t, ctx);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "Copy from", mode);
        break;
    case MAINMENU_ST_DELETE_SELECT:
        mainmenu_render_save_slots(t, ctx);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "delete", mode);
        break;
    case MAINMENU_ST_COPY_SELECT_TO:
        mainmenu_render_save_slots(t, ctx);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "copy to", mode);
        break;
    case MAINMENU_ST_COPY_NO_OR_YES:
        mainmenu_render_yes_no(t, ctx);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "COPY?", mode);
        break;
    case MAINMENU_ST_DELETE_NO_OR_YES:
        mainmenu_render_yes_no(t, ctx);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "DELETE?", mode);
        break;
    default:
        BAD_PATH
        break;
    }

    if (!t->fade_to_game) return;

    gfx_ctx_s ctxfill = gfx_ctx_display();
    ctxfill.pat       = gfx_pattern_interpolate(t->fade_to_game, FADETICKS_MM_GAME);
    gfx_rec_fill(ctxfill, (rec_i32){0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H}, PRIM_MODE_BLACK);
}

static void mainmenu_render_save_slots(mainmenu_s *t, gfx_ctx_s ctx)
{
    fnt_s font = asset_fnt(FNTID_LARGE);
    int   mode = SPR_MODE_BLACK;

    for (int n = 0; n < 3; n++) {
        savefile_s *sf = &t->savefiles[n].sf;
        int         y  = 20 + 20 * n;
        fnt_draw_ascii(ctx, font, (v2_i32){40, y}, sf->hero_name, mode);
    }
}

static void mainmenu_render_yes_no(mainmenu_s *t, gfx_ctx_s ctx)
{
    fnt_s font = asset_fnt(FNTID_LARGE);
    int   mode = SPR_MODE_BLACK;
    fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "No", mode);
    fnt_draw_ascii(ctx, font, (v2_i32){200, 20}, "Yes", mode);
}