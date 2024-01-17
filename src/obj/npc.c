// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    int x;
} npc_s;

obj_s *npc_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_NPC;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_INTERACTABLE;
    o->w = 16;
    o->h = 16;

    o->gravity_q8.y = 30;
    o->drag_q8.x    = 250;
    o->drag_q8.y    = 255;

    sprite_simple_s *spr = &o->sprites[0];
    o->n_sprites         = 1;
    spr->trec            = asset_texrec(TEXID_HERO, 0, 0, 64, 64);
    spr->offs.x          = (o->w - spr->trec.r.h) / 2;
    spr->offs.y          = o->h - spr->trec.r.h;

    return o;
}

obj_s *npc_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = npc_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;

    map_obj_strs(mo, "Dialogfile", o->filename);

    return o;
}

void npc_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;
}

void npc_on_interact(game_s *g, obj_s *o)
{
    textbox_load_dialog(g, &g->textbox, o->filename);
}

void npc_on_animate(game_s *g, obj_s *o)
{
}