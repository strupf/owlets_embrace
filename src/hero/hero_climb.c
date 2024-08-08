// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_climb(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    if (inp_action_jp(INP_A)) {
        h->climbing  = 0;
        h->climbingp = 0;
        o->facing    = -o->facing;
        o->v_q8.x    = o->facing * 500;
        i32 dpad_x   = inp_x();
        if (dpad_x == o->facing) {
            o->v_q8.x += dpad_x * 400;
        }
        hero_start_jump(g, o, HERO_JUMP_GROUND);
        return;
    }

    i32     y_offs = inp_y() * 3;
    rec_i32 r      = {o->pos.x, o->pos.y + y_offs, o->w, o->h};

    if (map_traversable(g, r) &&
        !hero_is_climbing_y_offs(g, o, o->facing, y_offs)) {
        h->climbing  = 0;
        h->climbingp = 0;
        o->v_q8.y    = -1100;
    }
    o->tomove.y = y_offs;
}

bool32 hero_is_climbing(game_s *g, obj_s *o, i32 facing)
{
    return hero_is_climbing_y_offs(g, o, facing, 0);
}

bool32 hero_is_climbing_y_offs(game_s *g, obj_s *o, i32 facing, i32 dy)
{
    if (!facing) return 0;
    if (obj_grounded(g, o)) return 0;

    i32 x  = (0 < facing ? o->pos.x + o->w : o->pos.x - 1);
    i32 y1 = o->pos.y + dy;
    i32 y2 = o->pos.y + o->h - 1 + dy;

    for (i32 y = y1; y <= y2; y++) {
        if (map_traversable_pt(g, x, y)) {
            return 0;
        }
    }

    return 1;
}