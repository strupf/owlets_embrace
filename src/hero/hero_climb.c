// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_walljump(g_s *g, obj_s *o, i32 dir)
{
    hero_s *h        = (hero_s *)o->heap;
    h->climbing      = 0;
    h->impact_ticks  = 0;
    h->walljump_tick = dir * WALLJUMP_MOM_TICKS;
    o->animation     = 0;
    o->facing        = dir;
    o->v_q8.x        = dir * 600;
    hero_start_jump(g, o, HERO_JUMP_WALL);
}

void hero_update_climb(g_s *g, obj_s *o)
{
    hero_s *h       = (hero_s *)o->heap;
    i32     dpad_y  = inp_y();
    o->linked_solid = obj_handle_from_obj(0);
    i32 sta         = hero_stamina_modify(g, o, -4);

    if (inp_action_jp(INP_A)) {
        hero_walljump(g, o, -o->facing);
    } else if (dpad_y && sta) {
        i32 N = 3;
        if (dpad_y < 0) {
            hero_stamina_modify(g, o, -32);
            N = 2;
            o->animation += 2;
        } else {
            o->animation = 0;
        }

        i32 n_moved = 0;
        for (i32 n = 0; n < N; n++) {
            rec_i32 r = {o->pos.x, o->pos.y + dpad_y, o->w, o->h};
            if (!map_traversable(g, r) ||
                !hero_is_climbing_offs(g, o, o->facing, 0, dpad_y))
                break;
            obj_move(g, o, 0, dpad_y);
            n_moved++;
        }

        if (!n_moved) {
            h->climbing     = 0;
            h->impact_ticks = 0;
            if (dpad_y < 0) {
                o->v_q8.y = -1100;
            } else {
                o->v_q8.y = +256;
            }
        }
    } else if (!sta) {
        o->v_q8.y += 20;
        o->v_q8.y = min_i32(o->v_q8.y, 256);
        obj_move_by_q8(g, o, 0, o->v_q8.y);
    }

    if (h->climbing) {
        for (obj_each(g, it)) {
            if (it->mass == 0 || it == o) continue;
            rec_i32 r = o->facing == 1 ? obj_rec_right(o) : obj_rec_left(o);
            if (overlap_rec(r, obj_aabb(it))) {
                o->linked_solid = obj_handle_from_obj(it);
            }
        }
    }
}

bool32 hero_is_climbing(g_s *g, obj_s *o, i32 facing)
{
    return hero_is_climbing_offs(g, o, facing, 0, 0);
}

bool32 hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy)
{
    if (!facing) return 0;
    rec_i32 r = {o->pos.x + dx, o->pos.y + dy, o->w, o->h};
    if (map_blocked(g, o, r, 0)) return 0;
    if (obj_grounded(g, o)) return 0;

    i32 x  = dx + (0 < facing ? o->pos.x + o->w : o->pos.x - 1);
    i32 y1 = dy + o->pos.y + 2;
    i32 y2 = dy + o->pos.y + o->h - 1 - 4;

    for (i32 y = y1; y <= y2; y++) {
        if (map_traversable_pt(g, x, y)) {
            return 0;
        }
    }

    return 1;
}