// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    u32 anim_frame;
    u32 anim_propeller;
} flyblob_s;

void flyblob_on_update(game_s *g, obj_s *o);
void flyblob_on_animate(game_s *g, obj_s *o);

void flyblob_load(game_s *g, map_obj_s *mo)
{
    obj_s     *o = obj_create(g);
    flyblob_s *f = (flyblob_s *)o->mem;

    o->on_animate = flyblob_on_animate;
    o->on_update  = flyblob_on_update;
    o->w          = 16;
    o->h          = 16;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->health_max = 3;
    o->health     = o->health_max;

    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->n_sprites = 2;
}

void flyblob_on_update(game_s *g, obj_s *o)
{
    flyblob_s *f = (flyblob_s *)o->mem;
}

void flyblob_on_animate(game_s *g, obj_s *o)
{
    flyblob_s    *f  = (flyblob_s *)o->mem;
    obj_sprite_s *s0 = &o->sprites[0];
    obj_sprite_s *s1 = &o->sprites[1];
    f->anim_propeller++;
    f->anim_frame++;

    // offset of propeller on each bob frame
    static const i8 propeller_offs[] = {-5, -6, -6, -5, -3,
                                        +0, +2, +3, +2, -2};

    u32 frameblob = 1 + frame_from_ticks_pingpong(f->anim_frame / 4, 4);
    u32 frameprop = (f->anim_propeller >> 1) & 3;

    s0->offs.x = -18;
    s0->offs.y = -18;
    s1->offs.x = s0->offs.x;
    s1->offs.y = s0->offs.y + propeller_offs[frameblob];

    o->n_sprites = 2;
    if (o->enemy.hurt_tick) { // display hurt frame
        o->n_sprites = 1;
        frameblob    = 10;
    }

    s0->trec = asset_texrec(TEXID_FLYBLOB, frameblob * 64, 64, 64, 64);
    s1->trec = asset_texrec(TEXID_FLYBLOB, frameprop * 64, 0, 64, 64);
}
