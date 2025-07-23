// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 pt[24];
    i32    slen_q4[24];
    b32    circ;
    i32    n_pt;
    i32    k_q16;
    i32    l_q4;
    i32    x_q4;
} lookahead_s;

static_assert(sizeof(lookahead_s) <= OBJ_MEM_BYTES, "size");

void lookahead_on_update(g_s *g, obj_s *o);
void lookahead_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void lookahead_load(g_s *g, map_obj_s *mo)
{
    sizeof(lookahead_s);
    obj_s *o          = obj_create(g);
    o->ID             = OBJID_LOOKAHEAD;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->w              = mo->w;
    o->h              = mo->h;
    lookahead_s *la   = (lookahead_s *)o->mem;
    i32          n_pt = 0;
    la->circ          = map_obj_bool(mo, "Circ");
    v2_i16 *pt        = map_obj_arr(mo, "PT", &n_pt);
    la->pt[0]         = obj_pos_center(o);
    la->n_pt          = 1;
    o->on_draw        = lookahead_on_draw;
    for (i32 n = 0; n < n_pt; n++) {
        la->pt[la->n_pt].x = (i32)pt[n].x << 4;
        la->pt[la->n_pt].y = (i32)pt[n].y << 4;
        la->n_pt++;
    }

    for (i32 n = n_pt - 1; 0 <= n; n--) {
        la->pt[la->n_pt].x = (i32)pt[n].x << 4;
        la->pt[la->n_pt].y = (i32)pt[n].y << 4;
        la->n_pt++;
    }

    v2_i32 pt_q4[24];
    for (i32 n = 0; n < la->n_pt; n++) {
        pt_q4[n].x = la->pt[n].x << 4;
        pt_q4[n].y = la->pt[n].y << 4;
    }

    for (i32 n = 0; n < la->n_pt; n++) {
        i32 l1         = v2_i32_spline_seg_len(pt_q4, la->n_pt, n, 1);
        la->slen_q4[n] = l1;
        la->l_q4 += l1;
    }
}

void lookahead_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s    ctx = gfx_ctx_display();
    lookahead_s *la  = (lookahead_s *)o->mem;

    for (i32 n = 0; n < la->n_pt; n++) {
        for (i32 s = 0; s < 65536; s += 256) {
            v2_i32 p = v2_i32_spline(la->pt, la->n_pt, n, s, 1);
            p        = v2_i32_add(p, cam);
            gfx_rec_fill(ctx, (rec_i32){p.x, p.y, 2, 2}, PRIM_MODE_BLACK);
        }
    }
}

void lookahead_on_update(g_s *g, obj_s *o)
{
    lookahead_s *la = (lookahead_s *)o->mem;
    la->k_q16 += 1024;

    i32 n_at  = (la->k_q16 >> 16);
    i32 k_q16 = (la->k_q16 & 0xFFFF);
#if 0
    la->x_q4 += 32;

    i32 l    = 0;
    i32 n_at = 65535;
    i32 k_q16;

    for (i32 n = 0; n < la->n_pt; n++) {
        i32 xs = la->x_q4 - l;
        i32 ln = la->slen_q4[n];
        l += ln;
        if (la->x_q4 < l) {
            n_at  = n;
            k_q16 = (xs << 16) / ln;
            assert(0 <= k_q16 && k_q16 < 65536);
            break;
        }
    }
#endif

    if (la->n_pt <= n_at) {
        g->cam.has_trg      = 0;
        g->cam.trg_fade_spd = 128;
        o->on_update        = 0;
    } else {
        g->cam.trg = v2_i32_spline(la->pt, la->n_pt, n_at, k_q16, 1);
    }
}

void lookahead_start(g_s *g, obj_s *o)
{
    lookahead_s *la = (lookahead_s *)o->mem;
    if (!o->state) {
        o->on_update        = lookahead_on_update;
        o->state            = 1;
        g->cam.trg_fade_spd = 128;
        g->cam.has_trg      = 1;
    }
}