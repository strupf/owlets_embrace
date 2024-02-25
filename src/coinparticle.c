// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "coinparticle.h"
#include "game.h"

coinparticle_s *coinparticle_create(game_s *g)
{
    if (NUM_COINPARTICLE <= g->n_coinparticles) return NULL;
    coinparticle_s *c = &g->coinparticles[g->n_coinparticles++];
    *c                = (coinparticle_s){0};
    return c;
}

void coinparticle_update(game_s *g)
{
    obj_s *ohero   = obj_get_tagged(g, OBJ_TAG_HERO);
    v2_i32 heropos = {0};
    if (ohero) {
        heropos = obj_pos_center(ohero);
    }
    for (int n = g->n_coinparticles - 1; 0 <= n; n--) {
        coinparticle_s *c = &g->coinparticles[n];
        if (ohero && v2_distancesq(heropos, c->pos) < COINPARTICLE_COLLECT_DISTSQ) {
            inventory_add(&g->inventory, INVENTORY_ID_GOLD, 1);
            snd_play_ext(SNDID_COIN, 1.f, 1.f);
            *c = g->coinparticles[--g->n_coinparticles];
            continue;
        }
        c->tick--;
        if (c->tick <= 0 || !game_traversable_pt(g, c->pos.x, c->pos.y)) {
            *c = g->coinparticles[--g->n_coinparticles];
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
                c->vel_q8.x = -((c->vel_q8.x * 230) >> 8);
                break;
            }
            c->pos.x = x;
        }
        for (int m = abs_i(dy), s = sgn_i(dy); 0 < m; m--) {
            i32 y = c->pos.y + s;
            if (!game_traversable_pt(g, c->pos.x, y)) {
                c->vel_q8.y = -((c->vel_q8.y * 220) >> 8);
                break;
            }
            c->pos.y = y;
        }
    }
}

void coinparticle_draw(game_s *g, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = asset_texrec(TEXID_MISCOBJ, 0, 160, 16, 16);
    for (int n = 0; n < g->n_coinparticles; n++) {
        coinparticle_s *c = &g->coinparticles[n];
        v2_i32          p = v2_add(c->pos, cam);
        tr.r.x            = (((sys_tick() + n) >> 2) % 6) * 16;
        p.y -= 16;
        p.x -= 8;
        int mode = rngr_i32(0, 10) == 1 ? SPR_MODE_WHITE : 0;
        gfx_spr(ctx, tr, p, 0, mode);
    }
}