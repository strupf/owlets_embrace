// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    MAPTRANSITION_TYPE_TELEPORT,
    MAPTRANSITION_TYPE_SLIDE,
};

enum {
    MAPTRANSITION_FADE_NONE,
    MAPTRANSITION_FADE_OUT,
    MAPTRANSITION_FADE_BLACK,
    MAPTRANSITION_FADE_IN,
    //
    NUM_MAPTRANSITION_PHASES
};

const u8 maptransition_phase[NUM_MAPTRANSITION_PHASES] = {
    0,
    15,
    10,
    25};

void maptransition_init(g_s *g, u32 map_hash, i32 type, v2_i32 hero_feet)
{
    maptransition_s *mt = &g->maptransition;
    mt->map_hash        = map_hash;
    mt->dir             = 0;
    mt->type            = type;
    mt->hero_feet       = hero_feet;
    mt->fade_tick       = 0;
    mt->fade_phase      = 1;
    g->substate         = SUBSTATE_MAPTRANSITION;
    grapplinghook_destroy(g, &g->ghook);
}

void maptransition_teleport(g_s *g, u32 map_hash, v2_i32 hero_feet)
{
    maptransition_init(g, map_hash, MAPTRANSITION_TYPE_TELEPORT, hero_feet);
}

bool32 maptransition_try_hero_slide(g_s *g)
{
    maptransition_s *mt = &g->maptransition;

    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o || o->health == 0) return 0;

    i32 touchedbounds = 0;
    if (o->pos.x <= 0)
        touchedbounds = DIRECTION_W;
    if (g->pixel_x <= o->pos.x + o->w)
        touchedbounds = DIRECTION_E;
    if (o->pos.y <= 0)
        touchedbounds = DIRECTION_N;
    if (g->pixel_y <= o->pos.y + o->h)
        touchedbounds = DIRECTION_S;

    if (!touchedbounds) return 0;

    rec_i32 aabb = obj_aabb(o);

    map_neighbor_s *neighbor = 0;
    for (i32 n = 0; n < NUM_MAP_NEIGHBORS; n++) {
        map_neighbor_s *mn = &g->map_neighbors[n];
        if (mn->hash == 0) break;

        rec_i32 rn = {mn->x, mn->y, mn->w, mn->h};
        if (overlap_rec_touch(aabb, rn)) {
            neighbor = mn;
            break;
        }
    }

    if (!neighbor) {
        pltf_log("no room\n");
        return 0;
    }

    aabb.x -= neighbor->x;
    aabb.y -= neighbor->y;

    v2_i16 hvel = o->v_q8;

    switch (touchedbounds) {
    case DIRECTION_E:
        aabb.x = 8;
        break;
    case DIRECTION_W:
        aabb.x = neighbor->w - aabb.w - 8;
        break;
    case DIRECTION_N:
        aabb.y = neighbor->h - aabb.h - 8;
        hvel.y = min_i32(hvel.y, -1200);
        break;
    case DIRECTION_S:
        aabb.y = 2;
        hvel.y = 512;
        break;
    }

    v2_i32 feet = {aabb.x + aabb.w / 2, aabb.y + aabb.h};
    maptransition_init(g, neighbor->hash, MAPTRANSITION_TYPE_SLIDE, feet);
    mt->dir       = touchedbounds;
    mt->hero_v_q8 = hvel;
    o->bumpflags  = 0;
    return 1;
}

void maptransition_update(g_s *g)
{
    maptransition_s *mt = &g->maptransition;
    if (!mt->fade_phase) return;

    const i32 ticks = maptransition_phase[mt->fade_phase];
    mt->fade_tick++;
    if (mt->fade_tick < ticks) return;

    mt->fade_tick = 0;
    mt->fade_phase++;
    mt->fade_phase %= NUM_MAPTRANSITION_PHASES;

    if (mt->fade_phase == 0) {
        g->substate = 0;
        return;
    }
    if (mt->fade_phase != MAPTRANSITION_FADE_IN) return;

    game_load_map(g, mt->map_hash);
    obj_s *ohero = obj_get_hero(g);
    if (ohero) {
        ohero->pos.x = mt->hero_feet.x - ohero->w / 2;
        ohero->pos.y = mt->hero_feet.y - ohero->h;
        if (mt->dir) {
            ohero->v_q8 = mt->hero_v_q8;
        }
    }
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    objs_animate(g);
    aud_allow_playing_new_snd(1);
    cam_init_level(g, &g->cam);
}

void maptransition_draw(g_s *g, v2_i32 cam)
{
    tex_s            display = asset_tex(0);
    maptransition_s *mt      = &g->maptransition;
    if (mt->fade_phase == MAPTRANSITION_FADE_BLACK) {
        tex_clr(display, GFX_COL_BLACK);
        return;
    }

    const i32 ticks = maptransition_phase[mt->fade_phase];

    gfx_pattern_s pat = {0};

    switch (mt->fade_phase) {
    case MAPTRANSITION_FADE_OUT:
        pat = gfx_pattern_interpolate(mt->fade_tick, ticks);
        break;
    case MAPTRANSITION_FADE_IN:
        pat = gfx_pattern_interpolate(ticks - mt->fade_tick, ticks);
        break;
    }

    spm_push();

    tex_s tmp = tex_create_opaque(display.w, display.h, spm_allocator);
    for (i32 y = 0; y < tmp.h; y++) {
        u32 p = ~pat.p[y & 7];
        for (i32 x = 0; x < tmp.wword; x++) {
            tmp.px[x + y * tmp.wword] = p;
        }
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    if (mt->fade_phase == MAPTRANSITION_FADE_IN && ohero) {
        gfx_ctx_s ctxc = gfx_ctx_default(tmp);
        v2_i32    cpos = v2_i32_add(obj_pos_center(ohero), cam);
        i32       cird = ease_out_quad(0, 200, min_i32(mt->fade_tick, ticks / 2), ticks / 2);
        gfx_cir_fill(ctxc, cpos, cird, GFX_COL_WHITE);
    }

    u32 *p1 = display.px;
    u32 *p2 = tmp.px;
    for (i32 n = 0; n < PLTF_DISPLAY_NUM_WORDS; n++) {
        *p1++ &= *p2++;
    }
    spm_pop();
}
