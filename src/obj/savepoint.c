// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define NUM_SAVEPOINT_BUTTERFLY 8

typedef struct {
    v2_i32 p_q8;
    v2_i32 v_q8;
    i32    ang;
    i16    frame;
} savepoint_butterfly_s;

typedef struct {
    savepoint_butterfly_s f[NUM_SAVEPOINT_BUTTERFLY];
} savepoint_s;

static_assert(sizeof(savepoint_s) <= OBJ_MEM_BYTES, "savepoint");

void savepoint_load(g_s *g, map_obj_s *mo)
{
    obj_s       *o = obj_create(g);
    savepoint_s *s = (savepoint_s *)o->mem;
    o->ID          = OBJID_SAVEPOINT;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = mo->h;
    for (i32 n = 0; n < NUM_SAVEPOINT_BUTTERFLY; n++) {
        savepoint_butterfly_s *f = s->f;
        f->p_q8.x                = rngr_sym_i32(256);
        f->p_q8.y                = rngr_sym_i32(256);
        f->ang                   = rngr_i32(0, 1 << 18);
        f->frame                 = rngr_i32(-1, +1);
    }
}

void savepoint_on_update(g_s *g, obj_s *o)
{
    savepoint_s *s = (savepoint_s *)o->mem;
    for (i32 n = 0; n < NUM_SAVEPOINT_BUTTERFLY; n++) {
        savepoint_butterfly_s *f = &s->f[n];

        for (i32 i = 0; i < NUM_SAVEPOINT_BUTTERFLY; i++) {
            if (i == n) continue;
            savepoint_butterfly_s *t    = &s->f[i];
            v2_i32                 v_dt = v2_sub(f->p_q8, t->p_q8);
            i32                    dst  = v2_len(v_dt);
            if (dst && dst < 2000) {
                v_dt    = v2_setlen(v_dt, (2000 - dst) / 20);
                f->v_q8 = v2_add(f->v_q8, v_dt);
            }
        }

#define SAVEPOINT_CENTER_R 64

        i32 dsq = v2_lensq(f->p_q8) >> 16; // distance squared in pixels
        if (SAVEPOINT_CENTER_R <= dsq) {   // keep around the savepoint
            v2_i32 d = v2_mulq(f->p_q8, dsq, 20);
            f->v_q8  = v2_sub(f->v_q8, d);
        } else { // but keep them from touching the center directly
            v2_i32 d = v2_mulq(f->p_q8, SAVEPOINT_CENTER_R - dsq, 1);
            f->v_q8  = v2_add(f->v_q8, d);
        }

        f->ang += rngr_sym_i32(100 << 8);
        f->v_q8.x += sin_q16(f->ang) >> 13;
        f->v_q8.y += cos_q16(f->ang) >> 13;
        if ((20 << 8) <= f->p_q8.y) { // keep from flying into the ground
            f->v_q8.y -= 10;
        }

        f->v_q8 = v2_truncate(f->v_q8, 128); // vmax
        f->v_q8 = f->v_q8;

        switch (f->frame) {
        case -1:
            if (-50 < f->v_q8.x) f->frame = 0;
            break;
        case +0:
            if (f->v_q8.x < -80) f->frame = -1;
            if (+80 < f->v_q8.x) f->frame = +1;
            break;
        case +1:
            if (f->v_q8.x < +50) f->frame = 0;
            break;
        }
    }

    for (i32 n = 0; n < NUM_SAVEPOINT_BUTTERFLY; n++) {
        s->f[n].p_q8 = v2_add(s->f[n].p_q8, s->f[n].v_q8);
    }
}

void savepoint_on_interact(g_s *g, obj_s *o)
{
    savepoint_s *s = (savepoint_s *)o->mem;
}

void savepoint_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    savepoint_s *s       = (savepoint_s *)o->mem;
    gfx_ctx_s    ctx     = gfx_ctx_display();
    tex_s        tex     = asset_tex(TEXID_SAVEPOINT);
    texrec_s     tr      = {tex, 0, 0, 16, 16};
    texrec_s     tr_tree = {tex, 0, 32, 64, 96};
    v2_i32       p_bf    = {o->pos.x + 36, o->pos.y + 8};
    v2_i32       p0      = v2_add(p_bf, cam);
    v2_i32       p_tree  = v2_add(o->pos, cam);
    p_tree.y -= 32;
    gfx_spr(ctx, tr_tree, p_tree, 0, 0);

    for (i32 n = 0; n < NUM_SAVEPOINT_BUTTERFLY; n++) {
        savepoint_butterfly_s *f = &s->f[n];
        v2_i32                 p = v2_add(v2_shr(f->p_q8, 8), p0);

        i32 frID = (n + (g->tick >> 2)) % 6;
        frID     = min_i32(frID == 5 ? 1 : frID, 2);
        tr.x     = frID * 16;

        i32 flip = 0;
        switch (f->frame) {
        case 0:
            frID = 3 + ((n + (g->tick / 10)) & 1);
            break;
        case 1:
            flip = SPR_FLIP_X;
            break;
        }

        gfx_spr(ctx, tr, p, flip, 0);
    }
}