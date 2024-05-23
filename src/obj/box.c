// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void box_on_update(game_s *g, obj_s *o);
void box_on_animate(game_s *g, obj_s *o);

void box_load(game_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->on_update  = box_on_update;
    o->on_animate = box_on_animate;
    o->ID         = 1000;
    o->flags      = OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_CARRYABLE |
               OBJ_FLAG_SOLID;
    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
    spr->trec         = asset_texrec(TEXID_MISCOBJ, 512, 128, 32, 32);

    o->moverflags = OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_MAP |
                    OBJ_MOVER_SLOPES;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->w            = mo->w;
    o->h            = mo->h;
    o->mass         = 1;
    o->gravity_q8.y = 70;
    o->drag_q8.x    = 128;
    o->drag_q8.y    = 256;
}

void box_on_update(game_s *g, obj_s *o)
{
    if (o->state == 1) {

        o->bumpflags = 0;
        return;
    }

    o->drag_q8.x = obj_grounded(g, o) ? 240 : 253;

    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = (-o->vel_q8.y * 50) >> 8;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = (-o->vel_q8.x * 50) >> 8;
    }
    o->bumpflags = 0;
}

void box_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    spr->offs         = carryable_animate_spr_offset(o);
}

void box_on_lift(game_s *g, obj_s *o)
{
    o->state = 1;
}

void box_on_drop(game_s *g, obj_s *o)
{
    o->state = 0;
}
