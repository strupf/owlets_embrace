// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 p0;
    v2_i32 p1;
} flyer_s;

obj_s *flyer_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_FLYER;
    o->flags = OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
#if GAME_JUMP_ATTACK
               OBJ_FLAG_PLATFORM | OBJ_FLAG_PLATFORM_HERO_ONLY |
#endif
               OBJ_FLAG_KILL_OFFSCREEN;

    o->w          = 16;
    o->h          = 16;
    o->health_max = 1;
    o->health     = o->health_max;
    o->enemy      = enemy_default();
    o->facing     = 1;
    return o;
}

void flyer_load(game_s *g, map_obj_s *mo)
{
    obj_s *o   = flyer_create(g);
    o->pos.x   = mo->x;
    o->pos.y   = mo->y;
    flyer_s *f = (flyer_s *)o->mem;
    f->p0.x    = mo->x;
    f->p0.y    = mo->y;
    v2_i16 p1  = map_obj_pt(mo, "P");
    f->p1.x    = p1.x << 4;
    f->p1.y    = p1.y << 4;
}

void flyer_on_update(game_s *g, obj_s *o)
{
    o->timer++;
    flyer_s *f = (flyer_s *)o->mem;
    i32      i = sin_q16(o->timer << 10) + 0x10000;
    v2_i32   p = v2_lerp(f->p0, f->p1, i, 0x20000);
#if GAME_JUMP_ATTACK
    o->tomove = v2_sub(p, o->pos);
#else
    o->pos = p;
#endif
}
