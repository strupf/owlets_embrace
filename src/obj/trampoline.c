// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define TRAMPOLINE_EXTEND_TICKS 8
#define TRAMPOLINE_WOBBLE_TICKS 80
#define TRAMPOLINE_THICKNESS    4

enum {
    TRAMPOLINE_HOR,
    TRAMPOLINE_VER,
};

void trampoline_on_update(g_s *g, obj_s *o);
void trampoline_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void trampoline_load(g_s *g, map_obj_s *mo)
{
    obj_s *o     = obj_create(g);
    o->ID        = OBJID_TRAMPOLINE;
    o->on_update = trampoline_on_update;
    o->on_draw   = trampoline_on_draw;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;

    if (mo->w == 16) {
        o->state = TRAMPOLINE_VER;
        o->w     = TRAMPOLINE_THICKNESS;
        o->h     = mo->h;
        o->pos.x += (16 - TRAMPOLINE_THICKNESS) / 2;
    } else if (mo->h == 16) {
        o->state = TRAMPOLINE_HOR;
        o->w     = mo->w;
        o->h     = TRAMPOLINE_THICKNESS;
        o->pos.y += (16 - TRAMPOLINE_THICKNESS) / 2;
    } else {
        BAD_PATH
    }
    o->flags = OBJ_FLAG_SOLID;
}

void trampoline_on_update(g_s *g, obj_s *o)
{
    if (o->timer) {
        o->timer++;
        if (TRAMPOLINE_WOBBLE_TICKS <= o->timer) {
            o->timer    = 0;
            o->substate = DIR_NONE; // bump direction
        }
    } else {
        o->substate = DIR_NONE; // bump direction
    }
}

static i32 trampoline_spr_flip(obj_s *o)
{
    switch (o->state) {
    case TRAMPOLINE_HOR: return (o->substate == DIR_Y_POS ? 0 : SPR_FLIP_Y);
    case TRAMPOLINE_VER: return (o->substate == DIR_X_POS ? 0 : SPR_FLIP_X);
    }
    return 0;
}

void trampoline_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx     = gfx_ctx_display();
    i32       frameID = 0;
    i32       fl      = 0;

    if (o->timer) {
        if (o->timer <= TRAMPOLINE_EXTEND_TICKS) { // impact
            fl = trampoline_spr_flip(o);

            bool32 fully_extended =
                (TRAMPOLINE_EXTEND_TICKS * 1) / 4 <= o->timer &&
                o->timer <= (TRAMPOLINE_EXTEND_TICKS * 3) / 4;
            frameID = 2 + fully_extended;
        } else { // wobble
            // 4 sprite cycle with 2 sprites, flip the extended one
            i32 fr = (o->timer >> 1) & 3;
            if (fr == 3) {
                fl = (o->state == TRAMPOLINE_HOR ? SPR_FLIP_Y : SPR_FLIP_X);
            }
            frameID = (fr & 1);
        }
    } else if (o->substate) { // static extension
        fl = trampoline_spr_flip(o);
    }

    i32      n_tiles = max_i32(o->w >> 4, o->h >> 4);
    v2_i32   p       = v2_i32_add(o->pos, cam);
    texrec_s tr      = asset_texrec(TEXID_TRAMPOLINE, 0, 0, 0, 0);

    switch (o->state) {
    case TRAMPOLINE_HOR: {
        tr.w = 16;
        tr.h = 32;
        tr.y = frameID * 32;
        p.y -= (32 - TRAMPOLINE_THICKNESS) / 2;
        for (i32 n = 0; n < n_tiles; n++) {
            tr.x = ((0 < n) + (n == n_tiles - 1)) * 16;
            gfx_spr(ctx, tr, p, fl, 0);
            p.x += 16;
        }
        break;
    }
    case TRAMPOLINE_VER: {
        tr.w = 32;
        tr.h = 16;
        tr.x = frameID * 32 + 64;
        p.x -= (32 - TRAMPOLINE_THICKNESS) / 2;
        for (i32 n = 0; n < n_tiles; n++) {
            tr.y = ((0 < n) + (n == n_tiles - 1)) * 16;
            gfx_spr(ctx, tr, p, fl, 0);
            p.y += 16;
        }
        break;
    }
    }
}

void trampoline_do_bounce(g_s *g, obj_s *o)
{
    rec_i32 r = obj_aabb(o);

    for (obj_each(g, i)) {
        if (i->ID != OBJID_HERO) continue;
        hero_s *h = (hero_s *)i->heap;

        switch (o->state) {
        case TRAMPOLINE_HOR: {
            if (overlap_rec(r, obj_rec_top(i))) {
                o->substate = DIR_Y_NEG;
                if (i->v_q8.y < -100) {
                    i->v_q8.y = +1000;
                    i->bumpflags &= ~OBJ_BUMP_Y_NEG;
                    o->timer = 1;
                }
            }
            if (overlap_rec(r, obj_rec_bottom(i))) {
                o->substate = DIR_Y_POS;
                if (i->v_q8.y > +1000) {
                    i->v_q8.y = -2000;
                    i->bumpflags &= ~OBJ_BUMP_Y_POS;
                    o->timer = 1;
                }
            }
            break;
        }
        case TRAMPOLINE_VER: {
#define TRAMPOLINE_VX_TRESHOLD 500
            i32 bounced = 0;
            if (overlap_rec(r, obj_rec_left(i))) {
                o->substate = DIR_X_NEG;
                if (i->v_q8.x < -TRAMPOLINE_VX_TRESHOLD) {
                    bounced = +1;
                    i->bumpflags &= ~OBJ_BUMP_X_NEG;
                }
            }
            if (overlap_rec(r, obj_rec_right(i))) {
                o->substate = DIR_X_POS;
                if (i->v_q8.x > +TRAMPOLINE_VX_TRESHOLD) {
                    bounced = -1;
                    i->bumpflags &= ~OBJ_BUMP_X_POS;
                }
            }

            if (bounced) {
                g->freeze_tick        = 2;
                i->v_q8.x             = +3000 * bounced;
                i->v_q8.y             = -1300;
                h->air_block_ticks_og = 80;
                h->air_block_ticks    = bounced * h->air_block_ticks_og;
                h->low_grav_ticks_0   = 50;
                h->low_grav_ticks     = 50;
                o->timer              = 1;
            }
            break;
        }
        }
    }
}

void trampolines_do_bounce(g_s *g)
{
    for (obj_each(g, o)) {
        if (o->ID != OBJID_TRAMPOLINE) continue;
        trampoline_do_bounce(g, o);
    }
}