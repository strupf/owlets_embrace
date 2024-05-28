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
    for (i32 n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->vel_q8.x >> 4;
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
    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  trgrass;
    trgrass.t = asset_tex(TEXID_PLANTS);
    for (i32 n = 0; n < g->n_grass; n++) {
        grass_s *gr     = &g->grass[n];
        rec_i32  rgrass = {gr->pos.x - 8, gr->pos.y - 8, 32, 32};
        if (!overlap_rec(rgrass, camrec)) continue;

        v2_i32 pos = v2_add(gr->pos, camoffset);

        // prerender?
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

void wiggle_add(game_s *g, map_obj_s *mo)
{
    wiggle_deco_s *wd = &g->wiggle_deco[g->n_wiggle_deco++];
    *wd               = (wiggle_deco_s){0};
    if (str_eq_nc(mo->name, "Wiggle2")) {
        wd->tr     = asset_texrec(TEXID_WIGGLE_DECO, 0, 64, 64, 64);
        wd->r      = (rec_i32){mo->x - mo->w / 2, mo->y - mo->h, mo->w, mo->h};
        wd->offs.x = -16;
        wd->offs.y = -48;
    } else if (str_eq_nc(mo->name, "Wiggle1")) {
        wd->tr     = asset_texrec(TEXID_WIGGLE_DECO, 0, 0, 64, 64);
        wd->r      = (rec_i32){mo->x - mo->w / 2, mo->y - mo->h, mo->w, mo->h};
        wd->offs.x = -16;
        wd->offs.y = -48;
    }
}

void wiggle_animate(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;

    rec_i32 heroaabb = obj_aabb(ohero);
    for (i32 n = 0; n < g->n_wiggle_deco; n++) {
        wiggle_deco_s *wd = &g->wiggle_deco[n];
        if (wd->t) {
            wd->t--;
            wd->tr.r.x = 64 * ((wd->t >> 2) & 3);
        }

        if (overlap_rec(wd->r, heroaabb)) {
            if (!wd->overlaps) {
                wd->overlaps = 1;
                wd->t        = 20;
            }
        } else {
            wd->overlaps = 0;
        }
    }
}

void wiggle_draw(game_s *g, v2_i32 camoffset)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    for (i32 n = 0; n < g->n_wiggle_deco; n++) {
        wiggle_deco_s *wd     = &g->wiggle_deco[n];
        v2_i32         wd_pos = {wd->r.x + wd->offs.x, wd->r.y + wd->offs.y};
        gfx_spr(ctx, wd->tr, v2_add(wd_pos, camoffset), 0, 0);
    }
}