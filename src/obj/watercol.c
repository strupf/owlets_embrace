// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    WATERCOL_IDLE,
    WATERCOL_ANTICIPATE,
    WATERCOL_RISE,
    WATERCOL_HOLD,
    WATERCOL_FALL,
    //
    NUM_WATERCOL_PHASES
};

typedef struct {
    i32 h_max;
} watercol_s;

static const i16 g_watercol_ticks[NUM_WATERCOL_PHASES] =
    {
        50,  // idle
        20,  // anticipate
        20,  // rise
        200, // hold
        20   // fall
};

void watercol_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o = obj_create(g);
    watercol_s *w = (watercol_s *)o->mem;
    o->ID         = OBJID_WATERCOL;
    o->flags      = OBJ_FLAG_RENDER_AABB;
    o->w          = 48;
    w->h_max      = map_obj_i32(mo, "H_Max");
    o->pos.y      = mo->y;
    o->pos.x      = mo->x;
    o->w          = mo->w;
    o->h          = 0;
    o->flags |= OBJ_FLAG_PLATFORM;
}

void watercol_on_update(g_s *g, obj_s *o)
{
    watercol_s *w = (watercol_s *)o->mem;
    o->timer++;
    if (g_watercol_ticks[o->state] <= o->timer) {
        o->timer = 0;
        o->state = (o->state + 1) % NUM_WATERCOL_PHASES;
    }

    // if (o->state == WATERCOL_IDLE) {
    //    o->flags &= ~OBJ_FLAG_PLATFORM;
    // } else {
    if (!(o->flags & OBJ_FLAG_PLATFORM)) {
        i32 k = 0;
    }
    o->flags |= OBJ_FLAG_PLATFORM;
    //}

    i32 t = g_watercol_ticks[o->state];
    i32 h = 0;

    switch (o->state) {
    case WATERCOL_RISE: {
        h = ease_out_quad(0, w->h_max, o->timer, t);
        break;
    }
    case WATERCOL_HOLD: {
        h = w->h_max;
        break;
    }
    case WATERCOL_FALL: {
        h = ease_in_quad(w->h_max, 0, o->timer, t);
        break;
    }
    }

    i32 dh = h - o->h;
    o->h   = h;

    obj_move(g, o, 0, -dh);

    if (WATERCOL_RISE <= o->state) {
        obj_s *ohero = obj_get_hero(g);
        if (ohero && overlap_rec(obj_aabb(ohero), obj_aabb(o))) {
            // ohero->v_q8.y = max_i32(ohero->v_q8.y - 150, -256 * 6);
        }
    }
}

void watercol_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32 p = v2_i32_add(o->pos, cam);
    i32    t = pltf_cur_tick();
    p.y -= 32 + (2 * sin_q16(t << 14)) / 65536;
    p.x += (o->w - 64) / 2;
    p.x &= ~1;
    p.y &= ~1;
    texrec_s  tr  = asset_texrec(TEXID_WATERCOL,
                                 ((t >> 2) % 3) * 64, 0,
                                 64, 64);
    gfx_ctx_s ctx = gfx_ctx_display();
    gfx_spr(ctx, tr, p, 0, 0);
}