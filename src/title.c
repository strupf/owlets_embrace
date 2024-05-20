// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "app.h"
#include "game.h"
#include "sys/sys.h"

enum {
    TITLE_SOUND_BUTTON,
    TITLE_SOUND_START,
    TITLE_SOUND_PROMPT,
    TITLE_SOUND_DELETE,
    TITLE_SOUND_COPY,
    TITLE_SOUND_ABORT,
    TITLE_SOUND_NAVIGATE,
};

static void title_pressed_A(game_s *g, title_s *t);
static void title_pressed_B(title_s *t);
static void title_navigate(game_s *g, title_s *t, int dx, int dy);
static void title_op_start_file(game_s *g, title_s *t);
static void title_play_sound(int soundID);

void title_init(title_s *t)
{
#if 1
    save_s hs       = {0};
    hs.game_version = GAME_VERSION;
#if 1
    hs.upgrades[HERO_UPGRADE_AIR_JUMP_1] = 1;
    hs.upgrades[HERO_UPGRADE_AIR_JUMP_2] = 1;
    hs.upgrades[HERO_UPGRADE_AIR_JUMP_3] = 1;
#endif
    hs.upgrades[HERO_UPGRADE_HOOK]      = 1;
    hs.upgrades[HERO_UPGRADE_WALLJUMP]  = 0;
    hs.upgrades[HERO_UPGRADE_GLIDE]     = 1;
    hs.upgrades[HERO_UPGRADE_HOOK_LONG] = 1;
    hs.upgrades[HERO_UPGRADE_WHIP]      = 1;
    hs.upgrades[HERO_UPGRADE_SWIM]      = 1;
    hs.upgrades[HERO_UPGRADE_DIVE]      = 1;
    hs.upgrades[HERO_UPGRADE_SPRINT]    = 1;
    hs.health                           = 3;

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

void title_update(game_s *g, title_s *t)
{
#if TITLE_SKIP_TO_GAME
    title_op_start_file(g, t);
    return;
#endif

    t->title_blink++;

    // update feather animation
    t->feather_time += 0.05f;
    float s1 = sin_f(t->feather_time);
    t->feather_time += (1.f - ABS(s1)) * 0.05f;
    t->feather_y += 0.6f;

    if (t->feather_y >= 500.f) t->feather_y = 0.f;

    if (t->fade_to_game) {
        t->fade_to_game++;
        if (t->fade_to_game < (FADETICKS_MM_GAME + FADETICKS_MM_GAME_BLACK)) return;
        t->fade_to_game = 0;
        title_op_start_file(g, t);
        return;
    }

    if (inp_just_pressed(INP_B)) { // back or abort
        title_pressed_B(t);
        return;
    }

    if (inp_just_pressed(INP_A)) { // select or confirm
        title_pressed_A(g, t);
        return;
    }

    int dx = 0;
    int dy = 0;
    if (inp_just_pressed(INP_DPAD_L)) dx = -1;
    if (inp_just_pressed(INP_DPAD_R)) dx = +1;
    if (inp_just_pressed(INP_DPAD_U)) dy = -1;
    if (inp_just_pressed(INP_DPAD_D)) dy = +1;

    if (dx != 0 || dy != 0) {
        title_navigate(g, t, dx, dy);
    }
}

static void cb_title_to_press_start(void *arg)
{
    title_s *t     = (title_s *)arg;
    t->state       = TITLE_ST_PRESS_START;
    t->title_blink = 0;
}

static void title_pressed_A(game_s *g, title_s *t)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START:
        title_play_sound(TITLE_SOUND_BUTTON);
        title_op_start_file(g, t);
        break;
    }
}

static void title_pressed_B(title_s *t)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START:
        break;
    default:
        title_play_sound(TITLE_SOUND_ABORT);
        t->option = 0;
        break;
    }
}

static void title_navigate(game_s *g, title_s *t, int dx, int dy)
{
    switch (t->state) {
    case TITLE_ST_PRESS_START:
        break;
    default:
        BAD_PATH
        break;
    }
}

static void title_op_start_file(game_s *g, title_s *t)
{
    title_play_sound(TITLE_SOUND_START);
    game_load_savefile(g);
    g->state = APP_STATE_GAME;
    app_set_menu_gameplay();
}

static void title_play_sound(int soundID)
{
}

void title_render(title_s *t)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));
    tex_clr(asset_tex(0), GFX_COL_WHITE);
    int fade_i = 100;
    ctx.pat    = gfx_pattern_interpolate(fade_i, 100);

    { // feather falling animation and logo
        float s1  = sin_f(t->feather_time);
        float xx  = s1 * 20.f + 315.f;
        float yy  = t->feather_y + cos_f(t->feather_time * 2.f) * 8.f - 100.f;
        float ang = s1 * PI_FLOAT * 0.1f + 0.3f;

        int fade_anim = fade_i;
        fade_anim     = max_i(fade_anim, 10);
        if (t->state != TITLE_ST_PRESS_START) {
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
    if (t->state != TITLE_ST_PRESS_START) {
        gfx_rec_fill(ctx, (rec_i32){5, t->option * 20 + 25, 10, 10}, PRIM_MODE_BLACK);
    }

    texrec_s tbutton = asset_texrec(TEXID_TITLE, 0, 0, 64, 64);
    int      mode    = SPR_MODE_BLACK;

    switch (t->state) {
    case TITLE_ST_PRESS_START: {
#define PRESS_START_TICKS 40

        int b = t->title_blink % (PRESS_START_TICKS * 2);
        if (b > PRESS_START_TICKS)
            b = 2 * PRESS_START_TICKS - b;

        int fade_j = min_i((100 * b + (PRESS_START_TICKS / 2)) / PRESS_START_TICKS, fade_i);
        fade_j     = min_i(fade_j * 2, 100);
        ctx.pat    = gfx_pattern_interpolate(pow2_i32(fade_j), pow2_i32(100));

        i32 strl = fnt_length_px(font, "Press Start");
        fnt_draw_ascii(ctx, font, (v2_i32){200 - strl / 2, 200}, "Press Start", mode);
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