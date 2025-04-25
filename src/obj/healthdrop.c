// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HEALTHDROP_ST_IDLE,
    HEALTHDROP_ST_HOMING,
};

void healthdrop_on_update(g_s *g, obj_s *o);
void healthdrop_on_animate(g_s *g, obj_s *o);

obj_s *healthdrop_spawn(g_s *g, v2_i32 p)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_HEALTHDROP;
    o->w          = 22;
    o->h          = 22;
    o->pos.x      = p.x - o->w / 2;
    o->pos.y      = p.y - o->h / 2;
    o->on_animate = healthdrop_on_animate;
    o->on_update  = healthdrop_on_update;
    o->flags      = OBJ_FLAG_ACTOR;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->timer      = 15 * 50;
    return o;
}

void healthdrop_on_update(g_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_hero(g);
    v2_i32 p     = obj_pos_center(o);
    v2_i32 phero = {0};
    if (ohero) {
        phero = obj_pos_center(ohero);
    }

    switch (o->state) {
    case HEALTHDROP_ST_IDLE: {
        o->timer--;
        if (ohero && v2_i32_distancesq(phero, p) <= POW2(50)) {
            o->state      = HEALTHDROP_ST_HOMING;
            o->moverflags = 0;
        } else if (o->timer <= 0) {
            obj_delete(g, o);
        } else {
            if (o->timer < 100) {
                o->blinking = 1;
            }
            o->v_q8.y += 10;
            o->v_q8.y = min_i32(o->v_q8.y, 128 * 1);

            if (o->bumpflags & OBJ_BUMP_X) {
                obj_vx_q8_mul(o, -192);
            }
            if (o->bumpflags & OBJ_BUMP_Y) {
                o->v_q8.y = 0;
            }
            o->bumpflags = 0;

            if (abs_i32(o->v_q8.x) < 16) {
                o->v_q8.x = 0;
            }
        }
        break;
    }
    case HEALTHDROP_ST_HOMING: {
        if (ohero) {
            v2_i32 v  = v2_i32_from_i16(o->v_q8);
            v2_i32 vs = steer_seek(p, v, phero, 600);
            o->v_q8.x += vs.x >> 2;
            o->v_q8.y += vs.y >> 2;
        } else {
            obj_delete(g, o);
        }
        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void healthdrop_on_animate(g_s *g, obj_s *o)
{
    o->n_sprites = 1;
    o->animation++;

    obj_sprite_s *spr = &o->sprites[0];
    i32           fr  = ani_frame(ANIID_HEALTHDROP, o->animation);
    spr->offs.x       = (o->w - 32) / 2;
    spr->offs.y       = (o->h - 32) / 2;

    if (obj_grounded(g, o)) {
        spr->flip = o->sprites[1].flip;
    } else {
        // float from left to right and back and flip sides
        i32 len2 = 2 * ani_len(ANIID_HEALTHDROP);
        i32 t    = o->animation % len2;
        i32 k1   = (t * 131072) / len2;
        i32 k2   = (t * 65536) / len2;

        i32 offsx = ((6 * sin_q15(k1 + 32768)) / 32769);
        spr->offs.x += offsx;
        spr->offs.y += ((4 * cos_q15(k2 + 32768)) / 32769);
        if (0) {
        } else if (offsx <= -1) {
            fr += 9;
            o->sprites[1].flip = SPR_FLIP_X;
        } else if (offsx >= +1) {
            o->sprites[1].flip = 0;
        } else {
            fr = 8;
        }
    }

    spr->trec = asset_texrec(TEXID_HEARTDROP, 0, fr * 32, 32, 32);
}