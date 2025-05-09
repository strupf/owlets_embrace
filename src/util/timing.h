// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TIMING_H
#define TIMING_H

#include "gamedef.h"

// utility to display frame time graphs on the Playdate

#define TIMING_ENABLED 0

#if TIMING_ENABLED
#define TIMING_SHOW_DEFAULT 0 // whether graphs are showing per default
//
#define TIMING_Y_SCL        1000.f // note: result in seconds
#define TIMING_W_GRAPH      64     // pixels
#define TIMING_H_GRAPH      21     // pixels
#define TIMING_N_PER_PX     4      // ticks per visible pixel

enum {
    TIMING_ID_TICK_AND_DRAW,
    TIMING_ID_TICK,
    TIMING_ID_DRAW,
    //
    TIMING_NUM_GRAPHS
};

typedef struct {
    f32 avg;
    f32 acc;
    f32 t0;
    u8  t[TIMING_W_GRAPH];
} timing_el_s;

typedef struct {
    bool32      show;
    i32         c;
    i32         n;
    timing_el_s el[TIMING_NUM_GRAPHS];
} timing_s;

extern timing_s TIMING;

static inline void timing_beg(i32 ID)
{
    TIMING.el[ID].t0 = pltf_seconds();
}

static inline void timing_end(i32 ID)
{
    timing_s *t = &TIMING;
    t->el[ID].acc += (pltf_seconds() - t->el[ID].t0);
}

static i32 timing_el_y_at(timing_el_s *el, i32 x)
{
    i32 n = (TIMING_W_GRAPH + x + TIMING.n) & (TIMING_W_GRAPH - 1);
    return TIMING_H_GRAPH - el->t[n];
}

static void timing_end_frame()
{
    timing_s *t = &TIMING;

    // update this frame
    t->c++;
    if (t->c == TIMING_N_PER_PX) {
        t->c = 0;
        t->n = (t->n + 1) & (TIMING_W_GRAPH - 1);
        for (i32 n = 0; n < TIMING_NUM_GRAPHS; n++) {
            timing_el_s *te = &t->el[n];
            te->t[t->n]     = 0;
        }
    }

    t->el[0].acc = t->el[1].acc + t->el[2].acc;
    for (i32 n = 0; n < TIMING_NUM_GRAPHS; n++) {
        timing_el_s *te = &t->el[n];
        te->avg         = te->acc;
        te->acc         = 0.f;
        te->t[t->n]     = clamp_i32((i32)(te->avg * TIMING_Y_SCL + 0.5f),
                                    te->t[t->n],
                                    TIMING_H_GRAPH - 1);
    }

    if (!t->show) return;

    // render
    gfx_ctx_s ctx     = gfx_ctx_display();
    i32       xleft   = 400 - TIMING_W_GRAPH;
    rec_i32   rcanvas = {xleft, 0, TIMING_W_GRAPH, (TIMING_H_GRAPH + 2) * TIMING_NUM_GRAPHS};
    gfx_rec_fill_opaque(ctx, rcanvas, PRIM_MODE_WHITE);

    for (i32 i = 0; i < TIMING_NUM_GRAPHS; i++) {
        timing_el_s *el      = &t->el[i];
        i32          yg      = (TIMING_H_GRAPH + 2) * i;
        rec_i32      rborder = {xleft, yg + TIMING_H_GRAPH, TIMING_W_GRAPH, 2};
        gfx_rec_fill_opaque(ctx, rborder, PRIM_MODE_BLACK);

        i32 yp = timing_el_y_at(el, 1);
        for (i32 x = 1; x < TIMING_W_GRAPH; x++) {
            i32 y = timing_el_y_at(el, x);
#if 1 // draw vertical line = slightly more expensive
            i32 y0 = min_i32(yp, y);
            i32 y1 = max_i32(yp, y);
            yp     = y;
            for (i32 yy = y0; yy <= y1; yy++) {
                tex_px_unsafe_opaque_black(ctx.dst, x + xleft, yg + yy);
            }
#else
            tex_px_unsafe_opaque_black(ctx.dst, x + xleft, yg + y);
#endif
        }
    }
}

#endif
#endif