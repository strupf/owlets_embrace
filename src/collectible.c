// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

collectible_s *collectible_create(game_s *g)
{
    if (NUM_COLLECTIBLE <= g->n_collectibles) return NULL;
    collectible_s *c = &g->collectibles[g->n_collectibles++];
    *c               = (collectible_s){0};
    return c;
}

void collectibles_update(game_s *g)
{
    obj_s *ohero   = obj_get_tagged(g, OBJ_TAG_HERO);
    v2_i32 heropos = obj_pos_center(ohero);
    for (int n = g->n_collectibles - 1; 0 <= n; n--) {
        collectible_s *c = &g->collectibles[n];
        if (v2_distancesq(heropos, c->pos) < COLLECTIBLE_COLLECT_DISTSQ) {
            snd_play_ext(SNDID_COIN, 1.f, 1.f);
            *c = g->collectibles[--g->n_collectibles];
            continue;
        }
        c->tick--;
        if (c->tick <= 0 || !game_traversable_pt(g, c->pos.x, c->pos.y)) {
            *c = g->collectibles[--g->n_collectibles];
            continue;
        }

        c->vel_q8.x = ((c->vel_q8.x + c->acc_q8.x) * c->drag_q8.x) >> 8;
        c->vel_q8.y = ((c->vel_q8.y + c->acc_q8.y) * c->drag_q8.y) >> 8;
        c->pos_q8.x += c->vel_q8.x;
        c->pos_q8.y += c->vel_q8.y;
        i32 dx = c->pos_q8.x >> 8;
        i32 dy = c->pos_q8.y >> 8;
        c->pos_q8.x &= 0xFF;
        c->pos_q8.y &= 0xFF;

        for (int m = abs_i(dx), s = sgn_i(dx); 0 < m; m--) {
            i32 x = c->pos.x + s;
            if (!game_traversable_pt(g, x, c->pos.y)) {
                c->vel_q8.x = -((c->vel_q8.x * 240) >> 8);
                break;
            }
            c->pos.x = x;
        }
        for (int m = abs_i(dy), s = sgn_i(dy); 0 < m; m--) {
            i32 y = c->pos.y + s;
            if (!game_traversable_pt(g, c->pos.x, y)) {
                c->vel_q8.y = -((c->vel_q8.y * 240) >> 8);
                break;
            }
            c->pos.y = y;
        }
    }
}

void collectibles_draw(game_s *g, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    for (int n = 0; n < g->n_collectibles; n++) {
        collectible_s *c = &g->collectibles[n];
        v2_i32         p = v2_add(c->pos, cam);
        gfx_cir_fill(ctx, p, 10, 0);
    }
}