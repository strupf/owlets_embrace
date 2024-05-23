// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void juggernaut_on_update(game_s *g, obj_s *o)
{
}

void juggernaut_on_animate(game_s *g, obj_s *o)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_JUGGERNAUT, 0, 0, 96, 96);
    spr->offs.x       = -32;
    spr->offs.y       = -48;
}

void juggernaut_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_JUGGERNAUT;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_ENEMY;
    o->on_update        = juggernaut_on_update;
    o->on_animate       = juggernaut_on_animate;
    o->health           = 3;
    o->moverflags       = OBJ_MOVER_SLOPES | OBJ_MOVER_ONE_WAY_PLAT;
    o->w                = 32;
    o->h                = 32;
    o->gravity_q8.y     = 60;
    o->drag_q8.x        = 255;
    o->drag_q8.y        = 255;
    o->enemy.sndID_hurt = SNDID_ENEMY_HURT;
    o->enemy.sndID_die  = SNDID_ENEMY_DIE;
}
