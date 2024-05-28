// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    MAPTRANSITION_TYPE_TELEPORT,
    MAPTRANSITION_TYPE_SLIDE,
};

enum {
    MAPTRANSITION_FADE_NONE,
    MAPTRANSITION_FADE_OUT,
    MAPTRANSITION_FADE_IN,
    //
    NUM_MAPTRANSITION_PHASES
};

const i32 maptransition_phase[NUM_MAPTRANSITION_PHASES] = {
    0,
    15,
    30};

void maptransition_init(game_s *g, const char *file,
                        i32 type, v2_i32 hero_feet, v2_i32 hero_v, i32 facing)
{
    maptransition_s *mt = &g->maptransition;
    str_cpy(mt->to_load, file);
    mt->dir        = 0;
    mt->type       = type;
    mt->hero_feet  = hero_feet;
    mt->hero_v     = hero_v;
    mt->hero_face  = facing;
    mt->fade_tick  = 0;
    mt->fade_phase = 1;
    g->substate    = SUBSTATE_MAPTRANSITION;
}

void maptransition_teleport(game_s *g, const char *map, v2_i32 hero_feet)
{
    v2_i32 hero_v = {0};
    maptransition_init(g, map, MAPTRANSITION_TYPE_TELEPORT,
                       hero_feet, hero_v, +1);
}

bool32 maptransition_try_hero_slide(game_s *g)
{
    maptransition_s *mt = &g->maptransition;

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

    if (!touchedbounds) return 0;

    rec_i32 aabb = obj_aabb(o);
    v2_i32  vdir = direction_v2(touchedbounds);
    aabb.x += vdir.x + g->map_worldroom->x;
    aabb.y += vdir.y + g->map_worldroom->y;

    map_worldroom_s *nextroom = map_world_overlapped_room(&g->map_world, g->map_worldroom, aabb);

    if (!nextroom) {
        pltf_log("no room\n");
        nextroom = map_world_overlapped_room(&g->map_world, g->map_worldroom, aabb);
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

    maptransition_init(g, nextroom->filename,
                       MAPTRANSITION_TYPE_SLIDE, feet, hvel, o->facing);
    mt->dir           = touchedbounds;
    mt->airjumps_left = g->hero_mem.airjumps_left;
    return 1;
}

void maptransition_update(game_s *g)
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

    game_load_map(g, mt->to_load);
    obj_s  *hero      = hero_create(g);
    hero_s *hh        = (hero_s *)&g->hero_mem;
    hero->pos.x       = mt->hero_feet.x - hero->w / 2;
    hero->pos.y       = mt->hero_feet.y - hero->h;
    hero->facing      = mt->hero_face;
    hero->vel_q8      = mt->hero_v;
    hh->airjumps_left = mt->airjumps_left;
    v2_i32 hpos       = obj_pos_center(hero);

    u32     respawn_d    = U32_MAX;
    v2_i32 *resp_closest = NULL;
    for (int n = 0; n < g->n_respawns; n++) {
        v2_i32 *rp = &g->respawns[n];
        u32     d  = v2_distancesq(hpos, *rp);
        if (d < respawn_d) {
            respawn_d    = d;
            resp_closest = rp;
        }
    }

    if (resp_closest) {
        str_cpy(g->save.hero_mapfile, g->areaname.filename);
        g->save.hero_pos = *resp_closest;
        game_save_savefile(g);
    }
    game_prepare_new_map(g);
}

void maptransition_draw(game_s *g, v2_i32 cam)
{
    maptransition_s *mt = &g->maptransition;

    const i32 ticks  = maptransition_phase[mt->fade_phase];
    const i32 ticksh = ticks >> 1;
    const i32 ft     = mt->fade_tick - ticksh;

    gfx_pattern_s pat = {0};

    switch (mt->fade_phase) {
    case MAPTRANSITION_FADE_OUT:
        pat = gfx_pattern_interpolate(mt->fade_tick, ticks);
        break;
    case MAPTRANSITION_FADE_IN:
        if (ft < 0) {
            pat = gfx_pattern_100();
        } else {
            pat = gfx_pattern_interpolate(ticksh - ft, ticksh);
        }

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
    if (mt->fade_phase == MAPTRANSITION_FADE_IN && ohero && 0 <= ft) {
        gfx_ctx_s ctxc = gfx_ctx_default(tmp);
        v2_i32    cpos = v2_add(obj_pos_center(ohero), cam);
        ctxc.pat       = gfx_pattern_interpolate(ft, ticksh);
        i32 cird       = ease_out_quad(0, 200, ft, ticksh);
        gfx_cir_fill(ctxc, cpos, cird, PRIM_MODE_WHITE);
    }

    for (i32 y = 0; y < tmp.h; y++) {
        for (i32 x = 0; x < tmp.wword; x++) {
            i32 i = x + y * tmp.wword;
            display.px[i] &= tmp.px[i];
        }
    }

    spm_pop();
}
