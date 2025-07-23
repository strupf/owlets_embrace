// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "title.h"
#include "app.h"
#include "game.h"

#define TITLE_INTRO_LOGO_TICKS    20
#define TITLE_INTRO_TICKS         40
#define TITLE_CONFIRM_TICKS       100
#define TITLE_START_TICKS         60 // total time to fade to the game
#define TITLE_START_SLIDE_TICKS   40 // time of sliding the scene over
#define TITLE_START_FADE_TICKS    40 // time to fade out left gradient
#define TITLE_FADE_TICKS          16
#define TITLE_FADE_TICKS_TO_BLACK 12

// mainmenu state machine
enum {
    TITLE_ST_OVERVIEW, // title screen
    TITLE_ST_OVERVIEW_MENU,
    TITLE_ST_SETTINGS,
    TITLE_ST_CREDITS,
    //
    TITLE_ST_FILE_SELECT,
    TITLE_ST_FILE_SELECTED,
    TITLE_ST_FILE_CPY,
    TITLE_ST_FILE_CPY_CONFIRM,
    TITLE_ST_FILE_DEL_CONFIRM,
    TITLE_ST_FILE_NEW,
    TITLE_ST_TO_GAME,
};

enum {
    TITLE_OV_0_START,
    TITLE_OV_1_SETTINGS,
    TITLE_OV_2_CREDITS
};

enum {
    TITLE_FP_0_START,
    TITLE_FP_1_COPY,
    TITLE_FP_2_DELETE
};

void   title_to_state(title_s *t, i32 state, i32 fade);
void   title_load_previews(title_s *t);
void   title_draw_companion(title_s *t);
void   title_start_game(app_s *app, i32 slot);
void   title_draw_version();
//
void   title_btn_move_to(title_btn_s *b, i32 x, i32 y, i32 ticks);
v2_i32 title_btn_pos(title_s *t, i32 ID);
bool32 title_btn_pos_companion(title_s *t, i32 ID, v2_i32 *pos);
void   title_btn_toggle_ext(title_s *t, i32 ID, i32 show, i32 setto);
void   title_btn_toggle(title_s *t, i32 ID, i32 show);

void title_to_state(title_s *t, i32 state, i32 fade)
{
    t->state_prev = t->state;
    t->state      = state;
}

void title_init(title_s *t)
{
    g_s *g = &APP.game;
    title_load_previews(t);
    t->preload_slot     = -1;
    g->render_map_doors = 0;
    g->previewmode      = 1;
#if !TITLE_SKIP_TO_GAME
    mus_play_extv("M_WATERFALL", 0, 0, 0, 100, 256);
#endif
}

void title_start_game(app_s *app, i32 slot)
{
    g_s *g                = &app->game;
    app->title.state      = TITLE_ST_TO_GAME;
    app->title.start_tick = 0;
    game_cue_area_music(g);
#if PLTF_PD
    // pltf_pd_menu_add("savepoint", app_menu_callback_resetsave, 0);
    pltf_pd_menu_add("map", app_menu_callback_map, 0);
#endif
}

void title_gameplay_start(app_s *app)
{
    g_s *g                = &app->game;
    g->block_hero_control = 0;
    g->render_map_doors   = 1;
    g->previewmode        = 0;
    app->state            = APP_ST_GAME;
}

bool32 title_preload_available(title_s *t)
{
    switch (t->state) {
    case TITLE_ST_FILE_SELECT:
    case TITLE_ST_FILE_SELECTED:
    case TITLE_ST_FILE_CPY:
    case TITLE_ST_TO_GAME:
        return (t->saves[t->selected].exists);
    default: break;
    }
    return 0;
}

void title_try_preload(app_s *app, title_s *t)
{
    if (3 <= t->selected || !t->saves[t->selected].exists ||
        (0 <= t->preload_slot && t->preload_slot != t->selected)) {
        t->preload_fade_dir = 0;
    } else if (t->preload_slot == t->selected) {
        t->preload_fade_dir = 1;
    } else if (t->preload_fade_q7 == 0) {
        savefile_s *s         = &app->save;
        g_s        *g         = &app->game;
        g->block_hero_control = 1;
        g->save_slot          = t->selected;
        err32 err             = savefile_r(t->selected, s);
        if (err) {
            t->preload_slot     = -1;
            t->preload_fade_dir = 0;
        } else {
            t->preload_fade_dir = 1;
            t->preload_slot     = t->selected;
            game_load_savefile(g);
        }
    }
}

void title_update(app_s *app, title_s *t)
{
#if TITLE_SKIP_TO_GAME
    g_s *g = &app->game;
    savefile_r(0, g->savefile);
    game_load_savefile(g);
    title_start_game(app, 0);
    title_gameplay_start(app);
#else
    if (t->state != TITLE_ST_OVERVIEW) {
        t->tick++;
    }

    i32 dx = inp_btn_jp(INP_DL) ? -1 : (inp_btn_jp(INP_DR) ? +1 : 0);
    i32 dy = inp_btn_jp(INP_DU) ? -1 : (inp_btn_jp(INP_DD) ? +1 : 0);

    for (i32 n = 0; n < NUM_TITLE_BTNS; n++) {
        title_btn_s *b = &t->buttons[n];
        if (b->lerp_tick < b->lerp_tick_max) {
            b->lerp_tick++;
        } else {
            b->lerp_tick = b->lerp_tick_max;
        }
    }

    if (t->comp_bump) {
        t->comp_bump++;
        if (ani_len(ANIID_COMPANION_BUMP) <= t->comp_bump) {
            t->comp_bump = 0;
        }
    } else {
        title_btn_pos_companion(t, t->comp_btnID, &t->pos_comp_target);
        v2_i32 target_q8 = v2_i32_shl(t->pos_comp_target, 8);
        if (v2_i32_distancesq(t->pos_comp_target, v2_i32_shr(t->pos_comp_q8, 8)) < 10) {
            t->pos_comp_q8 = target_q8;
            t->v_comp_q8.x = 0;
            t->v_comp_q8.y = 0;
        } else {
            t->pos_comp_q8 = v2_i32_add(t->pos_comp_q8, t->v_comp_q8);
            v2_i32 steer   = steer_arrival(t->pos_comp_q8,
                                           t->v_comp_q8,
                                           target_q8,
                                           4000,
                                           32 << 7);
            t->v_comp_q8   = v2_i32_add(t->v_comp_q8, steer);
        }
    }

    switch (t->state) {
    case TITLE_ST_OVERVIEW: {
        if (t->title_fade_dir) {
            t->title_fade += t->title_fade_dir;

            if (TITLE_FADE_TICKS <= t->title_fade) {
                t->title_fade      = TITLE_FADE_TICKS;
                t->title_fade_dir  = -1;
                t->option          = 0;
                t->selected        = 0;
                t->preload_slot    = -1;
                t->preload_fade_q7 = 0;
                t->comp_btnID      = TITLE_BTN_SLOT_1;
                t->tick            = 0;
                t->title_subst     = 0;
                title_try_preload(app, t);
                title_to_state(t, TITLE_ST_FILE_SELECT, 0);

                for (i32 n = 0; n < NUM_TITLE_BTNS; n++) {
                    title_btn_toggle_ext(t, n, 0, 1);
                }

                title_btn_toggle(t, TITLE_BTN_SLOT_1, 1);
                title_btn_toggle(t, TITLE_BTN_SLOT_2, 1);
                title_btn_toggle(t, TITLE_BTN_SLOT_3, 1);
                title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 1);
            } else if (t->title_fade <= 0) {
                t->title_fade     = 0;
                t->title_fade_dir = 0;
            }
        } else {
            t->tick++;
#if GAME_DEMO
            if (inp_btn_jp(INP_A)) {
                t->title_fade_dir = +1; // fade
                snd_play(SNDID_MENU3, 1.f, 1.f);
            }
#else
            if (t->title_subst) {
                if (dy) {
                    t->option = clamp_i32(t->option + dy, 0, 2);
                } else if (inp_btn_jp(INP_A)) {
                    switch (t->option) {
                    case 0:
                        t->title_fade_dir = +1; // fade
                        break;
                    case 1:
                        APP.sm.n_curr = 0;
                        APP.sm.active = 1;
                        break;
                    }
                    snd_play(SNDID_MENU3, 1.f, 1.f);
                }
            } else {
                if (inp_btn_jp(INP_A)) {
                    t->title_subst = 1;
                }
            }
#endif
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
        if (t->title_fade_dir) {
            t->title_fade += t->title_fade_dir;

            if (TITLE_FADE_TICKS <= t->title_fade) {
                t->title_fade      = TITLE_FADE_TICKS;
                t->title_fade_dir  = -1;
                t->option          = 0;
                t->preload_slot    = -1;
                t->preload_fade_q7 = 0;
                t->tick            = 0;
                title_to_state(t, TITLE_ST_OVERVIEW, 0);
            } else if (t->title_fade <= 0) {
                t->title_fade     = 0;
                t->title_fade_dir = 0;
            }
        } else if (inp_btn_jp(INP_A)) {
            t->comp_bump = 1;
            snd_play(SNDID_MENU3, 1.f, 1.f);

            if (t->option < 3) {
                t->selected = t->option;

                if (t->saves[t->selected].exists) {
                    title_to_state(t, TITLE_ST_FILE_SELECTED, 0);

                    for (i32 n = 0; n < 3; n++) {
                        i32 bID = TITLE_BTN_SLOT_1 + n;
                        if (n == t->selected) {
                            title_btn_toggle(t, bID, 1);
                            t->buttons[bID].x_dst += 20;
                        } else {
                            title_btn_toggle(t, bID, 0);
                        }
                    }

                    title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 0);
                    title_btn_toggle(t, TITLE_BTN_SLOT_COPY, 1);
                    title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 1);
                } else {
                    // title_to_state(t, TITLE_ST_FILE_NEW, 0);
                }
            } else { // speedrun
            }
            t->option = 0;
        } else if (inp_btn_jp(INP_B)) {
            t->title_fade_dir = +1; // fade out
        } else {
#if !GAME_DEMO
            t->option   = clamp_i32((i32)t->option + dy, 0, 2);
            t->selected = t->option;
            if (dy) {
                snd_play(SNDID_MENU1, 1.f, 1.f);
            }
#endif

            switch (t->option) {
            case 0:
            case 1:
            case 2: t->comp_btnID = TITLE_BTN_SLOT_1 + t->option; break;
            case 3: t->comp_btnID = TITLE_BTN_SPEEDRUN; break;
            }
        }
        title_try_preload(app, t);
        break;
    }
    case TITLE_ST_FILE_NEW: {
        if (inp_btn_jp(INP_B)) {
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            for (i32 n = 0; n < 3; n++) {
                title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 1);
            }
            title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 1);
            title_btn_toggle(t, TITLE_BTN_SLOT_COPY, 0);
            title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 0);
        }
        break;
    }
    case TITLE_ST_FILE_SELECTED: {
        if (inp_btn_jp(INP_A)) {
            snd_play(SNDID_MENU3, 1.f, 1.f);

            switch (t->option) {
            case TITLE_FP_0_START: {
                t->comp_bump = 1;
                title_start_game(app, t->selected);
                for (i32 n = 0; n < NUM_TITLE_BTNS; n++) {
                    title_btn_toggle(t, n, 0);
                }
                break;
            }
            case TITLE_FP_1_COPY: {
                t->option    = t->selected == 0 ? 1 : 0;
                t->comp_bump = 1;
                title_to_state(t, TITLE_ST_FILE_CPY, 0);
                for (i32 n = 0; n < 3; n++) {
                    if (n != t->selected) {
                        title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 1);
                    }
                }
                title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 0);
                title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 0);
                break;
            }
            case TITLE_FP_2_DELETE: {
                t->comp_bump = 1;

                bool32 res = savefile_del(t->selected);
                title_load_previews(t);
                t->option     = 0;
                t->comp_btnID = TITLE_BTN_SLOT_1;
                title_to_state(t, TITLE_ST_FILE_SELECT, 0);
                for (i32 n = 0; n < 3; n++) {
                    title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 1);
                }
                title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 1);
                title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 0);
                title_btn_toggle(t, TITLE_BTN_SLOT_COPY, 0);
                break;
            }
            }
        } else if (inp_btn_jp(INP_B)) {
            t->option     = t->selected;
            t->comp_btnID = TITLE_BTN_SLOT_1 + t->selected;
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            for (i32 n = 0; n < 3; n++) {
                title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 1);
            }
            title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 1);
            title_btn_toggle(t, TITLE_BTN_SLOT_COPY, 0);
            title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 0);
        } else {
#if !GAME_DEMO
            t->option = clamp_i32((i32)t->option + dy, 0, 2);
#endif

            switch (t->option) {
            case 0: t->comp_btnID = TITLE_BTN_SLOT_1 + t->selected; break;
            case 1: t->comp_btnID = TITLE_BTN_SLOT_COPY; break;
            case 2: t->comp_btnID = TITLE_BTN_SLOT_DELETE; break;
            }
        }
        break;
    }
    case TITLE_ST_FILE_CPY: {
        if (inp_btn_jp(INP_A)) {
            snd_play(SNDID_MENU3, 1.f, 1.f);
            t->copy_to = t->option;

            savefile_s *s = &APP.save;

            err32 err_r = savefile_r(t->selected, s);
            if (err_r == 0) {
                err32 err_w = savefile_w(t->copy_to, s);
                if (err_w) {

                } else {
                }
            }
            t->option     = 0;
            t->comp_bump  = 1;
            t->comp_btnID = TITLE_BTN_SLOT_1;
            title_load_previews(t);
            title_to_state(t, TITLE_ST_FILE_SELECT, 0);
            for (i32 n = 0; n < 3; n++) {
                title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 1);
            }
            title_btn_toggle(t, TITLE_BTN_SPEEDRUN, 1);
            title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 0);
            title_btn_toggle(t, TITLE_BTN_SLOT_COPY, 0);
        } else if (inp_btn_jp(INP_B)) {
            t->option     = 1;
            t->comp_btnID = TITLE_BTN_SLOT_COPY;

            for (i32 n = 0; n < 3; n++) {
                if (n != t->selected) {
                    title_btn_toggle(t, TITLE_BTN_SLOT_1 + n, 0);
                }
            }
            title_to_state(t, TITLE_ST_FILE_SELECTED, 0);
            title_btn_toggle(t, TITLE_BTN_SLOT_DELETE, 1);
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

            t->comp_btnID = TITLE_BTN_SLOT_1 + t->option;
        }
        break;
    }
    case TITLE_ST_TO_GAME: {
        t->start_tick++;
        if (TITLE_START_TICKS <= t->start_tick) {
            title_gameplay_start(app);
        }
        break;
    }
    default: break;
    }

    if (0 <= t->preload_slot) {
        inp_state_s istate = {0};
        game_tick(&APP.game, istate);
    }

    switch (t->preload_fade_dir) {
    case 0: {
        t->preload_fade_q7 = max_i32((i32)t->preload_fade_q7 - 4, 0);
        if (t->preload_fade_q7 == 0) {
            t->preload_slot = -1;
        }
        break;
    }
    case 1: {
        t->preload_fade_q7 = min_i32((i32)t->preload_fade_q7 + 4, 128);
        break;
    }
    }
#endif
}

void title_draw_btn(title_s *t, i32 ID)
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    texrec_s  tbut = asset_texrec(TEXID_COVER, 0, 0, 0, 0);
    v2_i32    pbut = title_btn_pos(t, ID);
    fnt_s     font = asset_fnt(FNTID_MEDIUM);

    switch (ID) {
    case TITLE_BTN_SLOT_1:
    case TITLE_BTN_SLOT_2:
    case TITLE_BTN_SLOT_3: {
#if GAME_DEMO
        if (ID != TITLE_BTN_SLOT_1) return;
#endif
        tbut.y = 384;
        tbut.x += 5 * 32;
        tbut.w = 5 * 32;
        tbut.h = 1 * 32;

        save_preview_s *sp = &t->saves[ID - TITLE_BTN_SLOT_1];
        if (!sp->exists) {
            tbut.y += 32;
        }
        break;
    }
    case TITLE_BTN_SPEEDRUN: {
        tbut.y = 384 + 32;
        tbut.w = 4 * 32;
        tbut.h = 1 * 32;
#if GAME_DEMO
        return;
#endif
        break;
    }
    case TITLE_BTN_SLOT_COPY: {
        tbut.y = 448;
        tbut.w = 3 * 32;
        tbut.h = 1 * 32;
#if GAME_DEMO
        return;
#endif
        break;
    }
    case TITLE_BTN_SLOT_DELETE: {
        tbut.y = 448;
        tbut.w = 3 * 32;
        tbut.h = 1 * 32;
#if GAME_DEMO
        return;
#endif
        break;
    }
    }

    gfx_spr(ctx, tbut, pbut, 0, 0);

    switch (ID) {
    case TITLE_BTN_SLOT_1:
    case TITLE_BTN_SLOT_2:
    case TITLE_BTN_SLOT_3: {
        save_preview_s *sp    = &t->saves[ID - TITLE_BTN_SLOT_1];
        v2_i32          fpos1 = {pbut.x + 9, pbut.y + 4};
        v2_i32          fpos2 = {fpos1.x + 19, fpos1.y};

        if (sp->exists) {
            fnt_draw_outline_style(ctx, font, fpos2, sp->name, 3, 0);
        }
        char snum[2] = {'1' + (ID - TITLE_BTN_SLOT_1), 0};
        fnt_draw_outline_style(ctx, font, fpos1, snum, 4, 0);
        break;
    }
    case TITLE_BTN_SPEEDRUN: {
        v2_i32 fpos = {pbut.x + 10, pbut.y + 4};
        fnt_draw_outline_style(ctx, font, fpos, "Speedrun", 3, 0);
        break;
    }
    case TITLE_BTN_SLOT_COPY: {
        v2_i32 fpos = {pbut.x + 10, pbut.y + 4};
        fnt_draw_outline_style(ctx, font, fpos, "Copy", 3, 0);
        break;
    }
    case TITLE_BTN_SLOT_DELETE: {
        v2_i32 fpos = {pbut.x + 10, pbut.y + 4};
        fnt_draw_outline_style(ctx, font, fpos, "Delete", 3, 0);
        break;
    }
    }
}

void title_render(title_s *t)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);
    gfx_ctx_s ctxcov   = ctx;
    fnt_s     font     = asset_fnt(FNTID_LARGE);
    rec_i32   rfull    = {0, 0, 400, 240};
    tex_s     tex      = asset_tex(TEXID_DISPLAY_TMP);
    tex_clr(tdisplay, GFX_COL_BLACK);

    if (t->state == TITLE_ST_OVERVIEW) {
        texrec_s trcover = asset_texrec(TEXID_COVER, 0, 0, 400, 240);
        gfx_spr(ctx, trcover, (v2_i32){0, 0}, 0, 0);

        i32 ti1 = (i32)t->tick - 0;
        if (0 <= ti1) {
            texrec_s trlogo   = asset_texrec(TEXID_COVER, 0, 256, 7 * 32, 4 * 32);
            v2_i32   logo_pos = {92, 40};
            logo_pos.y += (3 * sin_q15(ti1 << 10)) / 32769;
            gfx_ctx_s ctxlogo = ctx;
            ctxlogo.pat       = gfx_pattern_interpolate(min_i32(ti1, TITLE_INTRO_LOGO_TICKS),
                                                        TITLE_INTRO_LOGO_TICKS);
            gfx_spr(ctxlogo, trlogo, logo_pos, 0, 0);
        }

        if (t->title_subst) {
            fnt_draw_outline_style(ctx, font, (v2_i32){200, 168},
                                   "Start", 2, 1);
            fnt_draw_outline_style(ctx, font, (v2_i32){200, 190},
                                   "Settings", 2, 1);
            fnt_draw_outline_style(ctx, font, (v2_i32){200, 212},
                                   "Credits", 2, 1);
            i32      fr_cursor  = ani_frame(ANIID_CURSOR, t->tick);
            texrec_s tr_cursor  = asset_texrec(TEXID_COVER, 352 + fr_cursor * 32, 384, 32, 32);
            v2_i32   pt_cursor1 = {160 - 16, 168 + 10 + 22 * t->option - 16};
            v2_i32   pt_cursor2 = {240 - 16, 168 + 10 + 22 * t->option - 16};
            gfx_spr(ctx, tr_cursor, pt_cursor1, 0, 0);
            gfx_spr(ctx, tr_cursor, pt_cursor2, SPR_FLIP_X, 0);
        } else {
            i32 ti2 = (i32)t->tick - 20;
            if (0 <= ti2) {
                gfx_ctx_s ctxs = ctx;
                i32       fs   = cos_q15(65536 + (ti2 << 10)) + 32768;
                i32       pt   = lerp_i32(0, GFX_PATTERN_MAX, min_i32(fs, 32768), 32768);

                ctxs.pat = gfx_pattern_bayer_4x4(pt);

                u8 fntstr[32] = {0};
                str_cpy(fntstr, "- Press ");
                str_append_char(fntstr, FNT_GLYPH_BTN_A);
                str_append(fntstr, " -");
                fnt_draw_outline_style(ctxs, font, (v2_i32){200, 190},
                                       fntstr, 2, 1);
            }
        }

    } else if (0 <= t->preload_slot) {
        game_draw(&APP.game);
        mcpy(tex.px, ctx.dst.px, sizeof(u32) * tex.wword * tex.h);

        i32 ti    = min_i32(t->start_tick, TITLE_START_SLIDE_TICKS);
        i32 xoffs = ease_in_out_quad(100, 0, ti, TITLE_START_SLIDE_TICKS) & ~1;

        gfx_spr(ctx, texrec_from_tex(tex), (v2_i32){xoffs, 0}, 0, 0);
        rec_i32   rfill   = {0, 0, xoffs, 240};
        rec_i32   rfade   = {xoffs, 0, 400, 240};
        gfx_ctx_s ctxfade = ctx;
        ctxfade.pat       = gfx_pattern_interpolate(128 - t->preload_fade_q7, 128);
        gfx_rec_fill(ctx, rfill, PRIM_MODE_BLACK);
        gfx_rec_fill(ctxfade, rfade, PRIM_MODE_BLACK);

        // left gradient
        texrec_s  trgradient = asset_texrec(TEXID_COVER, 832, 0, 128, 32);
        gfx_ctx_s ctxgrad    = ctx;

        if (TITLE_START_TICKS - TITLE_START_FADE_TICKS <= t->start_tick) {
            i32 ti = t->start_tick -
                     (TITLE_START_TICKS - TITLE_START_FADE_TICKS);
            ctxgrad.pat = gfx_pattern_interpolate(TITLE_START_FADE_TICKS - ti,
                                                  TITLE_START_FADE_TICKS);
        }
        for (i32 n = 0; n < 8; n++) {
            v2_i32 posgrad = {xoffs, n * 32};
            gfx_spr(ctxgrad, trgradient, posgrad, 0, SPR_MODE_BLACK);
        }
    }

    if (t->state != TITLE_ST_OVERVIEW) {
        for (i32 n = 0; n < NUM_TITLE_BTNS; n++) {
            title_draw_btn(t, n);
        }
        title_draw_companion(t);
    }

    v2_i32 phead = {12, 8};
    fnt_s  fhead = asset_fnt(FNTID_LARGE);
    i32    shead = 3;

    switch (t->state) {
    case TITLE_ST_OVERVIEW: {
        title_draw_version();
        break;
    }
    case TITLE_ST_SETTINGS: {
        break;
    }
    case TITLE_ST_CREDITS: {
        break;
    }
    case TITLE_ST_FILE_SELECT: {
        fnt_draw_outline_style(ctx, fhead, phead, "Select file", shead, 0);
        break;
    }
    case TITLE_ST_FILE_SELECTED: {
        fnt_draw_outline_style(ctx, fhead, phead, "Start file?", shead, 0);
        break;
    }
    case TITLE_ST_FILE_CPY: {
        fnt_draw_outline_style(ctx, fhead, phead, "Copy to", shead, 0);
        break;
    }
    case TITLE_ST_FILE_CPY_CONFIRM: {
        break;
    }
    case TITLE_ST_FILE_DEL_CONFIRM: {
        break;
    }
    }

    i32 patIDlog = lerp_i32(0, GFX_PATTERN_MAX,
                            min_i32(t->title_fade, TITLE_FADE_TICKS_TO_BLACK),
                            TITLE_FADE_TICKS_TO_BLACK);
    ctxcov.pat   = gfx_pattern_bayer_4x4(patIDlog);
    gfx_rec_fill(ctxcov, (rec_i32){0, 0, 400, 240}, PRIM_MODE_BLACK);
}

void title_load_previews(title_s *t)
{
    savefile_s *s = &APP.save;

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
}

void title_draw_companion(title_s *t)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    i32 frx = ani_frame(ANIID_COMPANION_FLY, t->tick);
    i32 fry = 0;
    i32 x   = t->pos_comp_q8.x >> 8;
    i32 y   = t->pos_comp_q8.y >> 8;

    texrec_s tr = asset_texrec(TEXID_COMPANION, frx * 96, fry * 64, 96, 64);

    if (t->comp_bump) {
        tr.y = 64 * 6;
        tr.x = 96 * ani_frame(ANIID_COMPANION_BUMP, t->comp_bump);
    }
    v2_i32 pos = {x - 48 + 6, y - 32};

    if (t->start_tick) {
        pos.x -= t->start_tick * 16;
    }

    gfx_spr(ctx, tr, pos, SPR_FLIP_X, 0);
}

void title_draw_version()
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    texrec_s  tnum = asset_texrec(TEXID_BUTTONS, 0, 224, 8, 16);

    i32 lv = 7 + 4 + 7 * (10 <= GAME_V_MIN ? 2 : 1) + 4 +
             7 * (10 <= GAME_V_PAT ? 2 : 1);
    v2_i32 pnum = {397 - lv, 227};

    // major
    tnum.x = 8 * (GAME_V_MAJ);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 7;
    // dot
    tnum.x = 80;
    gfx_spr_tile_32x32(ctx, tnum, pnum); // "."
    pnum.x += 4;
    // minor
    if (10 <= GAME_V_MIN) {
        tnum.x = 8 * (GAME_V_MIN / 10);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 7;
    }
    tnum.x = 8 * (GAME_V_MIN % 10);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 7;
    // dot
    tnum.x = 80;
    gfx_spr_tile_32x32(ctx, tnum, pnum); // "."
    pnum.x += 4;

    // patch
    if (100 <= GAME_V_PAT) {
        tnum.x = 8 * (GAME_V_PAT / 100);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 7;
    }
    if (10 <= GAME_V_PAT) {
        tnum.x = 8 * ((GAME_V_PAT / 10) % 10);
        gfx_spr_tile_32x32(ctx, tnum, pnum);
        pnum.x += 7;
    }
    tnum.x = 8 * (GAME_V_PAT % 10);
    gfx_spr_tile_32x32(ctx, tnum, pnum);
    pnum.x += 7;
}

v2_i32 title_btn_pos(title_s *t, i32 ID)
{
    title_btn_s *b = &t->buttons[ID];
    v2_i32       p = {0};

    if (b->lerp_tick_max) {
        i32 ti = (i32)b->lerp_tick;
        i32 tj = (i32)b->lerp_tick_max;

        p.x = ease_in_out_quad(b->x_src, b->x_dst, ti, tj);
        p.y = ease_in_out_quad(b->y_src, b->y_dst, ti, tj);
    } else {
        p.x = b->x_dst;
        p.y = b->y_dst;
    }
    p.x &= ~1;
    p.y &= ~1;
    return p;
}

bool32 title_btn_pos_companion(title_s *t, i32 ID, v2_i32 *pos)
{
    title_btn_s *b = &t->buttons[ID];
    pos->x         = b->x_dst;
    pos->y         = b->y_dst;
    bool32 haspos  = 1;

    switch (ID) {
    default: haspos = 0; break;
    case TITLE_BTN_SLOT_1:
    case TITLE_BTN_SLOT_2:
    case TITLE_BTN_SLOT_3: {

        break;
    }
    case TITLE_BTN_SLOT_COPY: {

        break;
    }
    case TITLE_BTN_SLOT_DELETE: {

        break;
    }
    }

    pos->x -= 20;
    pos->y += 16;
    return haspos;
}

void title_btn_move_to(title_btn_s *b, i32 x, i32 y, i32 ticks)
{
    b->lerp_tick     = 0;
    b->lerp_tick_max = ticks;
    b->x_src         = b->x_dst;
    b->y_src         = b->y_dst;
    b->x_dst         = x;
    b->y_dst         = y;
}

void title_btn_toggle_ext(title_s *t, i32 ID, i32 show, i32 setto)
{
#define TITLE_BTN_SLOT_X_SHOW    40
#define TITLE_BTN_SLOT_X_HIDE    -160
#define TITLE_BTN_SLOT_Y         42
#define TITLE_BTN_SLOT_Y_SPACING 36
#define TITLE_BTN_SLOT_TICKS     8

    title_btn_s *b = &t->buttons[ID];
    switch (ID) {
    case TITLE_BTN_SLOT_1: {
        if (show) {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_SHOW,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 0, TITLE_BTN_SLOT_TICKS);
        } else {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_HIDE,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 0, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    case TITLE_BTN_SLOT_2: {
        if (show) {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_SHOW,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 1, TITLE_BTN_SLOT_TICKS);
        } else {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_HIDE,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 1, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    case TITLE_BTN_SLOT_3: {
        if (show) {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_SHOW,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 2, TITLE_BTN_SLOT_TICKS);
        } else {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_HIDE,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 2, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    case TITLE_BTN_SPEEDRUN: {
        if (show) {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_SHOW,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 3, TITLE_BTN_SLOT_TICKS);
        } else {
            title_btn_move_to(b, TITLE_BTN_SLOT_X_HIDE,
                              TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 3, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    case TITLE_BTN_SLOT_COPY: {
        if (show) {
            title_btn_move_to(b, 60, TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 3, 10);
        } else {
            title_btn_move_to(b, 60, TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 3 + 120, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    case TITLE_BTN_SLOT_DELETE: {
        if (show) {
            title_btn_move_to(b, 60, TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 4, 10);
        } else {
            title_btn_move_to(b, 60, TITLE_BTN_SLOT_Y + TITLE_BTN_SLOT_Y_SPACING * 4 + 120, TITLE_BTN_SLOT_TICKS);
        }
        break;
    }
    }

    if (setto) {
        b->lerp_tick = b->lerp_tick_max;
    }
}

void title_btn_toggle(title_s *t, i32 ID, i32 show)
{
    title_btn_toggle_ext(t, ID, show, 0);
}

void title_paused(title_s *t)
{
#if PLTF_PD
    tex_s tex = asset_tex(0);
    pltf_pd_menu_image_upd(tex.px, tex.wword, tex.w, tex.h);
    if (t->state == TITLE_ST_OVERVIEW) {
        pltf_pd_menu_image_put(100);
    } else {
        pltf_pd_menu_image_put(200);
    }
#endif
}