// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "transition.h"
#include "game.h"

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

void transition_check_hero_slide(transition_s *t, game_s *g)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o) return;
    if (!g->map_worldroom) {
        BAD_PATH
        return;
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

    if (!touchedbounds) return;

    rec_i32 aabb = obj_aabb(o);
    v2_i32  vdir = direction_v2(touchedbounds);
    aabb.x += vdir.x + g->map_worldroom->x;
    aabb.y += vdir.y + g->map_worldroom->y;

    map_worldroom_s *nextroom = map_world_overlapped_room(&g->map_world, g->map_worldroom, aabb);

    if (!nextroom) {
        sys_printf("no room\n");
        return;
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
        break;
    }

    v2_i32 feet = {trgaabb.x + trgaabb.w / 2, trgaabb.y + trgaabb.h};

    transition_start(t, g, nextroom->filename, 0, feet, hvel, o->facing);
    t->dir = touchedbounds;
}

void transition_start_respawn(game_s *g, transition_s *t)
{
    g->transition = *t;
    hero_s h      = *((hero_s *)obj_get_tagged(g, OBJ_TAG_HERO)->mem);

    game_load_map(g, t->to_load);
    obj_s *hero      = hero_create(g);
    hero->pos.x      = t->hero_feet.x - hero->w / 2;
    hero->pos.y      = t->hero_feet.y - hero->h;
    hero->facing     = t->hero_face;
    hero->vel_q8     = t->hero_v;
    hero_s *hh       = (hero_s *)hero->mem;
    hh->sprinting    = h.sprinting;
    hh->sprint_ticks = h.sprint_ticks;

    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    for (obj_each(g, o)) {
        obj_game_animate(g, o); // just setting initial sprites for obj
    }
    aud_allow_playing_new_snd(1);

    cam_s *cam  = &g->cam;
    v2_i32 hpos = obj_pos_center(hero);
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

    switch (t->fade_phase) {
    case NUM_TRANSITION_PHASES:
        t->fade_phase = 0;
        break;
    case TRANSITION_BLACK: {
        g->transition_last = *t;
        transition_start_respawn(g, t);
        break;
    }
    }
}

bool32 transition_blocks_gameplay(transition_s *t)
{
    return (t->fade_phase == TRANSITION_FADE_OUT ||
            t->fade_phase == TRANSITION_BLACK);
}

bool32 transition_finished(transition_s *t)
{
    return (t->fade_phase == TRANSITION_FADE_NONE);
}

void transition_draw(game_s *g, transition_s *t, v2_i32 camoffset)
{
    if (transition_finished(t)) return;

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
    if (ohero && t->fade_phase != TRANSITION_FADE_OUT) {
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
