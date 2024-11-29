// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i16 p;
} spiderboss_leg_s;

typedef struct {
    spiderboss_leg_s legs[8];
} spiderboss_s;

static_assert(sizeof(spiderboss_s) <= OBJ_MEM_BYTES, "spiderboss");

void spiderboss_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o = obj_create(g);
    spiderboss_s *s = (spiderboss_s *)o->mem;
    o->ID           = OBJ_ID_SPIDERBOSS;
    o->w            = 32;
    o->h            = 32;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->flags        = OBJ_FLAG_RENDER_AABB;
    for (i32 n = 0; n < 8; n++) {
        s->legs[n].p = (v2_i16){-20, 0};
    }
}

void spiderboss_on_update(g_s *g, obj_s *o)
{
    spiderboss_s *s = (spiderboss_s *)o->mem;
    for (i32 n = 0; n < 8; n++) {
        s->legs[n].p.x = -55 + ((45 * sin_q16(g->gameplay_tick << 10)) >> 16);
        s->legs[n].p.y = 0 + ((20 * sin_q16(g->gameplay_tick << 11)) >> 16);
    }
}

void spiderboss_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    spiderboss_s *s   = (spiderboss_s *)o->mem;
    gfx_ctx_s     ctx = gfx_ctx_display();

    v2_i32 pc      = v2_add(cam, obj_pos_center(o));
    v2_i32 leg_ref = {0, -10};
    for (i32 n = 0; n < 1; n++) {
        spiderboss_leg_s *l  = &s->legs[n];
        v2_i32            p0 = {0};
        v2_i32            p1 = v2_i32_from_i16(l->p);
        v2_i32            pu, pv;
        i32               n_cir = intersect_cir(p0, 60, p1, 60, &pu, &pv);

        if (0 < n_cir) {
            v2_i32 pp = {0};
            if (n_cir == 1 ||
                v2_distancesq(pu, leg_ref) < v2_distancesq(pv, leg_ref)) {
                pp = pu;
            } else {
                pp = pv;
            }

            p0 = v2_add(p0, pc);
            p1 = v2_add(p1, pc);
            pp = v2_add(pp, pc);
            gfx_lin_thick(ctx, p0, pp, GFX_COL_BLACK, 3);
            gfx_lin_thick(ctx, p1, pp, GFX_COL_BLACK, 3);
        }
    }
}