// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"
#include "obj.h"

void crumbleblock_think_idle(game_s *g, obj_s *o);
void crumbleblock_think_crumbling(game_s *g, obj_s *o);
void crumbleblock_think_broken(game_s *g, obj_s *o);

obj_s *crumbleblock_create(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_set_flags(g, o,
                      OBJ_FLAG_SOLID,
                      OBJ_FLAG_THINK_1);
        o->think_1 = crumbleblock_think_idle;
        o->state   = 0;
        o->ID      = 11;
        o->type    = 0;
        o->w       = 16;
        o->h       = 16;
        return o;
}

void crumbleblock_think_idle(game_s *g, obj_s *o)
{
        obj_s *hero;
        if (!try_obj_from_handle(g->hero.obj, &hero)) return;

        rec_i32 herofeet = obj_rec_bottom(hero);
        rec_i32 aabb     = obj_aabb(o);

        if (overlap_rec_excl(herofeet, aabb)) {
                o->think_1 = crumbleblock_think_crumbling;
                o->timer   = 100; // ticks to break
                o->state   = 1;
        }
}

void crumbleblock_think_crumbling(game_s *g, obj_s *o)
{
        if (--o->timer == 0) {
                o->think_1 = crumbleblock_think_broken;
                o->timer   = 100; // ticks to restore
                o->state   = 2;
                obj_unset_flags(g, o, OBJ_FLAG_SOLID);
        }
}

void crumbleblock_think_broken(game_s *g, obj_s *o)
{
        if (--o->timer == 0) {
                o->think_1 = crumbleblock_think_idle;
                o->state   = 0;
                obj_set_flags(g, o, OBJ_FLAG_SOLID);
        }
}
