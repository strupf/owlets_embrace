// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define BITER_NUM_SEGS   8
#define BITER_SEG_DIST   15
#define BITER_SEG_DISTSQ POW2(BITER_SEG_DIST)
#define BITER_MAX_R      (BITER_SEG_DIST * (BITER_NUM_SEGS - 1))

enum {
    BITER_ST_IDLE,
    BITER_ST_ATTACK_ANTICIPATE,
    BITER_ST_ATTACK,
    BITER_ST_ATTACK_COOLDOWN,
};

enum {
    BITER_SUBST_IDLE_WAIT,
    BITER_SUBST_IDLE_MOVE,
};

typedef struct {
    v2_i32 p;
} biter_seg_s;

typedef struct {
    v2_i32      p;
    v2_i32      p_rng;
    v2_i32      p_new;
    v2_i32      p_old;
    v2_i32      anchor;
    biter_seg_s segs[BITER_NUM_SEGS];
} biter_s;

static_assert(sizeof(biter_s) <= OBJ_MEM_BYTES, "Biter size");

obj_s *biter_create(g_s *g, v2_i32 p)
{
    obj_s   *o = obj_create(g);
    biter_s *b = (biter_s *)o->mem;
    o->ID      = OBJID_BITER;
    o->w       = 16;
    o->h       = 16;
    o->pos     = p;
    b->anchor  = p;
    b->p_old   = p;
    b->p_new   = p;
    for (i32 n = 0; n < BITER_NUM_SEGS; n++) {
        b->segs[n].p = p;
    }
    return o;
}

void biter_load(g_s *g, map_obj_s *mo)
{
    v2_i32   p = {mo->x + mo->w / 2,
                  mo->y + mo->h / 2};
    obj_s   *o = biter_create(g, p);
    biter_s *b = (biter_s *)o->mem;
}

void biter_constrain(obj_s *o)
{
    biter_s *b                    = (biter_s *)o->mem;
    v2_i32   p_head               = obj_pos_center(o);
    b->segs[0].p                  = p_head;
    b->segs[BITER_NUM_SEGS - 1].p = b->anchor;

    for (i32 k = 0; k < 2; k++) {
        for (i32 n = 1; n < BITER_NUM_SEGS; n++) {
            biter_seg_s *b1  = &b->segs[n - 1];
            biter_seg_s *b2  = &b->segs[n];
            v2_i32       p1  = b1->p;
            v2_i32       p2  = b2->p;
            v2_i32       dt  = v2_sub(p1, p2);
            i32          lsq = v2_lensq(dt);
            if (lsq <= BITER_SEG_DISTSQ) continue;

            i32    l     = sqrt_i32(lsq);
            i32    l_set = BITER_SEG_DIST + (l - BITER_SEG_DIST) / 2;
            v2_i32 dt2   = v2_setlenl(dt, l, l_set);
            b1->p        = v2_add(p2, dt2);
            b2->p        = v2_sub(p1, dt2);
        }
        b->segs[0].p                  = p_head;
        b->segs[BITER_NUM_SEGS - 1].p = b->anchor;
    }
}

void biter_on_update(g_s *g, obj_s *o)
{
    biter_s *b = (biter_s *)o->mem;
    v2_i32   p = b->p;
    o->subtimer++;

    switch (o->state) {
    case BITER_ST_IDLE: {
        o->timer++;
        if (100 <= o->timer) {
            o->timer  = 0;
            i32    rp = rngr_i32((BITER_MAX_R * 2) / 4,
                                 (BITER_MAX_R * 3) / 4);
            i32    ap = rngr_i32(0, 1 << 18);
            v2_i32 tp = {(rp * sin_q16(ap)) >> 16,
                         (rp * cos_q16(ap)) >> 16};
            b->p_old  = b->p_new;
            b->p_new  = v2_add(tp, b->anchor);
        }

        if (o->timer <= 25) {
            p.x = ease_in_sine(b->p_old.x, b->p_new.x, o->timer, 25);
            p.y = ease_in_sine(b->p_old.y, b->p_new.y, o->timer, 25);
        }

        b->p_rng.x = (10 * sin_q16(o->subtimer << 11)) >> 16;
        b->p_rng.y = (8 * cos_q16(o->subtimer << 10)) >> 16;
        break;
    }
    }

    b->p         = p;
    v2_i32 p_end = v2_add(p, b->p_rng);
    v2_i32 dt    = v2_sub(p_end, b->anchor);
    dt           = v2_truncate(dt, BITER_MAX_R);
    p_end        = v2_add(b->anchor, dt);

    o->pos.x = p_end.x - o->w / 2;
    o->pos.y = p_end.y - o->h / 2;
}

void biter_on_animate(g_s *g, obj_s *o)
{
    biter_s *b = (biter_s *)o->mem;
    biter_constrain(o);
}

void biter_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    biter_s  *b   = (biter_s *)o->mem;
    gfx_ctx_s ctx = gfx_ctx_display();

    for (i32 n = 0; n < BITER_NUM_SEGS; n++) {
        biter_seg_s *bs    = &b->segs[n];
        v2_i32       p_seg = v2_add(bs->p, cam);
        gfx_cir_fill(ctx, p_seg, 10, GFX_COL_WHITE);
    }
}