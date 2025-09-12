// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    MUSHROOM_IDLE,
    MUSHROOM_JUMPED_ON,
    MUSHROOM_HOOKED,
};

typedef struct {
    i32 h_additional;
} mushroom_s;

#define MUSHROOM_BOUNCE_TICKS 32

void mushroom_on_update(g_s *g, obj_s *o);
void mushroom_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void mushroom_on_hook(g_s *g, obj_s *o, i32 hooked);

void mushroom_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o   = obj_create(g);
    mushroom_s *m   = (mushroom_s *)o->mem;
    o->UUID         = mo->UUID;
    o->ID           = OBJID_MUSHROOM;
    o->on_update    = mushroom_on_update;
    o->on_draw      = mushroom_on_draw;
    o->on_hook      = mushroom_on_hook;
    o->w            = mo->w;
    o->h            = mo->h - 8;
    m->h_additional = (mo->h - 32) >> 4;

    obj_place_to_map_obj(o, mo, 0, -1);
    o->pos.y = mo->y + 8;
    o->flags = OBJ_FLAG_OWL_JUMPSTOMPABLE;
    o->timer = rngr_i32(0, 100);
}

void mushroom_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case MUSHROOM_IDLE: {
        o->timer++;
        break;
    }
    case MUSHROOM_JUMPED_ON: {
        if (o->timer < 30) {
            // g->cam.cowl.force_lower_ceiling = min_i32(o->timer << 2, 30);
        }
        o->timer++;
        if (MUSHROOM_BOUNCE_TICKS <= o->timer) {
            o->state = MUSHROOM_IDLE;
            o->timer = 0;
        }
        break;
    }
    case MUSHROOM_HOOKED: {

        break;
    }
    }
}

void mushroom_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    mushroom_s *m    = (mushroom_s *)o->mem;
    i32         fr_x = o->w == 32 ? 0 : 1;
    i32         fr_y = 0;

    switch (o->state) {
    case MUSHROOM_IDLE: {
        fr_y = ((o->timer >> 4) & 7) == 0; // subtle idle bop
        break;
    }
    case MUSHROOM_JUMPED_ON: {
        if (o->timer < 1) {
            fr_y = 1;
            break;
        }
        if (o->timer < 2) {
            fr_y = 2;
            break;
        }
        if (o->timer < 6) {
            fr_y = 3;
            break;
        }

        i32 t = ((o->timer >> 1) & 3);

        if (o->timer < 4 + MUSHROOM_BOUNCE_TICKS / 2) {
            switch (t) {
            case 0: fr_y = 0; break;
            case 1: fr_y = 1; break;
            case 2: fr_y = 2; break;
            case 3: fr_y = 1; break;
            }
        } else {
            fr_y = t >> 1;
        }

        break;
    }
    case MUSHROOM_HOOKED: {

        break;
    }
    }

    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = asset_texrec(TEXID_MUSHROOM, fr_x * 64, fr_y * 32, 64, 32);
    v2_i32    p   = {cam.x + o->pos.x - ((64 - o->w) >> 1),
                     cam.y + o->pos.y - 8};
    gfx_spr(ctx, tr, p, 0, 0);

    texrec_s tr_add = asset_texrec(TEXID_MUSHROOM, 128, 32, 64, 16);
    p.y += 16;
    for (i32 n = 0; n < m->h_additional; n++) {
        p.y += 16;
        gfx_spr(ctx, tr_add, p, 0, 0);
    }
    p.y += 16;
    texrec_s tr_bot = asset_texrec(TEXID_MUSHROOM, 128, 0, 64, 16);
    gfx_spr(ctx, tr_bot, p, 0, 0);
}

void mushroom_on_jump_on(g_s *g, obj_s *o)
{
    o->state = MUSHROOM_JUMPED_ON;
    o->timer = 0;
}

void mushroom_on_hook(g_s *g, obj_s *o, i32 hooked)
{
    if (hooked) {
        o->state = MUSHROOM_HOOKED;
        o->timer = 0;
        o->flags &= ~OBJ_FLAG_OWL_JUMPSTOMPABLE;
    } else {
        o->state = MUSHROOM_JUMPED_ON;
        o->timer = 0;
        o->flags |= OBJ_FLAG_OWL_JUMPSTOMPABLE;
    }
}