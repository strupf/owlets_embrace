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
static void mainmenu_op_start_file(game_s *g, mainmenu_s *t);
static void mainmenu_play_sound(int soundID);

void mainmenu_init(mainmenu_s *t)
{
#if 1
    save_s hs = {0};

    hs.upgrades[HERO_UPGRADE_AIR_JUMP_1] = 1;
    hs.upgrades[HERO_UPGRADE_AIR_JUMP_2] = 1;
    hs.upgrades[HERO_UPGRADE_HOOK]       = 1;
    hs.upgrades[HERO_UPGRADE_WALLJUMP]   = 0;
    hs.upgrades[HERO_UPGRADE_GLIDE]      = 1;
    hs.upgrades[HERO_UPGRADE_LONG_HOOK]  = 1;
    hs.upgrades[HERO_UPGRADE_WHIP]       = 1;
    hs.health                            = 3;

    strcpy(hs.hero_mapfile, "Level_0");
    hs.hero_pos.x = 50;
    hs.hero_pos.y = 100;

    void *f = sys_file_open(SAVEFILE_NAME, SYS_FILE_W);
    if (f) {
        sys_file_write(f, (const void *)&hs, sizeof(save_s));
        sys_file_close(f);
    }
#endif
}

void mainmenu_update(game_s *g, mainmenu_s *t)
{
#if MAINMENU_SKIP_TO_GAME
    mainmenu_op_start_file(g, t);
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
        mainmenu_op_start_file(g, t);
        g->substate.state = SUBSTATE_MAINMENU_FADE_IN;
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

static void cb_mainmenu_to_press_start(void *arg)
{
    mainmenu_s *t  = (mainmenu_s *)arg;
    t->state       = MAINMENU_ST_PRESS_START;
    t->title_blink = 0;
}

static void mainmenu_pressed_A(game_s *g, mainmenu_s *t)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        mainmenu_play_sound(MAINMENU_SOUND_BUTTON);
        mainmenu_op_start_file(g, t);
        break;
    }
}

static void mainmenu_pressed_B(mainmenu_s *t)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        break;
    default:
        mainmenu_play_sound(MAINMENU_SOUND_ABORT);
        t->option = 0;
        break;
    }
}

static void mainmenu_navigate(game_s *g, mainmenu_s *t, int dx, int dy)
{
    switch (t->state) {
    case MAINMENU_ST_PRESS_START:
        break;
    default:
        BAD_PATH
        break;
    }
}

static void mainmenu_op_start_file(game_s *g, mainmenu_s *t)
{
    mainmenu_play_sound(MAINMENU_SOUND_START);
    game_load_savefile(g);

    g->state = GAMESTATE_GAMEPLAY;
}

static void mainmenu_play_sound(int soundID)
{
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
    default:
        BAD_PATH
        break;
    }

    if (!t->fade_to_game) return;

    gfx_ctx_s ctxfill = gfx_ctx_display();
    ctxfill.pat       = gfx_pattern_interpolate(t->fade_to_game, FADETICKS_MM_GAME);
    gfx_rec_fill(ctxfill, (rec_i32){0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H}, PRIM_MODE_BLACK);
}