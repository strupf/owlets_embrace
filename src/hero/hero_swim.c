// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

bool32 hero_is_submerged(game_s *g, obj_s *o, i32 *water_depth)
{
    i32 wd = water_depth_rec(g, obj_aabb(o));
    if (water_depth) {
        *water_depth = wd;
    }
    return (o->h <= wd);
}

i32 hero_breath_tick(game_s *g)
{
    hero_s *h = &g->hero_mem;
    return h->breath_ticks;
}

i32 hero_breath_tick_max(game_s *g)
{
    return (hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 2500 : 100);
}

void hero_update_swimming(game_s *g, obj_s *o)
{
    hero_s *h           = &g->hero_mem;
    o->drag_q8.x        = 240;
    h->sprint_dtap      = 0;
    h->sprint_ticks     = 0;
    h->flytime          = 0;
    h->flytime_added    = 0;
    h->jump_ui_may_hide = 1;

    i32    dpad_x      = inp_x();
    i32    dpad_y      = inp_y();
    i32    water_depth = 0;
    bool32 submerged   = hero_is_submerged(g, o, &water_depth);

    if (!submerged) {
        h->diving = 0;
    }

    o->animation++;

    if (h->diving && hero_has_upgrade(g, HERO_UPGRADE_DIVE)) {
        o->drag_q8.y    = 230;
        o->gravity_q8.y = 0;
        // o->vel_q8.y -= HERO_GRAVITY / 2;

        if (dpad_y) {
            i32 i0 = (dpad_y == sgn_i32(o->vel_q8.y) ? abs_i32(o->vel_q8.y) : 0);
            i32 ay = (max_i32(512 - i0, 0) * 128) >> 8;
            o->vel_q8.y += ay * dpad_y;
        }
    } else {
        o->drag_q8.y = 220;
        if (!hero_has_upgrade(g, HERO_UPGRADE_SWIM) && 0 < h->swimticks) {
            h->swimticks--; // swim ticks are reset when grounded later on
        }
        bool32 can_swim = hero_has_upgrade(g, HERO_UPGRADE_SWIM) ||
                          0 < h->swimticks;
        if (can_swim) {
            i32 i0 = pow_i32(min_i32(water_depth, 70), 2);
            i32 i1 = pow_i32(70, 2);
            i32 k0 = min_i32(water_depth, 30);
            i32 k1 = 30;

            i32 ch = lerp_i32(25, 70, k0, k1) +
                     lerp_i32(0, 110, i0, i1);
            o->vel_q8.y -= ch;

        } else {
            o->vel_q8.y -= min_i32(5 + water_depth, 40);
            h->diving = 1;
        }

        if (hero_has_upgrade(g, HERO_UPGRADE_DIVE) && 0 < inp_y()) {
            o->tomove.y += 10;
            o->vel_q8.y = +1000;
            h->diving   = 1;
        } else if (inp_action_jp(INP_A)) {
            // see if we can jump
            i32 tx1 = max_i32((o->pos.x + 0) >> 4, 0);
            i32 ty1 = max_i32((o->pos.y - 16) >> 4, 0);
            i32 ty2 = min_i32((o->pos.y + 12) >> 4, g->tiles_y - 1);
            i32 tx2 = min_i32((o->pos.x + o->w - 1) >> 4, g->tiles_x - 1);

            for (i32 yy = ty1; yy <= ty2; yy++) {
                for (i32 xx = tx1; xx <= tx2; xx++) {
                    i32 i = xx + yy * g->tiles_x;
                    if (g->tiles[i].collision ||
                        (g->tiles[i].type & TILE_WATER_MASK))
                        continue;
                    o->tomove.y  = -12;
                    o->drag_q8.y = 256;
                    hero_start_jump(g, o, HERO_JUMP_WATER);
                    goto BREAK_JUMP;
                }
            }
        BREAK_JUMP:;
        }
    }

    if (dpad_x != sgn_i32(o->vel_q8.x)) {
        o->vel_q8.x /= 2;
    }
    if (dpad_x) {
        i32 i0 = (dpad_x == sgn_i32(o->vel_q8.x) ? abs_i32(o->vel_q8.x) : 0);
        i32 ax = (max_i32(512 - i0, 0) * 32) >> 8;
        o->vel_q8.x += ax * dpad_x;
    }

    if (submerged && h->diving) {
        i32 breath_tm   = hero_breath_tick_max(g);
        h->breath_ticks = min_i32(h->breath_ticks + 1, breath_tm);
        if (breath_tm <= h->breath_ticks) {
            hero_kill(g, o);
        }
    }
}
