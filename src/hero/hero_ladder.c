// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_ladder(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    o->flags &= ~OBJ_FLAG_MOVER;
    hero_restore_grounded_stuff(g, o);

    if (o->pos.x != h->ladderx) {
        h->onladder = 0;
        return;
    }

    if (inp_action_jp(INP_A)) {
        h->onladder = 0;
        hero_start_jump(g, o, 0);
        i32 dpad_x = inp_x();
        o->v_q8.x  = dpad_x * 200;
        return;
    }

    if (inp_action_jp(INP_B)) {
        h->onladder = 0;
        return;
    }

    i32 dpad_y = inp_y();
    i32 dir_y  = dpad_y << 1;

    for (i32 m = abs_i32(dir_y); m; m--) {
        rec_i32 aabb = obj_aabb(o);
        aabb.y += dpad_y;
        if (!map_traversable(g, aabb)) break;
        if (obj_grounded_at_offs(g, o, (v2_i32){0, dpad_y})) {
            o->pos.y += dpad_y;
            h->onladder = 0;
            break;
        }
        if (!tile_map_ladder_overlaps_rec(g, aabb, NULL)) break;
        o->pos.y += dpad_y;
    }

    if (dir_y) {
        o->animation++;
    }
}

bool32 hero_try_snap_to_ladder(game_s *g, obj_s *o, i32 diry)
{
    rec_i32 aabb    = obj_aabb(o);
    rec_i32 rladder = aabb;
    if (obj_grounded(g, o) && 0 < diry) {
        rladder = obj_rec_bottom(o);
    }

    v2_i32 lpos;
    if (!tile_map_ladder_overlaps_rec(g, rladder, &lpos)) return 0;

    hero_s *hero = &g->hero_mem;

    i32 posx = (lpos.x << 4) + 8 - (aabb.w / 2);
    aabb.x   = posx;
    aabb.y += diry;
    if (!map_traversable(g, aabb)) return 0;

    hero_action_ungrapple(g, o);
    o->flags &= ~OBJ_FLAG_MOVER;
    hero_restore_grounded_stuff(g, o);
    o->pos.x = posx;
    o->pos.y += diry;
    o->v_q8.x         = 0;
    o->v_q8.y         = 0;
    o->animation      = 0;
    hero->onladder    = 1;
    hero->ladderx     = posx;
    hero->attack_tick = 0;
    return 1;
}

// tries to calculate a snapped ladder position
bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout)
{
    return hero_rec_on_ladder(g, obj_aabb(o), rout);
}

// tries to calculate a snapped ladder position
bool32 hero_rec_on_ladder(game_s *g, rec_i32 aabb, rec_i32 *rout)
{
    i32 tx1 = (aabb.x >> 4);
    i32 tx2 = ((aabb.x + aabb.w - 1) >> 4);
    i32 ty  = ((aabb.y + (aabb.h >> 1)) >> 4);

    for (i32 tx = tx1; tx <= tx2; tx++) {
        if (g->tiles[tx + ty * g->tiles_x].collision != TILE_LADDER) continue;

        i32     lc_x = (tx << 4) + 8;
        rec_i32 r    = {lc_x - (aabb.w >> 1), aabb.y, aabb.w, aabb.h}; // aabb if on ladder
        if (!map_traversable(g, r)) continue;                          // if ladder position is valid
        if (rout) {
            *rout = r;
        }
        return 1;
    }
    return 0;
}