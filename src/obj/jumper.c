// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    JUMPER_ST_IDLE,
    JUMPER_ST_ANTICIPATE,
    JUMPER_ST_JUMPING,
    JUMPER_ST_LANDED,
};

void jumper_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->w     = 16;
    o->h     = 16;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->ID    = OBJID_JUMPER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->moverflags = OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_TERRAIN_COLLISIONS;
}

void jumper_on_update(g_s *g, obj_s *o)
{
    o->timer++;
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q8.x = -o->v_q8.x / 2;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;
    o->v_q8.y += 70;
    bool32 grounded = obj_grounded(g, o);
    if (!grounded && o->state != JUMPER_ST_JUMPING) {
        o->timer = 0;
        o->state = JUMPER_ST_JUMPING;
    }

    switch (o->state) {
    case JUMPER_ST_IDLE: {
        if (50 <= o->timer) {
            o->timer = 0;
            o->state = JUMPER_ST_ANTICIPATE;
        }
        break;
    }
    case JUMPER_ST_ANTICIPATE: {
        if (20 <= o->timer) {
            o->timer  = 0;
            o->state  = JUMPER_ST_JUMPING;
            o->v_q8.y = -2000;
        }
        break;
    }
    case JUMPER_ST_JUMPING: {
        if (grounded) {
            o->timer = 0;
            o->state = JUMPER_ST_LANDED;
        }
        break;
    }
    case JUMPER_ST_LANDED: {
        if (10 <= o->timer) {
            o->timer = 0;
            o->state = JUMPER_ST_IDLE;
        }
        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void jumper_on_animate(g_s *g, obj_s *o)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];

    i32 fr_x    = 0;
    i32 fr_y    = 0;
    spr->offs.x = (o->w - 64) / 2;
    spr->offs.y = (o->h - 64);

    switch (o->state) {
    case JUMPER_ST_IDLE: {

        break;
    }
    case JUMPER_ST_ANTICIPATE: {

        break;
    }
    case JUMPER_ST_JUMPING: {

        break;
    }
    case JUMPER_ST_LANDED: {

        break;
    }
    }

    spr->trec = asset_texrec(TEXID_JUMPER, fr_x * 64, fr_y * 64, 64, 64);
}
