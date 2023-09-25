// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "game.h"

static void select_savestate(game_s *g, mainmenu_s *m);
static void select_title(game_s *g, mainmenu_s *m);
static void select_copy_savestate(game_s *g, mainmenu_s *m);
static void select_delete_savestate(game_s *g, mainmenu_s *m);

static void switch_to_gameplay(void *arg)
{
        game_s *g = (game_s *)arg;
        g->state  = GAMESTATE_GAMEPLAY;
}

static void select_game_file(game_s *g, int slotID)
{
        game_savefile_new(g, slotID);
        fading_start(&g->global_fade,
                     60, 60, 60,
                     switch_to_gameplay,
                     NULL,
                     g);
        g->mainmenu.input_blocked = 1;
}

static void update_option_select(mainmenu_s *m)
{
        if (m->num_options == 0) return;
        if (os_inp_just_pressed(INP_UP)) {
                m->curr_option--;
        } else if (os_inp_just_pressed(INP_DOWN)) {
                m->curr_option++;
        }
        m->curr_option = clamp_i(m->curr_option, 0, m->num_options - 1);
}

static void cb_fade_to_select_file(void *arg)
{
        mainmenu_s *m  = (mainmenu_s *)arg;
        m->state       = MAINMENU_STATE_SELECT_FILE;
        m->curr_option = 0;
        m->num_options = 3;
}

static void cb_fade_to_title(void *arg)
{
        mainmenu_s *m        = (mainmenu_s *)arg;
        m->state             = MAINMENU_STATE_TITLE;
        m->curr_option       = 0;
        m->num_options       = 1;
        m->press_start_ticks = 0;
}

static void update_title_screen(game_s *g, mainmenu_s *m)
{
        m->press_start_ticks++;

        if (os_inp_just_pressed(INP_A)) {
                fading_start(&m->fade, 30, 10, 30, cb_fade_to_select_file, NULL, m);
        }
}

static void update_select_file_screen(game_s *g, mainmenu_s *m)
{
        update_option_select(m);

        if (os_inp_just_pressed(INP_B)) {
                fading_start(&m->fade, 30, 10, 30, cb_fade_to_title, NULL, m);
                return;
        }

        if (!os_inp_just_pressed(INP_A)) return;

        // select this option
        switch (m->curr_option) {
        case 0: {
                select_game_file(g, 0);
        } break;
        case 1: {
                select_game_file(g, 1);
        } break;
        case 2: {
                select_game_file(g, 2);
        } break;
        case 3: {

        } break;
        case 4: {

        } break;
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
                update_title_screen(g, m);
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
                                    (v2_i32){32.f, 32.f}, ang);
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
                int ytext    = 60;
                int yspacing = 25;
                int xtext    = 140;
                gfx_text_ascii(ctx, &font, "File 1", (v2_i32){xtext, ytext + yspacing * 0});
                gfx_text_ascii(ctx, &font, "File 2", (v2_i32){xtext, ytext + yspacing * 1});
                gfx_text_ascii(ctx, &font, "File 3", (v2_i32){xtext, ytext + yspacing * 2});
                gfx_text_ascii(ctx, &font, "Options", (v2_i32){xtext, ytext + yspacing * 3});
                gfx_text_ascii(ctx, &font, "Quit", (v2_i32){xtext, ytext + yspacing * 4});

                gfx_rec_fill(ctx, (rec_i32){0, m->curr_option * yspacing + ytext, 10, 10});
        } break;
        }
}