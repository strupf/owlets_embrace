// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void walker_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }

    if ((o->bumpflags & OBJ_BUMPED_X) || obj_would_fall_down_next(g, o, o->facing)) {
        o->facing = -o->facing;
    }

    o->bumpflags = 0;
    o->tomove.x  = time_now() & 1 ? o->facing : 0;
}

void walker_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *spr     = &o->sprites[0];
    i32           frameID = (time_now() >> 3) & 3;
    spr->flip             = 0 < o->facing ? 0 : SPR_FLIP_X;
    spr->trec             = asset_texrec(TEXID_SKELETON, frameID * 64, 64, 64, 64);
    spr->offs.y           = -(spr->trec.r.h - o->h);
    spr->offs.x           = -(spr->trec.r.w - o->w) / 2;
}

void walker_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_WALKER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->on_update  = walker_on_update;
    o->on_animate = walker_on_animate;

    o->w     = 20;
    o->h     = 30;
    o->pos.x = mo->x;
    o->pos.y = mo->y + mo->h - o->h;

    o->drag_q8.y    = 255;
    o->health_max   = 5;
    o->health       = o->health_max;
    o->enemy        = enemy_default();
    o->gravity_q8.y = 70;
    o->facing       = 1;
    o->n_sprites    = 1;
}
