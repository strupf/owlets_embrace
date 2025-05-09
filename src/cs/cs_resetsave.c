// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

enum {
    CS_RESETSAVE_FADE_OUT,
    CS_RESETSAVE_BLACK,
    CS_RESETSAVE_FADE_IN
};

#define CS_RESETSAVE_TICKS_FADE_OUT 15
#define CS_RESETSAVE_TICKS_BLACK    10
#define CS_RESETSAVE_TICKS_FADE_IN  25

void cs_resetsave_update(g_s *g, cs_s *cs);
void cs_resetsave_draw(g_s *g, cs_s *cs, v2_i32 cam);

void cs_resetsave_enter(g_s *g)
{
    cs_s *cs = &g->cuts;
    cs_reset(g);
    cs->on_update   = cs_resetsave_update;
    cs->on_draw     = cs_resetsave_draw;
    g->block_update = 1;
}

void cs_resetsave_update(g_s *g, cs_s *cs)
{
    switch (cs->phase) {
    case CS_RESETSAVE_FADE_OUT: {
        if (CS_RESETSAVE_TICKS_FADE_OUT <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_RESETSAVE_BLACK: {
        if (CS_RESETSAVE_TICKS_BLACK <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
            mus_play_ext(0, 0, 0, 0, 100, 0, 0);
            g->musicID = 0;
            aud_stop_all_snd_instances();
            game_update_savefile(g);
            game_load_savefile(g);
            g->block_update = 0;
        }
        break;
    }
    case CS_RESETSAVE_FADE_IN: {
        if (CS_RESETSAVE_TICKS_FADE_IN <= cs->tick) {
            cs_reset(g);
        }
        break;
    }
    }
}

void cs_resetsave_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    tex_s display = asset_tex(0);

    switch (cs->phase) {
    case CS_RESETSAVE_FADE_OUT: {
        render_map_transition_out(g, cam, cs->tick, CS_RESETSAVE_TICKS_FADE_OUT);
        break;
    }
    case CS_RESETSAVE_BLACK: {
        tex_clr(display, GFX_COL_BLACK);
        break;
    }
    case CS_RESETSAVE_FADE_IN: {
        render_map_transition_in(g, cam, cs->tick, CS_RESETSAVE_TICKS_FADE_IN);
        break;
    }
    }
}
