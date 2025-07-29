// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "fluid_area.h"
#include "app.h"
#include "game.h"

i32 fluid_pt_y(i32 y_q8)
{
    return ((y_q8 >> 8) + 8);
}

void fluid_area_destroy(g_s *g, fluid_area_s *a)
{
    *a = g->fluid_areas[--g->n_fluid_areas];
}

fluid_area_s *fluid_area_create(g_s *g, rec_i32 r, i32 type, b32 surface)
{
    if (g->n_fluid_areas == ARRLEN(g->fluid_areas)) return 0;

    fluid_area_s *a = &g->fluid_areas[g->n_fluid_areas++];
    a->type         = type;
    a->x            = r.x;
    a->y            = r.y;
    a->w            = r.w;
    a->h            = r.h;

    if (surface) {
        a->s.n   = r.w / (4 << (i32)(type == FLUID_AREA_LAVA)) + 3;
        a->s.pts = game_alloctn(g, fluid_pt_s, a->s.n);

        switch (type) {
        case FLUID_AREA_WATER:
            a->s.r     = 5;
            a->s.c1    = 2200;
            a->s.c2    = 120;
            a->s.d_q16 = 64200;
            a->s.steps = 4;
            break;
        case FLUID_AREA_LAVA:
            a->s.r     = 8;
            a->s.c1    = 3000;
            a->s.c2    = 50;
            a->s.d_q16 = 64000;
            a->s.steps = 1;
            break;
        }
    } else {
        a->s.pts   = 0;
        a->s.min_y = 0;
        a->s.max_y = 0;
    }

    return a;
}

void fluid_surface_step(fluid_surface_s *b)
{
    // forces and velocity integration
    for (i32 i = 0; i < b->steps; i++) {
        b->pts[0].y_q8        = b->pts[1].y_q8;
        b->pts[b->n - 1].y_q8 = b->pts[b->n - 2].y_q8;

        for (i32 n = 1; n < b->n - 1; n++) {
            fluid_pt_s *p = &b->pts[n];
            i32         f = 0;
            f += (p - 1)->y_q8 - p->y_q8;
            f += (p + 1)->y_q8 - p->y_q8;
            p->v_q8 += (f * b->c1 - p->y_q8 * b->c2) >> 16;
        }

        for (i32 n = b->n - 2; 1 <= n; n--) {
            fluid_pt_s *p = &b->pts[n];
            p->y_q8 += p->v_q8;
        }
    }

    b->min_y = I8_MAX;
    b->max_y = I8_MIN;

    i32 vmax = 0;
    // dampening
    for (i32 n = 0; n < b->n; n++) {
        fluid_pt_s *p = &b->pts[n];
        p->v_q8       = mul_q16((i32)b->d_q16, p->v_q8);
        p->v_q8 += rngr_sym_i32(b->r);
        i32 y    = fluid_pt_y(p->y_q8);
        b->min_y = min_i32(b->min_y, y);
        b->max_y = max_i32(b->max_y, y);
        vmax     = max_i32(vmax, p->v_q8);
    }
}

void fluid_area_update(fluid_area_s *b)
{
    if (b->s.pts) {
        b->tick++;
        if (b->ticks_to_idle) {
            b->ticks_to_idle--;
        }
        fluid_surface_step(&b->s);
    }
}

void fluid_area_impact(fluid_area_s *b, i32 x_mid, i32 w, i32 str, i32 type)
{
    i32 wi = b->type == FLUID_AREA_WATER ? 4 : 8;
    i32 x0 = x_mid - w / 2;
    i32 x1 = x0 + w;
    i32 i0 = max_i32(x0 / wi + 1, 1);
    i32 i1 = min_i32(x1 / wi + 1, b->s.n - 2);
    i32 id = i1 - i0;
    if (id == 0) return;

    b->ticks_to_idle = 400;

    for (i32 i = i0; i <= i1; i++) {
        fluid_pt_s *p = &b->s.pts[i];
        switch (type) {
        case FLUID_AREA_IMPACT_COS: {
            i32 k = -(cos_q15(((i - i0) << 17) / id) - 32768);
            p->v_q8 += (str * k) >> 16;
            break;
        }
        case FLUID_AREA_IMPACT_FLAT: {
            p->v_q8 += str;
            break;
        }
        }

        p->v_q8 = ssat(p->v_q8, 9);
    }
}

void fluid_area_draw(gfx_ctx_s ctx, fluid_area_s *b, v2_i32 cam, i32 pass)
{
    i32       fill_col = PRIM_MODE_WHITE;
    gfx_ctx_s ctx_fill = ctx;
    switch (pass) {
    case 0:
        fill_col     = PRIM_MODE_BLACK_WHITE;
        ctx_fill.pat = gfx_pattern_2x2(B2(11),
                                       B2(01));
        break;
    case 1:
        if (b->type == FLUID_AREA_LAVA) {
            fill_col = PRIM_MODE_BLACK_WHITE;

            i32 t   = b->tick << 10;
            i32 shr = ((4 * (sin_q15(t) + 32767)) / 65536);
            i32 shl = ((2 * (cos_q15(t * 3) + 32767)) / 65536);

            if ((shr ^ shl) & 1) { // align pattern to 50/50 in screen space
                shl++;
            }
            i32 pID = (b->tick / 12) % 19;
            if (pID <= 7) {
                if (4 <= pID) {
                    pID = 7 - pID;
                }
            } else if (10 <= pID && pID <= (10 + 5)) {
                pID -= 10;
                if (3 <= pID) {
                    pID = 5 - pID;
                }
            } else {
                pID = 0;
            }
        } else {
            fill_col     = PRIM_MODE_BLACK;
            ctx_fill.pat = gfx_pattern_2x2(B2(10),
                                           B2(01));
        }

        break;
    }

    i32 bx = b->x + cam.x;
    i32 by = b->y + cam.y;
    i32 i0 = 1;
    i32 i1 = b->s.n - 2;
    i32 wi = b->type == FLUID_AREA_WATER ? 4 : 8;

    // fill in the general area
    if (b->s.pts) {
        for (i32 i = i0; i <= i1; i++) {
            i32     y  = fluid_pt_y(b->s.pts[i].y_q8);
            rec_i32 rp = {bx + ((i - 1) * wi),
                          by + y,
                          wi,
                          b->s.max_y - y};
            gfx_rec_fill_opaque(ctx_fill, rp, fill_col);
        }
    }

    rec_i32 rfill = {bx, by + b->s.max_y, b->w, b->h - b->s.max_y};
    gfx_rec_fill_opaque(ctx_fill, rfill, fill_col);

    switch (pass) {
    case 0: { // only on 1st pass: lava bubbles (background)
        if (b->type != FLUID_AREA_LAVA) break;

        i32      bubanim = b->tick / 6;
        texrec_s trbubg  = asset_texrec(TEXID_FLSURF, 0, 16, 32, 16);

        for (i32 i = i0 + 1; i <= i1 - 1; i += 2) {
            // determine if there is a bubble at position i
            // make it look kinda random
            bool32 isbub_index = ((i) % 7) == 0 || ((i >> 1) % 14) <= 1;
            if (!isbub_index) continue;

            // loop over 32 "frames", but only draw if it's lower than 8
            i32 bubframe = (bubanim - (i * 3)) & 31;
            if (bubframe < 8) {
                i32    y0 = fluid_pt_y(b->s.pts[i].y_q8);
                v2_i32 p  = {bx + ((i - 1) * wi) - 16,
                             by + y0 - 14};
                trbubg.x  = bubframe * 32;
                gfx_spr(ctx, trbubg, p, 0, 0);
            }
        }
        break;
    }
    case 1: { // only on 2nd pass: surface sprite overlay (foreground)
        texrec_s trsurf = asset_texrec(TEXID_FLSURF, 0,
                                       b->type == FLUID_AREA_LAVA ? 8 : 0,
                                       wi, 8);
        for (i32 i = i0; i <= i1; i++) {
            i32    y0 = fluid_pt_y(b->s.pts[i + 0].y_q8);
            i32    y1 = fluid_pt_y(b->s.pts[i + 1].y_q8);
            v2_i32 p  = {bx + (i - 1) * wi,
                         by + y0 - 4};
            trsurf.x  = wi * (2 + clamp_sym_i32(y1 - y0, 2));
            gfx_spr(ctx, trsurf, p, 0, 0);
        }
        break;
    }
    }
}

// returns [0, r.h] indicating water height inside of rec
i32 water_depth_rec(g_s *g, rec_i32 r)
{
    i32 depth   = 0;
    i32 rbottom = r.y + r.h;

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *a  = &g->fluid_areas[n];
        rec_i32       rf = {a->x, a->y, a->w, a->h};
        if (overlap_rec(r, rf)) {
            i32 d = min_i32(r.h, rbottom - rf.y);
            depth = max_i32(depth, d);
        }
    }

    return depth;
}
