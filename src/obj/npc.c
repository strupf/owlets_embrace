// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    NPC_MOVEMENT_STANDING,
    NPC_MOVEMENT_WANDER,
};

typedef struct {
    int movement;
    int movedir;
} npc_s;

obj_s *npc_create(game_s *g)
{
    obj_s *o = obj_create(g);

    npc_s *npc    = (npc_s *)o->mem;
    npc->movement = NPC_MOVEMENT_WANDER;

    o->ID    = OBJ_ID_NPC;
    o->flags = OBJ_FLAG_SPRITE |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_INTERACTABLE |
               OBJ_FLAG_MOVER;

    o->moverflags = OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLOPES;
    o->w            = 16;
    o->h            = 20;
    o->gravity_q8.y = 50;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 255;
    o->vel_cap_q8.x = 96; // don't move faster than 1 pixel per frame -> falls down

    sprite_simple_s *spr = &o->sprites[0];
    o->n_sprites         = 1;
    spr->trec            = asset_texrec(TEXID_NPC, 0, 4 * 64, 64, 64);
    spr->offs.x          = (o->w - spr->trec.r.h) / 2;
    spr->offs.y          = o->h - spr->trec.r.h;

    return o;
}

void npc_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = npc_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;

    map_obj_strs(mo, "Dialogfile", o->filename);
    o->sprites[0].trec.r.y = map_obj_i32(mo, "Model") * 64;
}

void npc_on_update(game_s *g, obj_s *o)
{
    bool32 bumpedx = o->bumpflags & OBJ_BUMPED_X;
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (bumpedx) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;
    if (!obj_grounded(g, o)) return;

    npc_s *npc = (npc_s *)o->mem;
    switch (npc->movement) {
    case NPC_MOVEMENT_WANDER: {
        // move left, right or stand still based on a random timer
        // ramp up velocity

        if (--o->timer <= 0) {
            o->drag_q8.x = 180; // slowly stop movement
            if (o->timer < -100 && abs_i(o->vel_q8.x) < 10) {
                o->timer     = rngr_i32(80, 150);
                o->vel_q8.x  = rngr_i32(-1, +1);
                o->drag_q8.x = 256;
            }
        } else {
            o->vel_q8.x <<= 1;
        }

        if (bumpedx || obj_would_fall_down_next(g, o, sgn_i(o->vel_q8.x))) {
            o->vel_q8.x = 0;
        }
    } break;
    }
}

void npc_on_interact(game_s *g, obj_s *o)
{
    npc_s *npc  = (npc_s *)o->mem;
    o->vel_q8.x = 0;
    textbox_load_dialog(g, &g->textbox, o->filename);
}

void npc_on_animate(game_s *g, obj_s *o)
{
    // o->animation++;
    o->sprites[0].trec.r.x = ((o->animation >> 4) & 1) * 64;
    if (o->vel_q8.x < 0) {
        o->sprites[0].flip = SPR_FLIP_X;
    } else if (o->vel_q8.x > 0) {
        o->sprites[0].flip = 0;
    }
}