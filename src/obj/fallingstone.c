// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define FALLINGSTONE_VY_MAX (256 * 3)

obj_s *fallingstone_spawn(g_s *g)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_FALLINGSTONE;
    o->w          = 8;
    o->h          = 8;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->subID      = rngr_i32(0, 2);
    o->flags =
        OBJ_FLAG_HURT_ON_TOUCH |
        OBJ_FLAG_KILL_OFFSCREEN;
    return o;
}

void fallingstone_burst(g_s *g, obj_s *o)
{
    o->flags      = 0;
    o->state      = 1;
    o->moverflags = 0;
    obj_delete(g, o);
}

void fallingstone_on_update(g_s *g, obj_s *o)
{
    if (o->state == 1) return;
    if (o->bumpflags & OBJ_BUMP_XY) {
        fallingstone_burst(g, o);
        return;
    }

    o->v_q8.y += 50;
    o->v_q8.y = min_i32(o->v_q8.y, FALLINGSTONE_VY_MAX);
    obj_vx_q8_mul(o, 246);
    if (abs_i32(o->v_q8.x) < 32) {
        o->v_q8.x = 0;
    }
    obj_move_by_v_q8(g, o);
}

void fallingstone_on_animate(g_s *g, obj_s *o)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_MISCOBJ,
                                     736 + o->subID * 16,
                                     192,
                                     16, 16);
    spr->offs.x       = -4;
    spr->offs.y       = -4;
}