// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// a chain like arm (tree root, tentacle?) connection from the background to something

#include "game.h"

#define TENDRILCONNECTION_MAX_NODES 24
#define TENDRILCONNECTION_L_SEG     24
#define TENDRILCONNECTION_Q         8

typedef struct {
    v2_i32 p_q8;
} tendril_node_s;

typedef struct tendrilconnection_s {
    i32 n_nodes;
    ALIGNAS(8)
    tendril_node_s seg[TENDRILCONNECTION_MAX_NODES];
} tendrilconnection_s;

void tendrilconnection_on_animate(g_s *g, obj_s *o);
void tendrilconnection_on_draw(g_s *g, obj_s *o, v2_i32 cam);

obj_s *tendrilconnection_create(g_s *g)
{
    obj_s               *o = obj_create(g);
    tendrilconnection_s *c = (tendrilconnection_s *)o->mem;
    o->ID                  = OBJID_TENDRILCONNECTION;
    o->render_priority     = RENDER_PRIO_BACKGROUND + 1;
    o->on_animate          = tendrilconnection_on_animate;
    o->on_draw             = tendrilconnection_on_draw;
    return o;
}

void tendrilconnection_setup(g_s *g, obj_s *o, v2_i32 p_start, v2_i32 p_end, i32 l_max)
{
    tendrilconnection_s *c = (tendrilconnection_s *)o->mem;

    c->n_nodes = clamp_i32(1 + l_max / TENDRILCONNECTION_L_SEG, 2, TENDRILCONNECTION_MAX_NODES);

    for (i32 n = 0; n < c->n_nodes; n++) {
        tendril_node_s *s = &c->seg[n];
        v2_i32          p = v2_i32_lerp(p_start, p_end, n, c->n_nodes - 1);
        s->p_q8           = v2_i32_shl(p, TENDRILCONNECTION_Q);
    }

    for (i32 k = 0; k < 10; k++) {
        tendrilconnection_on_animate(g, o);
    }
}

void tendrilconnection_on_animate(g_s *g, obj_s *o)
{
    tendrilconnection_s *c = (tendrilconnection_s *)o->mem;
    if (c->n_nodes <= 1) return;

    i32 l = TENDRILCONNECTION_L_SEG << TENDRILCONNECTION_Q;

    for (i32 k = (i32)c->n_nodes - 1; 1 <= k; k--) {
        tendril_node_s *sa = &c->seg[k - 1];
        tendril_node_s *sb = &c->seg[k];

        sa->p_q8.y += (i32)(1.8f * (f32)(1 << TENDRILCONNECTION_Q));
        v2_i32 p0    = sa->p_q8;
        v2_i32 p1    = sb->p_q8;
        v2_i32 dt    = v2_i32_sub(p1, p0);
        i32    len   = v2_i32_len_appr(dt);
        i32    new_l = l + ((len - l + 1) >> 1);
        v2_i32 vadd  = v2_i32_setlenl(dt, len, new_l);

        if (1 < k) {
            sa->p_q8 = v2_i32_sub(p1, vadd);
        }
        if (k < c->n_nodes - 1) {
            sb->p_q8 = v2_i32_add(p0, vadd);
        }
    }
}

void tendrilconnection_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    tendrilconnection_s *c = (tendrilconnection_s *)o->mem;
    if (c->n_nodes <= 1) return;

    static i32 kk = -100;
    kk++;
    // pltf_log("%i | %i\n", kk, atan2_i32(kk, -10));
    gfx_ctx_s ctx = gfx_ctx_display();

    gfx_ctx_s ctxp = ctx;
    for (i32 n = 1; n < c->n_nodes; n++) {
        tendril_node_s *s1 = &c->seg[n - 1];
        tendril_node_s *s2 = &c->seg[n];
        v2_i32          p1 = v2_i32_add(cam, v2_i32_shr(s1->p_q8, TENDRILCONNECTION_Q));
        v2_i32          p2 = v2_i32_add(cam, v2_i32_shr(s2->p_q8, TENDRILCONNECTION_Q));

        v2_i32 pdt = v2_i32_sub(p2, p1);
#if 0
        f32 ang = atan2_f(pdt.y, pdt.x);
        i32 img = modu_i32((i32)(64.f * ang / PI2_FLOAT), 64);
#else
        i32 ang = atan2_i32(pdt.y, pdt.x);
        i32 img = modu_i32(((64 * ang) >> 18) + 0, 64);
#endif
        texrec_s trr = asset_texrec(TEXID_ROOTS, 64, img * 64, 64, 64);
        v2_i32   pp  = v2_i32_lerp(p1, p2, 1, 2);
        pp.x -= 32;
        pp.y -= 32;

        i32 k    = c->n_nodes - 1 - (n);
        //  k                  = (n - 1);
        ctxp.pat = gfx_pattern_bayer_4x4(max_i32(10, GFX_PATTERN_MAX - k));
        gfx_spr(ctxp, trr, pp, 0, SPR_MODE_BLACK_ONLY_WHITE_PT_OPAQUE);
        // gfx_lin_thick(ctxp, p1, p2, PRIM_MODE_BLACK_WHITE, 14);
    }
}

void tendrilconnection_constrain_ends(obj_s *o, v2_i32 p_start, v2_i32 p_end)
{
    tendrilconnection_s *c = (tendrilconnection_s *)o->mem;
    if (c->n_nodes <= 1) return;

    tendril_node_s *s0 = &c->seg[0];
    tendril_node_s *s1 = &c->seg[c->n_nodes - 1];
    s0->p_q8           = v2_i32_shl(p_start, TENDRILCONNECTION_Q);
    s1->p_q8           = v2_i32_shl(p_end, TENDRILCONNECTION_Q);
}