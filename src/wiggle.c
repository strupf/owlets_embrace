// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void grass_put(game_s *g, i32 tx, i32 ty)
{
    if (g->n_grass >= NUM_GRASS) return;
    grass_s *gr = &g->grass[g->n_grass++];
    *gr         = (grass_s){0};
    gr->pos.x   = tx * 16;
    gr->pos.y   = ty * 16;
    gr->type    = rngr_i32(0, 2);
}

void grass_animate(game_s *g)
{
    for (u32 n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->v_q8.x >> 4;
            }
        }

        gr->v_q8 += rngr_sym_i32(6) - ((gr->x_q8 * 15) >> 8);
        gr->x_q8 += gr->v_q8;
        gr->x_q8 = clamp_i32(gr->x_q8, -256, +256);
        gr->v_q8 = (gr->v_q8 * 230) >> 8;
    }
}

void grass_draw(game_s *g, rec_i32 camrec, v2_i32 camoffset)
{
    gfx_ctx_s ctx     = gfx_ctx_display();
    texrec_s  trgrass = {0};
    trgrass.t         = asset_tex(TEXID_PLANTS);

    for (u32 n = 0; n < g->n_grass; n++) {
        grass_s *gr     = &g->grass[n];
        rec_i32  rgrass = {gr->pos.x - 8, gr->pos.y - 8, 32, 32};
        if (!overlap_rec(rgrass, camrec)) continue;

        v2_i32 pos = v2_add(gr->pos, camoffset);

        for (i32 i = 0; i < 16; i++) {
            v2_i32 p = pos;
            p.y += i;
            p.x += (gr->x_q8 * (15 - i)) >> 8; // shear
            rec_i32 rg = {224 + 8, i + gr->type * 16, 16, 1};
            trgrass.r  = rg;
            gfx_spr(ctx, trgrass, p, 0, 0);
        }
    }
}

void deco_verlet_animate_single(game_s *g, deco_verlet_s *d);

void deco_verlet_obj_collision(game_s *g, obj_s *o, i32 r)
{
    v2_i32 po = v2_shl(obj_pos_center(o), 6);
    i32    r2 = pow2_i32(r);

    for (u32 n = 0; n < g->n_deco_verlet; n++) {
        deco_verlet_s *d = &g->deco_verlet[n];

        v2_i32 p = v2_shl(d->pos, 6);
        for (u32 n = 1; n < d->n_pt; n++) {
            deco_verlet_pt_s *pt = &d->pt[n];
            v2_i32            pp = v2_add(p, v2_i32_from_i16(pt->p));
            v2_i32            dt = v2_sub(pp, po);
            i32               ls = v2_lensq(dt);
            if (r2 <= ls) continue;
            dt    = v2_setlen(dt, r);
            dt    = v2_add(dt, po);
            dt    = v2_sub(dt, p);
            pt->p = v2_i16_from_i32(dt, 0);
        }
    }
}

void deco_verlet_animate(game_s *g)
{
    for (u32 n = 0; n < g->n_deco_verlet; n++) {
        deco_verlet_s *d = &g->deco_verlet[n];
        deco_verlet_animate_single(g, d);
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        deco_verlet_obj_collision(g, ohero, 500);
    }
}

void deco_verlet_animate_single(game_s *g, deco_verlet_s *d)
{
    for (u32 n = 1; n < d->n_pt; n++) {
        deco_verlet_pt_s *pt  = &d->pt[n];
        v2_i16            tmp = pt->p;
        v2_i16            dt  = v2_i16_sub(pt->p, pt->pp);
        pt->p                 = v2_i16_add(pt->p, dt);
        pt->p                 = v2_i16_add(pt->p, d->grav);
        pt->pp                = tmp;
    }

    i32 r2 = pow2_i32(d->dist) + 1;

    for (u32 k = 0; k < d->n_it; k++) {
        for (u32 n = 1; n < d->n_pt; n++) {
            deco_verlet_pt_s *pt1   = &d->pt[n - 1];
            deco_verlet_pt_s *pt2   = &d->pt[n];
            v2_i16            dt    = v2_i16_sub(pt1->p, pt2->p);
            i32               lensq = v2_i16_lensq(dt);

            if (lensq <= r2) continue;
            i32 len = (i32)(sqrt_f32((f32)lensq) + .5f);
            i32 ll  = d->dist + ((len - d->dist) >> 1);

            v2_i16 vdt = {divr_i32((dt.x * ll), len),
                          divr_i32((dt.y * ll), len)};
            v2_i16 p1  = pt1->p;
            v2_i16 p2  = pt2->p;
            pt1->p     = v2_i16_add(p2, vdt);
            pt2->p     = v2_i16_sub(p1, vdt);
        }
        d->pt[0].p.x = 0;
        d->pt[0].p.y = 0;
        if (d->haspos_2) {
            d->pt[d->n_pt - 1].p.x = d->pos_2.x;
            d->pt[d->n_pt - 1].p.y = d->pos_2.y;
        }
    }
}

void deco_verlet_draw(game_s *g, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    for (u32 n = 0; n < g->n_deco_verlet; n++) {
        deco_verlet_s *d    = &g->deco_verlet[n];
        v2_i32         padd = v2_add(d->pos, cam);

        for (u32 k = 1; k < d->n_pt; k++) {
            v2_i32 v1 = v2_i32_from_i16(d->pt[k - 1].p);
            v2_i32 v2 = v2_i32_from_i16(d->pt[k + 0].p);
            v1        = v2_shr(v1, 6);
            v2        = v2_shr(v2, 6);
            v1        = v2_add(v1, padd);
            v2        = v2_add(v2, padd);
            gfx_lin_thick(ctx, v1, v2, GFX_COL_BLACK, 2);
        }
    }
}