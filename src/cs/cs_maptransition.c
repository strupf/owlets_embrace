// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

enum {
    MAPTRANSITION_TYPE_TELEPORT,
    MAPTRANSITION_TYPE_SLIDE
};

enum {
    MAPTRANSITION_FADE_OUT,
    MAPTRANSITION_FADE_BLACK,
    MAPTRANSITION_FADE_IN
};

typedef struct maptransition_s {
    v2_i32 hero_feet;
    v2_i16 hero_v_q8;
    u32    map_hash;
    u8     fade_phase;
    u8     type;
    u8     dir;
    u8     fade_tick;
    i32    teleport_ID;
} maptransition_s;

static_assert(sizeof(maptransition_s) <= CS_MEM_BYTES, "Size");

#define MAPTRANSITION_TICKS_F_OUT   15
#define MAPTRANSITION_TICKS_F_BLACK 10
#define MAPTRANSITION_TICKS_F_IN    25

void cs_maptransition_update(g_s *g, cs_s *cs);
void cs_maptransition_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_maptransition_load_map(g_s *g, cs_s *cs);

void cs_maptransition_enter(g_s *g)
{
    cs_s *cs = &g->cuts;
    cs_reset(g);
    cs->on_update = cs_maptransition_update;
    cs->on_draw   = cs_maptransition_draw;
}

void cs_maptransition_init(g_s *g, cs_s *cs, u32 map_hash, i32 type, v2_i32 hero_feet)
{
    maptransition_s *mt    = (maptransition_s *)cs->mem;
    hero_s          *h     = (hero_s *)&g->hero;
    obj_s           *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    mt->map_hash           = map_hash;
    mt->dir                = 0;
    mt->type               = type;
    mt->hero_feet          = hero_feet;
    mt->fade_tick          = 0;
    mt->fade_phase         = 1;
    h->safe_pos.x          = hero_feet.x;
    h->safe_pos.y          = hero_feet.y;
    h->safe_v.x            = sgn_i32(ohero->v_q8.x) * 500;
    h->safe_v.y            = 0;
    h->safe_facing         = ohero->facing;
    if (ohero->v_q8.y < 0) {
        h->safe_v.y = -1000;
    }
    grapplinghook_destroy(g, &g->ghook);
}

bool32 cs_maptransition_try_slide_enter(g_s *g)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o || o->health == 0) return 0;

    hero_s *h = (hero_s *)o->heap;

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

    rec_i32     aabb      = obj_aabb(o);
    map_room_s *mneighbor = 0;
    map_room_s *mcur      = g->map_room_cur;

    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mn = &g->map_rooms[n];
        if (mn == mcur) continue;

        rec_i32 rn = {(mn->x - mcur->x) << 4, (mn->y - mcur->y) << 4,
                      mn->w << 4, mn->h << 4};
        if (overlap_rec_touch(aabb, rn)) {
            mneighbor = mn;
            break;
        }
    }
    if (!mneighbor) { // end of rooms
        pltf_log("no room\n");
        return 0;
    }

    aabb.x -= (mneighbor->x - mcur->x) << 4;
    aabb.y -= (mneighbor->y - mcur->y) << 4;
    v2_i16 hvel = o->v_q8;

    switch (touchedbounds) {
    case DIRECTION_E:
        aabb.x = 8;
        break;
    case DIRECTION_W:
        aabb.x = (mneighbor->w << 4) - aabb.w - 8;
        break;
    case DIRECTION_N:
        aabb.y       = (mneighbor->h << 4) - aabb.h - 8;
        hvel.y       = min_i32(hvel.y, -1200);
        // if A pressed before transition and released the vy will be reduced,
        // resulting in the player not making the screen transition
        // -> clear jumpticks
        h->jumpticks = 0;
        break;
    case DIRECTION_S:
        aabb.y = 2;
        hvel.y = 512;
        break;
    }

    cs_s            *cs = &g->cuts;
    maptransition_s *mt = (maptransition_s *)cs->mem;
    cs_maptransition_enter(g);

    v2_i32 feet = {aabb.x + aabb.w / 2, aabb.y + aabb.h};
    cs_maptransition_init(g, cs, mneighbor->hash,
                          MAPTRANSITION_TYPE_SLIDE, feet);
    g->block_update = 1;
    mt->dir         = touchedbounds;
    mt->hero_v_q8   = hvel;
    o->bumpflags    = 0;
    return 1;
}

void cs_maptransition_teleport(g_s *g, u32 map_hash, v2_i32 pos)
{
    obj_s *o = obj_get_hero(g);
    assert(o);

    cs_s            *cs = &g->cuts;
    maptransition_s *mt = (maptransition_s *)cs->mem;
    cs_maptransition_enter(g);
    cs_maptransition_init(g, cs, map_hash,
                          MAPTRANSITION_TYPE_TELEPORT, pos);
    g->block_update = 1;
    mt->hero_v_q8.x = 0;
    mt->hero_v_q8.y = 0;
    hero_s *h       = (hero_s *)o->heap;
}

void cs_maptransition_update(g_s *g, cs_s *cs)
{
    maptransition_s *mt = (maptransition_s *)cs->mem;

    switch (cs->phase) {
    case MAPTRANSITION_FADE_OUT: {
        if (MAPTRANSITION_TICKS_F_OUT <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case MAPTRANSITION_FADE_BLACK: {
        if (MAPTRANSITION_TICKS_F_BLACK <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
            cs_maptransition_load_map(g, cs);
        }
        break;
    }
    case MAPTRANSITION_FADE_IN: {
        if (MAPTRANSITION_TICKS_F_IN <= cs->tick) {
            g->block_update = 0;
            cs_reset(g);
        }
        break;
    }
    }
}

void cs_maptransition_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    tex_s display = asset_tex(0);

    switch (cs->phase) {
    case MAPTRANSITION_FADE_OUT: {
        render_map_transition_out(g, cam, cs->tick, MAPTRANSITION_TICKS_F_OUT);
        break;
    }
    case MAPTRANSITION_FADE_BLACK: {
        tex_clr(display, GFX_COL_BLACK);
        break;
    }
    case MAPTRANSITION_FADE_IN: {
        render_map_transition_in(g, cam, cs->tick, MAPTRANSITION_TICKS_F_IN);
        break;
    }
    }
}

void cs_maptransition_load_map(g_s *g, cs_s *cs)
{
    maptransition_s *mt    = (maptransition_s *)cs->mem;
    obj_s           *ohero = obj_get_hero(g);

    if (ohero) {
        ohero->pos.x = mt->hero_feet.x - ohero->w / 2;
        ohero->pos.y = mt->hero_feet.y - ohero->h;
        if (mt->dir) {
            ohero->v_q8 = mt->hero_v_q8;
        }
    }

    game_load_map(g, mt->map_hash);

    if (ohero) {
        v2_i32  hpos = obj_pos_center(ohero);
        v2_i32 *sp   = 0;
        u32     dc   = U32_MAX;
        for (i32 n = 0; n < g->n_save_points; n++) {
            u32 d = v2_i32_distancesq(g->save_points[n], hpos);
            if (d < dc) {
                sp = &g->save_points[n];
                dc = d;
            }
        }

        if (sp) {
            game_save_savefile(g, *sp);
        }
    }

    if (save_event_exists(g, SAVE_EV_COMPANION_FOUND)) {
        companion_spawn(g, ohero);
    }

    cam_init_level(g, &g->cam);
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    objs_animate(g);
    aud_allow_playing_new_snd(1);
}