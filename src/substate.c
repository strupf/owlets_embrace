// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "substate.h"
#include "game.h"

void   respawn_start(game_s *g, respawn_s *rs);
bool32 respawn_finished(respawn_s *rs);
bool32 respawn_blocks_gameplay(respawn_s *rs);
void   respawn_update(game_s *g, respawn_s *rs);
void   respawn_draw(game_s *g, respawn_s *rs);
//
bool32 substate_blocks_gameplay(substate_s *st);
bool32 substate_finished(substate_s *st);
void   substate_update(game_s *g, substate_s *st);
void   substate_draw(game_s *g, substate_s *st, v2_i32 cam);
//
void   upgrade_start_animation(upgrade_s *h, int upgrade);
bool32 upgrade_finished(upgrade_s *h);
void   upgrade_tick(upgrade_s *h);
void   upgrade_draw(game_s *g, upgrade_s *h, v2_i32 camoffset);
//
void   transition_teleport(transition_s *t, game_s *g, const char *mapfile, v2_i32 hero_feet);
bool32 transition_check_hero_slide(transition_s *t, game_s *g, int *touched);
void   transition_update(game_s *g, transition_s *t);
bool32 transition_blocks_gameplay(transition_s *t);
bool32 transition_finished(transition_s *t);
void   transition_draw(game_s *g, transition_s *t, v2_i32 camoffset);
void   transition_start_respawn(game_s *g, transition_s *t);

bool32 substate_blocks_gameplay(substate_s *st)
{
    switch (st->state) {
    case SUBSTATE_RESPAWN:
        return respawn_blocks_gameplay(&st->respawn);
    case SUBSTATE_TRANSITION:
        return transition_blocks_gameplay(&st->transition);
    case SUBSTATE_UPGRADE:
        return !upgrade_finished(&st->upgrade);
    case SUBSTATE_TEXTBOX: return 1;
    case SUBSTATE_MAINMENU_FADE_IN: return 0;
    case SUBSTATE_FREEZE: return 1;
    }
    return 0;
}

bool32 substate_finished(substate_s *st)
{
    switch (st->state) {
    case SUBSTATE_RESPAWN:
        return respawn_finished(&st->respawn);
    case SUBSTATE_TRANSITION:
        return transition_finished(&st->transition);
    case SUBSTATE_UPGRADE:
        return upgrade_finished(&st->upgrade);
    case SUBSTATE_TEXTBOX:
        return textbox_finished(&st->textbox);
    case SUBSTATE_MAINMENU_FADE_IN: return 0;
    case SUBSTATE_FREEZE: return (st->freeze_tick <= 0);
    }
    return 1;
}

void substate_update(game_s *g, substate_s *st)
{
    switch (st->state) {
    case SUBSTATE_RESPAWN:
        respawn_update(g, &st->respawn);
        break;
    case SUBSTATE_TRANSITION:
        transition_update(g, &st->transition);
        break;
    case SUBSTATE_UPGRADE:
        upgrade_tick(&st->upgrade);
        break;
    case SUBSTATE_TEXTBOX:
        textbox_update(g, &st->textbox);
        break;
    case SUBSTATE_MAINMENU_FADE_IN:
        st->mainmenu_tick++;
        if (FADETICKS_GAME_IN <= st->mainmenu_tick) {
            st->mainmenu_tick = 0;
            st->state         = 0;
        }
        break;
    case SUBSTATE_FREEZE: st->freeze_tick--; break;
    }

    if (substate_finished(st)) {
        st->state = 0;
    }
}

void substate_draw(game_s *g, substate_s *st, v2_i32 cam)
{
    switch (st->state) {
    case SUBSTATE_RESPAWN:
        respawn_draw(g, &st->respawn);
        break;
    case SUBSTATE_TRANSITION:
        transition_draw(g, &st->transition, cam);
        break;
    case SUBSTATE_UPGRADE:
        upgrade_draw(g, &st->upgrade, cam);
        break;
    case SUBSTATE_TEXTBOX:
        textbox_draw(&st->textbox, cam);
        break;
    case SUBSTATE_MAINMENU_FADE_IN: {
        gfx_ctx_s ctx = gfx_ctx_display();
        int       t   = FADETICKS_GAME_IN - st->mainmenu_tick;
        ctx.pat       = gfx_pattern_interpolate(t, FADETICKS_GAME_IN);
        gfx_rec_fill(ctx, (rec_i32){0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H}, PRIM_MODE_BLACK);
    } break;
    }
}

void substate_transition_teleport(game_s *g, substate_s *st, const char *map, v2_i32 hero_feet)
{
    assert(g->substate.state == 0);
    st->state = SUBSTATE_TRANSITION;
    transition_teleport(&st->transition, g, map, hero_feet);
}

bool32 substate_transition_try_hero_slide(game_s *g, substate_s *st)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero || ohero->health <= 0) return 0;

    int touched;
    int r = transition_check_hero_slide(&st->transition, g, &touched);
    if (r) {
        st->state = SUBSTATE_TRANSITION;
        return 1;
    } else if (touched == DIRECTION_S) {
        hero_kill(g, ohero);
        return 1;
    }
    return 0;
}

void substate_upgrade_collected(game_s *g, substate_s *st, int upgrade)
{
    assert(g->substate.state == 0);
    st->state = SUBSTATE_UPGRADE;
    upgrade_start_animation(&st->upgrade, upgrade);
}

void substate_respawn(game_s *g, substate_s *st)
{
    st->state = SUBSTATE_RESPAWN;
    respawn_start(g, &st->respawn);
}

void substate_load_textbox(game_s *g, substate_s *st, const char *filename)
{
    st->state = SUBSTATE_TEXTBOX;
    textbox_load_dialog(g, &st->textbox, filename);
}

void substate_freeze(substate_s *st, int ticks)
{
    if (st->state == 0 && 0 < ticks) {
        st->state       = SUBSTATE_FREEZE;
        st->freeze_tick = ticks;
    }
}

// RESPAWN =====================================================================

enum {
    RESPAWN_NONE,
    RESPAWN_DYING,
    RESPAWN_GAMEOVER_FADE_IN,
    RESPAWN_GAMEOVER,
    RESPAWN_GAMEOVER_INPUT,
    RESPAWN_GAMEOVER_FADE_OUT,
    RESPAWN_BLACK,
    RESPAWN_FADE,
    //
    NUM_RESPAWN_PHASES
};

static int respawn_ticks(respawn_s *rs)
{
    static int phase_ticks[NUM_RESPAWN_PHASES] = {
        0,
        30, // dying
        20, // fade gameover
        80, // gameover
        10, // gameover input
        10, // fade gameover
        10, // black
        25  // fade
    };
    assert(0 <= rs->phase && rs->phase < NUM_RESPAWN_PHASES);
    return phase_ticks[rs->phase];
}

bool32 respawn_finished(respawn_s *rs)
{
    return (rs->phase == RESPAWN_NONE);
}

bool32 respawn_blocks_gameplay(respawn_s *rs)
{
    switch (rs->phase) {
    case RESPAWN_DYING:
    case RESPAWN_GAMEOVER_FADE_IN:
    case RESPAWN_GAMEOVER:
    case RESPAWN_GAMEOVER_INPUT:
    case RESPAWN_GAMEOVER_FADE_OUT:
    case RESPAWN_FADE:
        return 0;
    }
    return 1;
}

void respawn_start(game_s *g, respawn_s *rs)
{
    assert(0 <= g->respawn_closest);
    rs->phase = RESPAWN_DYING;
    rs->tick  = 0;
}

void respawn_update(game_s *g, respawn_s *rs)
{
    assert(0 <= rs->phase && rs->phase < NUM_RESPAWN_PHASES);
    if (rs->phase == 0) return;

    if (rs->phase == RESPAWN_GAMEOVER_INPUT) {
        if (inp_just_pressed(INP_A) || 1) {
            rs->tick = respawn_ticks(rs);
        } else {
            return;
        }
    }

    rs->tick++;
    if (rs->tick < respawn_ticks(rs)) return;

    rs->tick = 0;
    rs->phase++;
    rs->phase %= NUM_RESPAWN_PHASES;

    if (rs->phase == RESPAWN_BLACK) {
        // load map and setup
        game_load_map(g, g->areaname.filename);
        obj_s        *ohero = hero_create(g);
        respawn_pos_s rp    = g->respawns[g->respawn_closest];
        ohero->pos.x        = rp.r.x + rp.r.w / 2 - ohero->w / 2;
        ohero->pos.y        = rp.r.y + rp.r.h - ohero->h;

        aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
        for (obj_each(g, o)) {
            obj_game_animate(g, o); // just setting initial sprites for obj
        }
        aud_allow_playing_new_snd(1);

        cam_s *cam = &g->cam;
        cam_set_pos_px(cam, ohero->pos.x, ohero->pos.y);
        cam_init_level(g, cam);
    }
}

void respawn_draw(game_s *g, respawn_s *rs)
{
    assert(0 <= rs->phase && rs->phase < NUM_RESPAWN_PHASES);
    if (rs->phase == 0) return;
    const gfx_ctx_s ctx      = gfx_ctx_display();
    const int       ticks    = respawn_ticks(rs);
    const rec_i32   rdisplay = {0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H};

    gfx_ctx_s ctx_r      = ctx;
    int       p_gameover = 12;

    switch (rs->phase) {
    case RESPAWN_DYING: {
        int p     = lerp_i32(0, p_gameover, rs->tick, ticks);
        ctx_r.pat = gfx_pattern_bayer_4x4(p);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case RESPAWN_GAMEOVER_FADE_IN:
    case RESPAWN_GAMEOVER:
    case RESPAWN_GAMEOVER_INPUT: {
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

        if (rs->phase == RESPAWN_GAMEOVER_INPUT) {
        }
        break;
    }
    case RESPAWN_GAMEOVER_FADE_OUT: {
        int p     = lerp_i32(p_gameover, GFX_PATTERN_MAX, rs->tick, ticks);
        ctx_r.pat = gfx_pattern_bayer_4x4(p);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case RESPAWN_BLACK: {
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case RESPAWN_FADE: {
        ctx_r.pat = gfx_pattern_interpolate(ticks - rs->tick, ticks);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    }
}

// UPGRADE =====================================================================

#define UPGRADE_DESC_LINES 4

typedef struct {
    char name[32];
    char line[UPGRADE_DESC_LINES][64];
} upgrade_text_s;

static const upgrade_text_s g_upgrade_text[NUM_HERO_UPGRADES];

enum {
    UPGRADE_NONE,
    UPGRADE_TO_WHITE,
    UPGRADE_BARS_TO_BLACK,
    UPGRADE_TEXT_FADE_IN,
    UPGRADE_TEXT,
    UPGRADE_TEXT_WAIT,
    UPGRADE_FADE_OUT,
    //
    UPGRADE_NUM_PHASES
};

static int upgrade_phase_ticks(upgrade_s *h)
{
    static const int phase_ticks[UPGRADE_NUM_PHASES] = {
        0,
        50, // to white
        20, // to black
        30, // text fade in
        0,  // wait for input
        10, // delay fade out
        30  // fade out
    };
    return phase_ticks[h->phase];
}

void upgrade_start_animation(upgrade_s *h, int upgrade)
{
    h->t       = 0;
    h->phase   = 1;
    h->upgrade = upgrade;
    snd_play_ext(SNDID_UPGRADE, 1.f, 2.f);
}

bool32 upgrade_finished(upgrade_s *h)
{
    return (h->phase == 0);
}

void upgrade_tick(upgrade_s *h)
{
    if (h->phase == UPGRADE_TEXT) {
        if (inp_just_pressed(INP_A)) {
            snd_play_ext(SNDID_SELECT, 0.5, 1.f);
        } else {
            return;
        }
    }

    const int ticks = upgrade_phase_ticks(h);
    h->t++;
    if (h->t < ticks) return;
    h->t = 0;
    h->phase++;
    h->phase %= UPGRADE_NUM_PHASES;
}

void upgrade_draw(game_s *g, upgrade_s *h, v2_i32 camoffset)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;

    const int       ticks = upgrade_phase_ticks(h);
    const gfx_ctx_s ctx   = gfx_ctx_display();
    gfx_ctx_s       ctx_1 = ctx;
    gfx_ctx_s       ctx_2 = ctx;

    const upgrade_text_s *ut       = &g_upgrade_text[h->upgrade];
    rec_i32               rdisplay = {0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H};

    switch (h->phase) {
    case UPGRADE_TO_WHITE: { // growing white circle
        ctx_1.pat      = gfx_pattern_interpolate(h->t, ticks);
        v2_i32 drawpos = obj_pos_center(ohero);
        drawpos        = v2_add(drawpos, camoffset);
        int dtt        = 3 * max_i(abs_i(drawpos.x),
                                   abs_i(SYS_DISPLAY_W - drawpos.x));
        int cd         = (dtt * h->t) / ticks;

        gfx_rec_fill(ctx_1, rdisplay, PRIM_MODE_WHITE);
        gfx_cir_fill(ctx, drawpos, cd, PRIM_MODE_WHITE);
        break;
    }
    case UPGRADE_BARS_TO_BLACK: { // black bars closing to black screen
        gfx_rec_fill(ctx_1, rdisplay, PRIM_MODE_WHITE);
        int rh = ease_out_quad(0, 120, h->t, ticks);
        gfx_rec_fill(ctx_1,
                     (rec_i32){0, 0, SYS_DISPLAY_W, rh},
                     PRIM_MODE_BLACK);
        gfx_rec_fill(ctx_1,
                     (rec_i32){0, SYS_DISPLAY_H - rh, SYS_DISPLAY_W, rh},
                     PRIM_MODE_BLACK);
        break;
    }
    case UPGRADE_FADE_OUT:       // fade back to gameplay; fallthrough
    case UPGRADE_TEXT_FADE_IN: { // fade in upgrade text
        switch (h->phase) {
        case UPGRADE_FADE_OUT:
            ctx_1.pat = gfx_pattern_interpolate(ticks - h->t, ticks);
            ctx_2     = ctx_1;
            break;
        case UPGRADE_TEXT_FADE_IN:
            ctx_1.pat = gfx_pattern_interpolate(h->t, ticks);
            break;
        }
    } // fallthrough
    case UPGRADE_TEXT_WAIT:
    case UPGRADE_TEXT: { // show text
        gfx_rec_fill(ctx_2, rdisplay, PRIM_MODE_BLACK);

        fnt_s font1 = asset_fnt(FNTID_LARGE);
        fnt_s font2 = asset_fnt(FNTID_MEDIUM);
        int   l1    = fnt_length_px(font1, ut->name);

        fnt_draw_ascii(ctx_1, font1,
                       (v2_i32){(SYS_DISPLAY_W - l1) / 2, 40},
                       ut->name, SPR_MODE_WHITE);

        for (int i = 0; i < UPGRADE_DESC_LINES; i++) {
            int li = fnt_length_px(font2, ut->line[i]);
            fnt_draw_ascii(ctx_1, font2,
                           (v2_i32){(SYS_DISPLAY_W - li) / 2, 70 + i * 25},
                           ut->line[i], SPR_MODE_WHITE);
        }

        texrec_s tr_a = asset_texrec(TEXID_UI, 0, 0, 32, 32);
        tr_a.r.y      = 32 * ((sys_tick() >> 5) & 1);
        gfx_spr(ctx_1, tr_a, (v2_i32){350, 200}, 0, 0);
        break;
    }
    }
}

// TRANSITION ==================================================================

enum {
    TRANSITION_FADE_NONE,
    TRANSITION_FADE_OUT,
    TRANSITION_BLACK,
    TRANSITION_FADE_IN,
    //
    NUM_TRANSITION_PHASES
};

static int transition_ticks(transition_s *t)
{
    static const int g_transition_ticks[NUM_TRANSITION_PHASES] = {
        0,
        20,
        40,
        20};
    return g_transition_ticks[t->fade_phase];
}

static void transition_start(transition_s *t, game_s *g, const char *file,
                             int type, v2_i32 hero_feet, v2_i32 hero_v, int facing)
{
    t->dir       = 0;
    t->type      = type;
    t->hero_feet = hero_feet;
    t->hero_v    = hero_v;
    t->hero_face = facing;
    str_cpy(t->to_load, file);
    t->fade_tick  = 0;
    t->fade_phase = TRANSITION_FADE_OUT;
}

void transition_teleport(transition_s *t, game_s *g, const char *mapfile, v2_i32 hero_feet)
{
    transition_start(t, g, mapfile, 0, hero_feet, (v2_i32){0}, 1);
}

bool32 transition_check_hero_slide(transition_s *t, game_s *g, int *touched)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o || o->health <= 0) return 0;
    if (!g->map_worldroom) {
        BAD_PATH
        return 0;
    }

    int touchedbounds = 0;
    if (o->pos.x <= 0)
        touchedbounds = DIRECTION_W;
    if (g->pixel_x <= o->pos.x + o->w)
        touchedbounds = DIRECTION_E;
    if (o->pos.y <= 0)
        touchedbounds = DIRECTION_N;
    if (g->pixel_y <= o->pos.y + o->h)
        touchedbounds = DIRECTION_S;

    *touched = touchedbounds;
    if (!touchedbounds) return 0;

    rec_i32 aabb = obj_aabb(o);
    v2_i32  vdir = direction_v2(touchedbounds);
    aabb.x += vdir.x + g->map_worldroom->x;
    aabb.y += vdir.y + g->map_worldroom->y;

    map_worldroom_s *nextroom = map_world_overlapped_room(&g->map_world, g->map_worldroom, aabb);

    if (!nextroom) {
        sys_printf("no room\n");
        return 0;
    }

    rec_i32 nr      = {nextroom->x, nextroom->y, nextroom->w, nextroom->h};
    rec_i32 trgaabb = obj_aabb(o);
    trgaabb.x += g->map_worldroom->x - nr.x;
    trgaabb.y += g->map_worldroom->y - nr.y;

    v2_i32 hvel = o->vel_q8;
    switch (touchedbounds) {
    case DIRECTION_E:
        trgaabb.x = 8;
        break;
    case DIRECTION_W:
        trgaabb.x = nr.w - trgaabb.w - 8;
        break;
    case DIRECTION_N:
        trgaabb.y = nr.h - trgaabb.h - 8;
        hvel.y    = min_i(hvel.y, -1200);
        break;
    case DIRECTION_S:
        trgaabb.y = 8;
        hvel.y    = 256;
        break;
    }

    v2_i32 feet = {trgaabb.x + trgaabb.w / 2, trgaabb.y + trgaabb.h};

    transition_start(t, g, nextroom->filename, 0, feet, hvel, o->facing);
    t->dir = touchedbounds;
    return 1;
}

void transition_start_respawn(game_s *g, transition_s *t)
{
    hero_s h = *((hero_s *)obj_get_tagged(g, OBJ_TAG_HERO)->mem);

    game_load_map(g, t->to_load);
    obj_s  *hero     = hero_create(g);
    hero_s *hh       = (hero_s *)hero->mem;
    hero->pos.x      = t->hero_feet.x - hero->w / 2;
    hero->pos.y      = t->hero_feet.y - hero->h;
    hero->facing     = t->hero_face;
    hero->vel_q8     = t->hero_v;
    hh->sprinting    = h.sprinting;
    hh->sprint_ticks = h.sprint_ticks;
    v2_i32 hpos      = obj_pos_center(hero);

    u32 respawn_d      = U32_MAX;
    g->respawn_closest = -1;
    for (int n = 0; n < g->n_respawns; n++) {
        respawn_pos_s *r  = &g->respawns[n];
        v2_i32         rp = {r->r.x + r->r.w / 2,
                             r->r.y + r->r.h / 2};
        u32            d  = v2_distancesq(hpos, rp);
        if (d < respawn_d) {
            respawn_d          = d;
            g->respawn_closest = n;
        }
    }

    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    for (obj_each(g, o)) {
        obj_game_animate(g, o); // just setting initial sprites for obj
    }
    aud_allow_playing_new_snd(1);

    cam_s *cam = &g->cam;
    cam_set_pos_px(cam, hpos.x, hpos.y);
    cam_init_level(g, cam);
}

void transition_update(game_s *g, transition_s *t)
{
    if (transition_finished(t)) return;

    const int ticks = transition_ticks(t);
    t->fade_tick++;
    if (t->fade_tick < ticks) return;

    t->fade_tick = 0;
    t->fade_phase++;
    t->fade_phase %= NUM_TRANSITION_PHASES;

    switch (t->fade_phase) {
    case TRANSITION_BLACK: {
        transition_start_respawn(g, t);
        break;
    }
    }
}

bool32 transition_blocks_gameplay(transition_s *t)
{
    return (t->fade_phase != 0 && t->fade_phase <= TRANSITION_BLACK);
}

bool32 transition_finished(transition_s *t)
{
    return (t->fade_phase == TRANSITION_FADE_NONE);
}

void transition_draw(game_s *g, transition_s *t, v2_i32 camoffset)
{
    const int     ticks = transition_ticks(t);
    gfx_pattern_s pat   = {0};

    switch (t->fade_phase) {
    case TRANSITION_FADE_OUT:
        pat = gfx_pattern_interpolate(t->fade_tick, ticks);
        break;
    case TRANSITION_BLACK: {
        pat = gfx_pattern_interpolate(1, 1);
    } break;
    case TRANSITION_FADE_IN:
        pat = gfx_pattern_interpolate(ticks - t->fade_tick, ticks);
        break;
    }

    spm_push();

    tex_s display = asset_tex(0);
    tex_s tmp     = tex_create_opaque(display.w, display.h, spm_allocator);

    for (int y = 0; y < tmp.h; y++) {
        u32 p = ~pat.p[y & 7];
        for (int x = 0; x < tmp.wword; x++) {
            tmp.px[x + y * tmp.wword] = p;
        }
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && TRANSITION_BLACK <= t->fade_phase) {
        gfx_ctx_s ctxc = gfx_ctx_default(tmp);
        v2_i32    cpos = v2_add(obj_pos_center(ohero), camoffset);
        int       cird = 200;

        if (t->fade_phase == TRANSITION_BLACK) {
            int ticksh = ticks >> 1;
            int ft     = t->fade_tick - ticksh;
            if (0 <= ft) {
                ctxc.pat = gfx_pattern_interpolate(ft, ticksh);
                cird     = ease_out_quad(0, cird, ft, ticksh);
            } else {
                cird = 0;
            }
        }

        gfx_cir_fill(ctxc, cpos, cird, PRIM_MODE_WHITE);
    }
    for (int y = 0; y < tmp.h; y++) {
        for (int x = 0; x < tmp.wword; x++) {
            int i = x + y * tmp.wword;
            display.px[i] &= tmp.px[i];
        }
    }

    spm_pop();
}

// =============================================================================
static const upgrade_text_s g_upgrade_text[NUM_HERO_UPGRADES] =
    {
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"Air Jump",
         {"Your confidence grew. Maybe you can try to",
          "put those wings to use."}},
        //
        {"Grappling Hook",
         {"Maybe this tool can be",
          "put to good use!",
          "Use the crank to switch between whip",
          "and grappling hook."}},
        //
        {"Grappling Hook",
         {"Maybe this tool can be",
          "put to good use!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"Air Jump",
         {"Your confidence grew. You gained an additional",
          "jump when being midair."}},
        //
        {"Air Jump",
         {"Your confidence grew. Maybe you can try to",
          "put those wings to use."}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
};