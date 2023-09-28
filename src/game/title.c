// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "game.h"

static void switch_to_gameplay(void *arg)
{
        game_s *g = (game_s *)arg;
        g->state  = GAMESTATE_GAMEPLAY;
}

static void title_load_savefiles(mainmenu_s *m)
{
        savefile_read(0, &m->file0);
        savefile_read(1, &m->file1);
        savefile_read(2, &m->file2);
}

static void select_game_file(game_s *g, int slotID)
{
        game_savefile_new(g, slotID);
        fading_start(&g->global_fade,
                     30, 10, 30,
                     switch_to_gameplay,
                     NULL,
                     g);
        g->mainmenu.input_blocked = 1;
}

static void cb_fade_to_select_file(void *arg)
{
        mainmenu_s *m  = (mainmenu_s *)arg;
        m->state       = MAINMENU_STATE_SELECT_FILE;
        m->curr_option = 0;
        m->num_options = NUM_MAINMENU_FILESELECT_OPTIONS;
        title_load_savefiles(m);
#if 1 // create some mockup files for testing
        savefile_s f0 = {0};
        savefile_s f1 = {0};
        savefile_s f2 = {0};
        f0.tick       = 2;
        f1.tick       = 5;
        f2.tick       = 9;
        savefile_write(0, &f0);
        savefile_write(1, &f1);
        savefile_write(2, &f2);
#endif
        title_load_savefiles(m);
}

static void cb_fade_to_title(void *arg)
{
        mainmenu_s *m        = (mainmenu_s *)arg;
        m->state             = MAINMENU_STATE_TITLE;
        m->curr_option       = 0;
        m->num_options       = 1;
        m->press_start_ticks = 0;
}

static void title_abort_copy_or_delete(mainmenu_s *m)
{
        m->num_options = 5;
        m->curr_option = 0;
        m->copystate   = 0;
        m->deletestate = 0;
}

static void update_select_file_screen(game_s *g, mainmenu_s *m)
{
        if (os_inp_just_pressed(INP_B)) {
                if (m->copystate > 0 || m->deletestate > 0) {
                        title_abort_copy_or_delete(m);
                } else {
                        fading_start(&m->fade, 30, 10, 30, cb_fade_to_title, NULL, m);
                }
                return;
        }

        if (m->num_options > 1) {
                int opt_change = 0;
                if (os_inp_just_pressed(INP_UP)) {
                        opt_change = -1;
                } else if (os_inp_just_pressed(INP_DOWN)) {
                        opt_change = +1;
                }

                int o1 = m->curr_option;
                int o2 = clamp_i(o1 + opt_change, 0, m->num_options - 1);

                if (m->copystate == MAINMENU_COPY_SELECT_TO && o2 == m->file_select_1) {
                        if (m->file_select_1 == 1) {
                                o2 = (o1 == 0 ? 2 : 0);
                        } else {
                                o2 = o1;
                        }
                }

                m->curr_option = o2;
        }

        if (os_inp_just_pressed(INP_LEFT) || os_inp_just_pressed(INP_RIGHT)) {
                switch (m->copystate) {
                case MAINMENU_COPY_YES: m->copystate = MAINMENU_COPY_NO; return;
                case MAINMENU_COPY_NO: m->copystate = MAINMENU_COPY_YES; return;
                }

                switch (m->deletestate) {
                case MAINMENU_DELETE_YES: m->deletestate = MAINMENU_DELETE_NO; return;
                case MAINMENU_DELETE_NO: m->deletestate = MAINMENU_DELETE_YES; return;
                }
        }

        if (os_inp_just_pressed(INP_A)) {
                switch (m->deletestate) {
                case MAINMENU_DELETE_SELECT:
                        m->file_select_1 = m->curr_option;
                        m->deletestate   = MAINMENU_DELETE_NO;
                        m->num_options   = 1;
                        return;
                case MAINMENU_DELETE_YES:
                        savefile_delete(m->file_select_1);
                        title_abort_copy_or_delete(m);
                        title_load_savefiles(m);
                        m->animticks_delete = 30;
                        return;
                case MAINMENU_DELETE_NO:
                        title_abort_copy_or_delete(m);
                        return;
                }

                switch (m->copystate) {
                case MAINMENU_COPY_SELECT_FROM:
                        m->file_select_1 = m->curr_option;
                        m->copystate     = MAINMENU_COPY_SELECT_TO;
                        m->num_options   = 3;
                        m->curr_option   = m->curr_option == 0 ? 1 : 0;
                        return;
                case MAINMENU_COPY_SELECT_TO:
                        m->copystate     = MAINMENU_COPY_NO;
                        m->num_options   = 1;
                        m->file_select_2 = m->curr_option;
                        return;
                case MAINMENU_COPY_YES:
                        savefile_copy(m->file_select_1, m->file_select_2);
                        title_abort_copy_or_delete(m);
                        title_load_savefiles(m);
                        m->animticks_copy = 30;
                        return;
                case MAINMENU_COPY_NO:
                        title_abort_copy_or_delete(m);
                        return;
                }

                // select this option
                switch (m->curr_option) {
                case MAINMENU_FILESELECT_0: {
                        select_game_file(g, 0);
                } break;
                case MAINMENU_FILESELECT_1: {
                        select_game_file(g, 1);
                } break;
                case MAINMENU_FILESELECT_2: {
                        select_game_file(g, 2);
                } break;
                case MAINMENU_FILESELECT_DELETE: {
                        m->deletestate = MAINMENU_DELETE_SELECT;
                        m->num_options = 3;
                        m->curr_option = 0;
                } break;
                case MAINMENU_FILESELECT_COPY: {
                        m->copystate   = MAINMENU_COPY_SELECT_FROM;
                        m->num_options = 3;
                        m->curr_option = 0;
                } break;
                }
        }
}

void update_title(game_s *g, mainmenu_s *m)
{
        // update feather animation
        m->feather_time += 0.05f;
        float s1 = sin_f(m->feather_time);
        m->feather_time += (1.f - abs_f(s1)) * 0.05f;
        m->feather_y += 0.6f;

        if (m->feather_y >= 500.f) m->feather_y = 0;

        if (fading_update(&m->fade) != 0 || m->input_blocked)
                return;

        switch (m->state) {
        case MAINMENU_STATE_TITLE:
                m->press_start_ticks++;

                if (os_inpp_raw() == 0 && os_inp_raw() != 0) { // any button
                        fading_start(&m->fade, 30, 10, 30, cb_fade_to_select_file, NULL, m);
                }
                break;
        case MAINMENU_STATE_SELECT_FILE:
                update_select_file_screen(g, m);
                break;
        }
}

void draw_title(game_s *g, mainmenu_s *m)
{
        fnt_s         font = fnt_get(FNTID_DEFAULT);
        gfx_context_s ctx  = gfx_context_create(tex_get(0));
        ctx.col            = 1;

        int pid = GFX_PATTERN_100;
        switch (fading_phase(&m->fade)) {
        case FADE_PHASE_OUT:
                pid = fading_lerp(&m->fade, GFX_PATTERN_100, GFX_PATTERN_0);
                break;
        case FADE_PHASE_PAUSE:
                pid = GFX_PATTERN_0;
                break;
        case FADE_PHASE_IN:
                pid = fading_lerp(&m->fade, GFX_PATTERN_0, GFX_PATTERN_100);
                break;
        }
        ctx.pat = gfx_pattern_get(pid);
        ctx.src = tex_get(TEXID_TITLE);

        { // feather falling animation and logo
                float s1  = sin_f(m->feather_time);
                float xx  = s1 * 20.f + 315.f;
                float yy  = m->feather_y + cos_f(m->feather_time * 2.f) * 8.f - 100.f;
                float ang = s1 * PI_FLOAT * 0.1f + 0.3f;

                gfx_context_s ctx_logo = ctx;
                switch (m->state) {
                case MAINMENU_STATE_TITLE:
                        ctx_logo.pat = gfx_pattern_get(max_i(pid, 2));
                        break;
                case MAINMENU_STATE_SELECT_FILE:
                        ctx_logo.pat = gfx_pattern_get(2);
                        break;
                }

                gfx_sprite_rotated_(ctx_logo, (v2_i32){(int)xx, (int)yy},
                                    (rec_i32){256, 0, 64, 64},
                                    (v2_i32){32, 32}, ang);
                gfx_sprite(ctx_logo, (v2_i32){70, 10}, (rec_i32){0, 0, 256, 128}, 0);
        }

        switch (m->state) {
        case MAINMENU_STATE_TITLE: {
                gfx_context_s ctx_press_start = ctx;
                float         s1              = sin_f((float)m->press_start_ticks * 0.05f) + 1.f;
                int           pid_start       = clamp_i((int)(s1 * 0.5f * 25.f), 0, 16);
                pid_start                     = min_i(pid_start, pid);
                ctx_press_start.pat           = gfx_pattern_get(pid_start);
                gfx_text_ascii(ctx_press_start, &font, "Press A", (v2_i32){150, 180});
        } break;
        case MAINMENU_STATE_SELECT_FILE: {
                int  ytext    = 60;
                int  yspacing = 25;
                int  xtext    = 140;
                char sav0[8]  = {0};
                char sav1[8]  = {0};
                char sav2[8]  = {0};
                os_strcat_i32(sav0, m->file0.tick);
                os_strcat_i32(sav1, m->file1.tick);
                os_strcat_i32(sav2, m->file2.tick);

                gfx_text_ascii(ctx, &font, sav0, (v2_i32){xtext - 50, ytext + yspacing * 0});
                gfx_text_ascii(ctx, &font, sav1, (v2_i32){xtext - 50, ytext + yspacing * 1});
                gfx_text_ascii(ctx, &font, sav2, (v2_i32){xtext - 50, ytext + yspacing * 2});

                gfx_text_ascii(ctx, &font, "File 1", (v2_i32){xtext, ytext + yspacing * 0});
                gfx_text_ascii(ctx, &font, "File 2", (v2_i32){xtext, ytext + yspacing * 1});
                gfx_text_ascii(ctx, &font, "File 3", (v2_i32){xtext, ytext + yspacing * 2});
                gfx_text_ascii(ctx, &font, "Delete", (v2_i32){xtext, ytext + yspacing * 3});
                gfx_text_ascii(ctx, &font, "Copy", (v2_i32){xtext, ytext + yspacing * 4});

                char cfile1[16] = {0};
                char cfile2[16] = {0};
                os_strcat_i32(cfile1, m->file_select_1);
                os_strcat_i32(cfile2, m->file_select_2);
                switch (m->copystate) {
                case MAINMENU_COPY_SELECT_FROM:
                        gfx_text_ascii(ctx, &font, "copy from", (v2_i32){250, ytext});

                        break;
                case MAINMENU_COPY_SELECT_TO:
                        gfx_text_ascii(ctx, &font, "copy to", (v2_i32){250, ytext});
                        gfx_text_ascii(ctx, &font, cfile1, (v2_i32){250, ytext + 20});
                        break;
                case MAINMENU_COPY_YES:
                        gfx_text_ascii(ctx, &font, "copy yes?", (v2_i32){250, ytext});
                        gfx_text_ascii(ctx, &font, cfile1, (v2_i32){250, ytext + 20});
                        gfx_text_ascii(ctx, &font, cfile2, (v2_i32){250, ytext + 40});
                        break;
                case MAINMENU_COPY_NO:
                        gfx_text_ascii(ctx, &font, "copy no?", (v2_i32){250, ytext});
                        gfx_text_ascii(ctx, &font, cfile1, (v2_i32){250, ytext + 20});
                        gfx_text_ascii(ctx, &font, cfile2, (v2_i32){250, ytext + 40});
                        break;
                }

                switch (m->deletestate) {
                case MAINMENU_DELETE_SELECT:
                        gfx_text_ascii(ctx, &font, "del", (v2_i32){250, ytext});
                        break;
                case MAINMENU_DELETE_YES:
                        gfx_text_ascii(ctx, &font, "del yes?", (v2_i32){250, ytext});
                        gfx_text_ascii(ctx, &font, cfile1, (v2_i32){250, ytext + 20});
                        break;
                case MAINMENU_DELETE_NO:
                        gfx_text_ascii(ctx, &font, "del no?", (v2_i32){250, ytext});
                        gfx_text_ascii(ctx, &font, cfile1, (v2_i32){250, ytext + 20});
                        break;
                }

                if (m->num_options > 1) {
                        gfx_rec_fill(ctx, (rec_i32){0, m->curr_option * yspacing + ytext, 10, 10});
                }

        } break;
        }
}