// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    WATERLEAF_IDLE,
    WATERLEAF_ACTIVE,
    WATERLEAF_SHAKE,
    WATERLEAF_SINK,
    WATERLEAF_WAIT_FOR_RESPAWN,
    WATERLEAF_RESPAWN
};

#define WATERLEAF_TICKS_ACTIVE           170
#define WATERLEAF_TICKS_SHAKE            30
#define WATERLEAF_TICKS_SINK             30
#define WATERLEAF_TICKS_WAIT_FOR_RESPAWN 120
#define WATERLEAF_TICKS_RESPAWN          26

typedef struct {
    v2_i32 p_og;
} waterleaf_s;

void waterleaf_on_update(g_s *g, obj_s *o);
void waterleaf_on_animate(g_s *g, obj_s *o);
void waterleaf_on_impulse(g_s *g, obj_s *o, i32 x_q8, i32 y_q8);

void waterleaf_load(g_s *g, map_obj_s *mo)
{
    obj_s       *o = obj_create(g);
    waterleaf_s *w = (waterleaf_s *)o->mem;
    o->editorUID   = mo->UID;
    o->ID          = OBJID_WATERLEAF;
    o->flags       = OBJ_FLAG_PLATFORM;
    o->moverflags  = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = 16;
    w->p_og        = o->pos;
    o->on_animate  = waterleaf_on_animate;
    o->on_update   = waterleaf_on_update;
    o->on_impulse  = waterleaf_on_impulse;
    o->animation   = rngr_i32(0, 100);
}

void waterleaf_on_update(g_s *g, obj_s *o)
{
    waterleaf_s *w = (waterleaf_s *)o->mem;
    if (o->state == WATERLEAF_IDLE) {
        obj_s *ohero = obj_get_owl(g);
        if (obj_standing_on(ohero, o, 0, 0)) {
            o->state++;
        }
    } else {
        o->timer++;
    }
    if (o->state != WATERLEAF_IDLE) {
        o->timer++;
    }

    switch (o->state) {
    default: break;
    case WATERLEAF_ACTIVE:
    case WATERLEAF_SHAKE:
        obj_vx_q12_mul(o, Q_12(0.992));
        break;
    case WATERLEAF_SINK: {
        obj_vx_q12_mul(o, Q_12(0.97));
        o->v_q12.y += Q_VOBJ(0.0625);
        break;
    }
    }

    switch (o->state) {
    default: break;
    case WATERLEAF_ACTIVE:
    case WATERLEAF_SHAKE:
    case WATERLEAF_SINK: {
        if (o->bumpflags) {
            obj_vx_q8_mul(o, -Q_8(0.62));
        }
        o->bumpflags = 0;
        break;
    }
    }

    switch (o->state) {
    default: break;
    case WATERLEAF_ACTIVE: {
        if (WATERLEAF_TICKS_ACTIVE <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case WATERLEAF_SHAKE: {
        if (WATERLEAF_TICKS_SHAKE <= o->timer) {
            o->state++;
            o->timer    = 0;
            o->blinking = 1;
        }
        break;
    }
    case WATERLEAF_SINK: {
        if (WATERLEAF_TICKS_SINK <= o->timer) {
            o->state++;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
            o->flags &= ~OBJ_FLAG_PLATFORM;
            o->blinking = 0;
        }
        break;
    }
    case WATERLEAF_WAIT_FOR_RESPAWN: {
        if (WATERLEAF_TICKS_WAIT_FOR_RESPAWN <= o->timer) {
            o->state++;
            o->timer = 0;
            o->pos.x = w->p_og.x;
            o->pos.y = w->p_og.y;
            o->flags |= OBJ_FLAG_PLATFORM;
        }
        break;
    }
    case WATERLEAF_RESPAWN: {
        if (WATERLEAF_TICKS_RESPAWN <= o->timer) {
            o->state = 0;
            o->timer = 0;
        }
        break;
    }
    }

    o->subpos_q12 = v2_i32_add(o->subpos_q12, o->v_q12);
    i32 dx        = o->subpos_q12.x >> 12;
    i32 dy        = o->subpos_q12.y >> 12;
    o->subpos_q12.x &= 0xFFF;
    o->subpos_q12.y &= 0xFFF;

    for (i32 k = abs_i32(dx), s = sgn_i32(dx); k; k--) {
        bool32  bumped_other = 0;
        rec_i32 rr           = {o->pos.x + s, o->pos.y, o->w, o->h};

        for (obj_each(g, i)) {
            if (!(i != o && i->ID == OBJID_WATERLEAF)) continue;

            rec_i32 ri = obj_aabb(i);
            if (overlap_rec(rr, ri)) {
                waterleaf_on_impulse(g, i, o->v_q12.x >> 1, 0);
                obj_vx_q8_mul(o, -Q_8(0.5));
                bumped_other = 1;
            }
        }

        if (bumped_other) break;

        obj_move(g, o, s, 0);
    }
    obj_move(g, o, 0, dy);
}

void waterleaf_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    i32           w   = 96;
    i32           h   = 32;
    i32           fx  = 3;
    i32           fy  = 9;
    o->n_sprites      = 1;
    spr->offs.x       = (o->w - w) >> 1;
    spr->offs.y       = -10;

    switch (o->state) {
    default: o->n_sprites = 0; break;
    case WATERLEAF_IDLE:
    case WATERLEAF_ACTIVE:
    case WATERLEAF_SHAKE:
    case WATERLEAF_SINK: {
        o->animation += rngr_i32(0, 2);
        spr->offs.y += (3 * sin_q15(o->animation << 11)) / 32789;
        if (o->state == WATERLEAF_ACTIVE && o->timer < 20) {
            if (0) {
            } else if (o->timer < 2) {
                spr->offs.y += 2;
            } else if (o->timer < 6) {
                spr->offs.y += 4;
            } else if (o->timer < 10) {
                spr->offs.y += 2;
            } else if (o->timer < 12) {
                spr->offs.y += 0;
            } else if (o->timer < 16) {
                spr->offs.y -= 2;
            }
        }
        break;
    }
    case WATERLEAF_RESPAWN: {
        i32 t2 = WATERLEAF_TICKS_RESPAWN >> 1;
        if (o->timer < t2) {
            spr->offs.y += lerp_i32(8, -4, o->timer, t2);
        } else {
            spr->offs.y += lerp_i32(-4, 0, o->timer - t2, t2);
        }
        break;
    }
    }

    switch (o->w) {
    default:
    case 48: fy = 8; break;
    case 64: fy = 9; break;
    case 80: fy = 10; break;
    }

    spr->trec = asset_texrec(TEXID_MISCOBJ, fx * w, fy * h, w, h);
}

void waterleaf_on_impulse(g_s *g, obj_s *o, i32 x_q8, i32 y_q8)
{
    if (o->state == WATERLEAF_IDLE) {
        o->state = WATERLEAF_ACTIVE;
    }
    o->v_q12.x += x_q8;
}