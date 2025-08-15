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
    v2_i32 owl_feet;
    v2_i32 owl_v_q12;
    u8     map_name[32];
    u8     fade_phase;
    u8     type;
    u8     dir;
    u8     fade_tick;
    i32    teleport_ID;
} maptransition_s;

static_assert(sizeof(maptransition_s) <= CS_MEM_BYTES, "Size");

#define MAPTRANSITION_TICKS_F_OUT             15
#define MAPTRANSITION_TICKS_F_BLACK           15
#define MAPTRANSITION_TICKS_F_IN              25
#define MAPTRANSITION_UPWARDS_PIXEL_THRESHOLD 8 // make it easier to move up

void cs_maptransition_update(g_s *g, cs_s *cs);
void cs_maptransition_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_maptransition_load_map(g_s *g, cs_s *cs);

void cs_maptransition_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_maptransition_update;
    cs->on_draw   = cs_maptransition_draw;
}

void cs_maptransition_init(g_s *g, cs_s *cs, u8 *map_name, i32 type, v2_i32 hero_feet)
{
    maptransition_s *mt  = (maptransition_s *)cs->mem;
    obj_s           *owl = obj_get_tagged(g, OBJ_TAG_OWL);
    owl_s           *h   = (owl_s *)owl->heap;
    mcpy(mt->map_name, map_name, sizeof(mt->map_name));
    mt->dir        = 0;
    mt->type       = type;
    mt->owl_feet   = hero_feet;
    mt->fade_tick  = 0;
    mt->fade_phase = 1;
    h->safe_pos.x  = hero_feet.x;
    h->safe_pos.y  = hero_feet.y;
    h->safe_v.x    = sgn_i32(owl->v_q12.x) * Q_VOBJ(2.0);
    h->safe_v.y    = 0;
    h->safe_facing = owl->facing;
    if (owl->v_q12.y < 0) {
        h->safe_v.y = -Q_VOBJ(4.0);
    }
    grapplinghook_destroy(g, &g->ghook);
}

bool32 cs_maptransition_try_slide_enter(g_s *g)
{
    obj_s *o = 0;
    if (!(o = owl_if_present_and_alive(g))) return 0;

    owl_s  *h                 = (owl_s *)o->heap;
    rec_i32 aabb              = obj_aabb(o);
    rec_i32 r_check_neighbour = aabb;

    i32 touchedbounds = 0;
    if (o->pos.y < MAPTRANSITION_UPWARDS_PIXEL_THRESHOLD) {
        touchedbounds       = DIRECTION_N;
        r_check_neighbour.y = 0;
    }
    if (o->pos.x <= 0) {
        touchedbounds = DIRECTION_W;
    }
    if (g->pixel_x <= o->pos.x + o->w) {
        touchedbounds = DIRECTION_E;
    }
    if (g->pixel_y <= o->pos.y + o->h) {
        touchedbounds = DIRECTION_S;
    }

    if (!touchedbounds) return 0;

    map_room_s *mneighbor = 0;
    map_room_s *mcur      = g->map_room_cur;

    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mn = &g->map_rooms[n];
        if (mn == mcur) continue;

        rec_i32 rn = {(mn->x - mcur->x) << 4, (mn->y - mcur->y) << 4, mn->w << 4, mn->h << 4};
        if (overlap_rec_touch(r_check_neighbour, rn)) {
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
    v2_i32 hvel = o->v_q12;

    switch (touchedbounds) {
    case DIRECTION_E:
        aabb.x = 8;
        break;
    case DIRECTION_W:
        aabb.x = (mneighbor->w << 4) - aabb.w - 8;
        break;
    case DIRECTION_N:
        aabb.y        = (mneighbor->h << 4) - aabb.h - 8;
        hvel.y        = min_i32(hvel.y, -Q_VOBJ(5.5));
        h->jump_ticks = 0;
        break;
    case DIRECTION_S:
        aabb.y = MAPTRANSITION_UPWARDS_PIXEL_THRESHOLD;
        hvel.y = Q_VOBJ(1.0);
        break;
    }

    cs_s            *cs = &g->cs;
    maptransition_s *mt = (maptransition_s *)cs->mem;
    cs_maptransition_enter(g);

    v2_i32 feet = {aabb.x + aabb.w / 2, aabb.y + aabb.h};
    cs_maptransition_init(g, cs, mneighbor->map_name, MAPTRANSITION_TYPE_SLIDE, feet);
    g->block_update = 1;
    mt->dir         = touchedbounds;
    mt->owl_v_q12   = hvel;
    o->bumpflags    = 0;
    return 1;
}

void cs_maptransition_teleport(g_s *g, u8 *map_name, v2_i32 pos)
{
    obj_s *o = obj_get_owl(g);
    assert(o);

    cs_s            *cs = &g->cs;
    maptransition_s *mt = (maptransition_s *)cs->mem;
    cs_maptransition_enter(g);
    cs_maptransition_init(g, cs, map_name, MAPTRANSITION_TYPE_TELEPORT, pos);
    g->block_update = 1;
    mt->owl_v_q12.x = 0;
    mt->owl_v_q12.y = 0;
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
        if (cs->tick == 2) {
            cs_maptransition_load_map(g, cs); // sets tick variable
        }
        if (MAPTRANSITION_TICKS_F_BLACK <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
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
    f32              t1  = pltf_seconds();
    maptransition_s *mt  = (maptransition_s *)cs->mem;
    obj_s           *owl = obj_get_owl(g);

    if (owl) {
        owl->pos.x = mt->owl_feet.x - owl->w / 2;
        owl->pos.y = mt->owl_feet.y - owl->h;
        if (mt->dir) {
            owl->v_q12 = mt->owl_v_q12;
        }
    }
    owl_s *h = (owl_s *)owl->heap;
    game_load_map(g, mt->map_name);

    if (owl) {
        v2_i32  hpos = obj_pos_center(owl);
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
        companion_spawn(g, owl);
    }

    cam_init_level(g, &g->cam);
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    objs_animate(g);
    aud_allow_playing_new_snd(1);

    f32 seconds_loaded = pltf_seconds() - t1;
    i32 ticks_loaded   = (i32)(seconds_loaded * 50.f + 0.5f); // 50 = 1/0.020
    if (g->speedrun) {
        // don't adjust black phase time to not discriminate different
        // playdate CPU revisions. Also keep it short
        cs->tick = MAPTRANSITION_TICKS_F_BLACK - 1;
    } else {
        // adjust black phase time for time needed to load so it's
        // more constant across all transitions, whether the map to load
        // is big (slower loading) or small (faster loading)
        cs->tick = min_i32(MAPTRANSITION_TICKS_F_BLACK, 2 + ticks_loaded);
    }
    pltf_sync_timestep();
}