// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
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
static void mainmenu_navigate(mainmenu_s *t, int dx, int dy);
static void mainmenu_op_start_file(game_s *g, mainmenu_s *t, int index);
static void mainmenu_play_sound(int soundID);
static void mainmenu_update_savefiles(mainmenu_s *t);

void mainmenu_init(mainmenu_s *t)
{
#if 1
    // write some files to debug copy and delete
    t->savefiles[0].sf.tick = 1;
    t->savefiles[1].sf.tick = 5;
    t->savefiles[2].sf.tick = -4610;
    t->savefiles[0].sf.aquired_items |= 1 << HERO_ITEM_HOOK;
    t->savefiles[0].sf.aquired_items |= 1 << HERO_ITEM_WHIP;
    t->savefiles[0].sf.aquired_upgrades |= 1 << HERO_UPGRADE_HOOK;
    t->savefiles[0].sf.aquired_upgrades |= 1 << HERO_UPGRADE_WHIP;

    strcpy(t->savefiles[0].sf.area_filename, "map_01.tmj");
    savefile_write(0, &t->savefiles[0].sf);
    savefile_write(1, &t->savefiles[1].sf);
    savefile_write(2, &t->savefiles[2].sf);
#endif
    mainmenu_update_savefiles(t);
}

void mainmenu_update(game_s *g, mainmenu_s *t)
{
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
        mainmenu_navigate(t, dx, dy);
    }
}

void mainmenu_render(mainmenu_s *t)
{
    gfx_ctx_s ctx  = gfx_ctx_default(asset_tex(0));
    fnt_s     font = asset_fnt(FNTID_DEFAULT);

    gfx_rec_fill(ctx, (rec_i32){5, t->option * 20 + 25, 10, 10}, PRIM_MODE_BLACK);

    int mode = SPR_MODE_BLACK;

    char buf0[8] = {0};
    char buf1[8] = {0};
    char buf2[8] = {0};
    str_append_i(buf0, t->savefiles[0].sf.tick);
    str_append_i(buf1, t->savefiles[1].sf.tick);
    str_append_i(buf2, t->savefiles[2].sf.tick);
    fnt_draw_ascii(ctx, font, (v2_i32){200, 20}, buf0, mode);
    fnt_draw_ascii(ctx, font, (v2_i32){200, 40}, buf1, mode);
    fnt_draw_ascii(ctx, font, (v2_i32){200, 60}, buf2, mode);

    switch (t->state) {
    case MAINMENU_STATE_TITLE:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "Press Start", mode);
        break;
    case MAINMENU_ST_FILESELECT:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "File 0", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "File 1", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 60}, "File 2", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 80}, "Copy", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 100}, "Delete", mode);
        break;
    case MAINMENU_ST_COPY_SELECT_FROM:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "File 0", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "File 1", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 60}, "File 2", mode);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "Copy from", mode);
        break;
    case MAINMENU_ST_DELETE_SELECT:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "File 0", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "File 1", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 60}, "File 2", mode);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "delete", mode);
        break;
    case MAINMENU_ST_COPY_SELECT_TO:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "File 0", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "File 1", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 60}, "File 2", mode);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "copy to", mode);
        break;
    case MAINMENU_ST_COPY_NO_OR_YES:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "No", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "Yes", mode);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "COPY?", mode);
        break;
    case MAINMENU_ST_DELETE_NO_OR_YES:
        fnt_draw_ascii(ctx, font, (v2_i32){40, 20}, "No", mode);
        fnt_draw_ascii(ctx, font, (v2_i32){40, 40}, "Yes", mode);

        fnt_draw_ascii(ctx, font, (v2_i32){40, 200}, "DELETE?", mode);
        break;
    default:
        BAD_PATH
        break;
    }
}

static void mainmenu_pressed_A(game_s *g, mainmenu_s *t)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        t->state = MAINMENU_ST_FILESELECT;
        mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
        break;
        //
    case MAINMENU_ST_FILESELECT:
        switch (t->option) {
        case 0:
        case 1:
        case 2: // selected file to play
            mainmenu_op_start_file(g, t, t->option);
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
        t->state = MAINMENU_ST_PRESS_START;
        break;
    default:
        mainmenu_play_sound(MAINMENU_SOUND_ABORT);
        t->state  = MAINMENU_ST_FILESELECT;
        t->option = 0;
        break;
    }
}

static void mainmenu_navigate(mainmenu_s *t, int dx, int dy)
{
    switch (t->state) {
    case MAINMENU_STATE_TITLE:
        t->state = MAINMENU_ST_FILESELECT;
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