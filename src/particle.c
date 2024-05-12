// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle.h"
#include "game.h"

void particles_spawn(game_s *g, particles_s *pr, particle_desc_s desc, int n)
{
    if (pr->n == PARTICLE_NUM) return;
    for (int i = 0; i < n; i++) {
        v2_i32 pos = desc.p.p_q8;
        pos.x += rngr_sym_i32(desc.pr_q8.x);
        pos.y += rngr_sym_i32(desc.pr_q8.y);
        if (game_traversable_pt(g, pos.x >> 8, pos.y >> 8)) {
            particle_s *p = &pr->particles[pr->n++];
            *p            = desc.p;
            p->p_q8       = pos;
            p->v_q8.x += rngr_sym_i32(desc.vr_q8.x);
            p->v_q8.y += rngr_sym_i32(desc.vr_q8.y);
            p->a_q8.x += rngr_sym_i32(desc.ar_q8.x);
            p->a_q8.y += rngr_sym_i32(desc.ar_q8.y);
            p->size += rngr_i32(0, desc.sizer);
            p->ticks_max += rngr_i32(0, desc.ticksr);
            p->ticks = p->ticks_max;
            if (pr->n == PARTICLE_NUM) return;
        }
    }
}

void particles_update(game_s *g, particles_s *pr)
{
    for (int i = pr->n - 1; 0 <= i; i--) {
        particle_s *p  = &pr->particles[i];
        v2_i32      p0 = v2_shr(p->p_q8, 8);
        if (--p->ticks <= 0 || !game_traversable_pt(g, p0.x, p0.y)) {
            *p = pr->particles[--pr->n];
            continue;
        }

        p->v_q8   = v2_add(p->v_q8, p->a_q8);
        v2_i32 pp = v2_add(p->p_q8, p->v_q8);  // proposed new position
        v2_i32 pd = v2_sub(v2_shr(pp, 8), p0); // delta in pixels

        for (int m = abs_i(pd.x), s = sgn_i(pd.x); m; m--) {
            if (game_traversable_pt(g, p0.x + s, p0.y)) {
                p0.x += s;
            } else {
                p->v_q8.x = -(p->v_q8.x >> 1); // bounce off
                pp.x      = p0.x << 8;
                break;
            }
        }

        for (int m = abs_i(pd.y), s = sgn_i(pd.y); m; m--) {
            if (game_traversable_pt(g, p0.x, p0.y + s)) {
                p0.y += s;
            } else {
                p->v_q8.y = -(p->v_q8.y >> 1); // bounce off
                p->p_q8.y = p0.y << 8;
                pp.y      = p0.y << 8;
                break;
            }
        }

        p->p_q8 = pp;
    }
}

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
            hero_coins_change(g, 1);
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

        bool32 collided_bot = 0;
        for (int m = abs_i(dy), s = sgn_i(dy); 0 < m; m--) {
            i32 y = c->pos.y + s;
            if (!game_traversable_pt(g, c->pos.x, y)) {
                c->vel_q8.y  = -((c->vel_q8.y * 220) >> 8);
                collided_bot = (0 < dy);
                break;
            }
            c->pos.y = y;
        }

        if (collided_bot && -100 <= c->vel_q8.y) {
            c->vel_q8.x = 0;
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