// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    GAMEOVER_NONE,
    GAMEOVER_DYING,
    GAMEOVER_GAMEOVER_FADE_IN,
    GAMEOVER_GAMEOVER,
    GAMEOVER_GAMEOVER_INPUT,
    GAMEOVER_GAMEOVER_FADE_OUT,
    GAMEOVER_BLACK,
    GAMEOVER_FADE,
    //
    NUM_GAMEOVER_PHASES
};

static int gameover_ticks(gameover_s *go)
{
    static int phase_ticks[NUM_GAMEOVER_PHASES] = {
        0,
        30, // dying
        20, // fade gameover
        80, // gameover
        10, // gameover input
        10, // fade gameover
        10, // black
        25  // fade
    };
    assert(0 <= go->phase && go->phase < NUM_GAMEOVER_PHASES);
    return phase_ticks[go->phase];
}

void gameover_start(game_s *g)
{
    gameover_s *go = &g->gameover;
    go->tick       = 0;
    go->phase      = 1;
    g->substate    = SUBSTATE_GAMEOVER;
}

void gameover_update(game_s *g)
{
    gameover_s *go = &g->gameover;

    if (go->phase == GAMEOVER_GAMEOVER_INPUT) {
        if (!(inp_just_pressed(INP_A) || 1)) return;
        go->tick = gameover_ticks(go);
    }

    go->tick++;
    if (go->tick < gameover_ticks(go)) return;

    go->tick = 0;
    go->phase++;
    go->phase %= NUM_GAMEOVER_PHASES;
    if (go->phase == 0) {
        g->substate = 0;
        return;
    }

    if (go->phase == GAMEOVER_BLACK) {
        game_load_savefile(g);
    }
}

void gameover_draw(game_s *g, v2_i32 cam)
{
    gameover_s *go = &g->gameover;

    assert(0 <= go->phase && go->phase < NUM_GAMEOVER_PHASES);
    const gfx_ctx_s ctx      = gfx_ctx_display();
    const int       ticks    = gameover_ticks(go);
    const rec_i32   rdisplay = {0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H};

    gfx_ctx_s ctx_r      = ctx;
    int       p_gameover = 12;

    switch (go->phase) {
    case GAMEOVER_DYING: {
        int p     = lerp_i32(0, p_gameover, go->tick, ticks);
        ctx_r.pat = gfx_pattern_bayer_4x4(p);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case GAMEOVER_GAMEOVER_FADE_IN:
    case GAMEOVER_GAMEOVER:
    case GAMEOVER_GAMEOVER_INPUT: {
        ctx_r.pat = gfx_pattern_bayer_4x4(p_gameover);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);

        fnt_s font = asset_fnt(FNTID_LARGE);

        v2_i32 pos = {150, 100};
        for (int y = -2; y <= +2; y++) {
            for (int x = -2; x <= +2; x++) {
                v2_i32 p = pos;
                p.x += x;
                p.y += y;
                fnt_draw_ascii(ctx, font, p, "Game Over", SPR_MODE_WHITE);
            }
        }

        fnt_draw_ascii(ctx, font, pos, "Game Over", SPR_MODE_BLACK);

        if (go->phase == GAMEOVER_GAMEOVER_INPUT) {
        }
        break;
    }
    case GAMEOVER_GAMEOVER_FADE_OUT: {
        int p     = lerp_i32(p_gameover, GFX_PATTERN_MAX, go->tick, ticks);
        ctx_r.pat = gfx_pattern_bayer_4x4(p);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case GAMEOVER_BLACK: {
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case GAMEOVER_FADE: {
        ctx_r.pat = gfx_pattern_interpolate(ticks - go->tick, ticks);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    }
}
