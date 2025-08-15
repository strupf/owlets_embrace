// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss/boss_plant.h"
#include "game.h"

// TENTACLE LOGIC
#define BPLANT_TENTACLE_TICKS_APPEAR    10
#define BPLANT_TENTACLE_TICKS_SHOW      80
#define BPLANT_TENTACLE_TICKS_DISAPPEAR 16
#define BPLANT_TENTACLE_TICKS_FADE_OUT  16
#define BPLANT_TENTACLE_N_SEG           16

typedef struct {
    u16          t_emerge;
    u16          t_active;
    bplant_seg_s seg[BPLANT_TENTACLE_N_SEG];
} boss_plant_tentacle_s;

enum {
    BPLANT_TENTACLE_ANTICIPATE,
    BPLANT_TENTACLE_APPEAR,
    BPLANT_TENTACLE_SHOWN,
    BPLANT_TENTACLE_DISAPPEAR,
    BPLANT_TENTACLE_FADE_OUT_WAIT,
    BPLANT_TENTACLE_FADE_OUT
};

static inline i32 boss_plant_tentacle_dat(i32 n)
{
    return (max_i32((20 - n) >> 2, 2) << 8);
}

void boss_plant_tentacle_on_update(g_s *g, obj_s *o)
{
    boss_plant_tentacle_s *b = (boss_plant_tentacle_s *)o->mem;
    o->timer++;

    switch (o->state) {
    case BPLANT_TENTACLE_ANTICIPATE: {
        if (b->t_emerge <= o->timer) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case BPLANT_TENTACLE_APPEAR: {
        if (BPLANT_TENTACLE_TICKS_APPEAR <= o->timer) {
            o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case BPLANT_TENTACLE_SHOWN: {
        if (b->t_active <= o->timer) {
            o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case BPLANT_TENTACLE_DISAPPEAR: {
        if (BPLANT_TENTACLE_TICKS_DISAPPEAR <= o->timer) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case BPLANT_TENTACLE_FADE_OUT_WAIT: {
        if (6 <= o->timer) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case BPLANT_TENTACLE_FADE_OUT: {
        if (BPLANT_TENTACLE_TICKS_FADE_OUT <= o->timer) {
            obj_delete(g, o);
        }
        break;
    }
    }
}

#define TEX_TENTACLE_W_WORDS 4
#define TEX_TENTACLE_H       120

void boss_plant_tentacle_on_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx)
{
    boss_plant_tentacle_s *b = (boss_plant_tentacle_s *)o->mem;

    v2_i32 pground = {cam.x + o->pos.x + o->w / 2,
                      cam.y + o->pos.y + o->h};
    v2_i32 pos1    = pground;

    switch (o->state) {
    case BPLANT_TENTACLE_FADE_OUT_WAIT:
    case BPLANT_TENTACLE_FADE_OUT:
    case BPLANT_TENTACLE_ANTICIPATE: {
        pos1.y += TEX_TENTACLE_H;
        break;
    }
    case BPLANT_TENTACLE_APPEAR: {
        pos1.y += lerp_i32(TEX_TENTACLE_H, 0,
                           o->timer, BPLANT_TENTACLE_TICKS_APPEAR);
        break;
    }
    case BPLANT_TENTACLE_SHOWN: {
        break;
    }
    case BPLANT_TENTACLE_DISAPPEAR: {
        pos1.y += lerp_i32(0, TEX_TENTACLE_H,
                           o->timer, BPLANT_TENTACLE_TICKS_DISAPPEAR);
        break;
    }
    }

    // render tentacle to separate texture for black outline
    ALIGNAS(8) u32 texpx[2 * TEX_TENTACLE_W_WORDS * TEX_TENTACLE_H] = {0};

    tex_s textmp = {0};
    textmp.px    = texpx;
    textmp.wword = TEX_TENTACLE_W_WORDS * 2;
    textmp.w     = TEX_TENTACLE_W_WORDS * 32;
    textmp.h     = TEX_TENTACLE_H;
    textmp.fmt   = 1;

    v2_i32    texpos = {pos1.x - textmp.w / 2, pos1.y - TEX_TENTACLE_H};
    v2_i32    pos2   = {textmp.w / 2, textmp.h};
    gfx_ctx_s c      = gfx_ctx_default(textmp);

    // tentacle
    for (i32 n = 0; n < BPLANT_TENTACLE_N_SEG; n++) {
        v2_i32    pos = v2_i32_add(pos2, v2_i32_shr(b->seg[n].p_q8, 8));
        i32       d   = max_i32(30 - n * 2, 3);
        gfx_ctx_s c2  = c;
        c2.pat        = gfx_pattern_50();
        gfx_cir_fill(c2, pos, d, PRIM_MODE_BLACK_WHITE);
    }

    // middle shading
    for (i32 n = 3; n < BPLANT_TENTACLE_N_SEG; n++) {
        v2_i32 pos = v2_i32_add(pos2, v2_i32_shr(b->seg[n].p_q8, 8));
        i32    d   = 22 - n * 2;
        if (d <= 0) break;
        gfx_cir_fill(c, pos, d, PRIM_MODE_WHITE);
    }

    // outline in black
    tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
    tex_outline_col_ext(textmp, GFX_COL_BLACK, 0);

    // outer spikes
    for (i32 n = 0; n < BPLANT_TENTACLE_N_SEG; n++) {
        v2_i32 pos = v2_i32_add(pos2, v2_i32_shr(b->seg[n].p_q8, 8));
        i32    dh  = max_i32(32 - n * 2, 3) >> 1;
        if (3 <= dh && (n & 1) == 0) {
            for (i32 i = 2; i < 7; i++) {
                rec_i32 r1 = {pos.x - dh - i, pos.y - i + 2, i, 1};
                rec_i32 r2 = {pos.x + dh + 1, pos.y - i + 2, i, 1};
                gfx_rec_fill(c, r1, PRIM_MODE_BLACK);
                gfx_rec_fill(c, r2, PRIM_MODE_BLACK);
            }
        }
    }

    // draw to boss texture
    gfx_spr(ctx, texrec_from_tex(textmp), texpos, 0, 0);

    // rumbling ground
    i32    fr_ground = 0;
    i32    w_ground  = 96;
    i32    h_ground  = 32;
    v2_i32 posground = {pground.x - w_ground / 2, pground.y - h_ground};
    if (o->state == BPLANT_TENTACLE_ANTICIPATE ||
        o->state == BPLANT_TENTACLE_APPEAR) {
        if ((o->animation >> 2) & 1) { // shaking animation
            fr_ground = 1;
        }
    } else {
        fr_ground = 2; // emerged frame
    }

    if (o->state == BPLANT_TENTACLE_FADE_OUT) { // let ground disappear
        posground.y += lerp_i32(0, 32, o->timer, BPLANT_TENTACLE_TICKS_FADE_OUT);
    }
    texrec_s trground = asset_texrec(TEXID_BOSSPLANT,
                                     w_ground * 5,
                                     h_ground * (9 + fr_ground),
                                     w_ground,
                                     h_ground);
    posground.x &= ~1;
    posground.y &= ~1;
    gfx_spr(ctx, trground, posground, 0, 0);
}

void boss_plant_tentacle_on_animate(g_s *g, obj_s *o)
{
    boss_plant_tentacle_s *b = (boss_plant_tentacle_s *)o->mem;
    o->animation++;

    // wobble end segment around
    bplant_seg_s *bseg = &b->seg[BPLANT_TENTACLE_N_SEG - 1];
    bseg->p_q8.x       = ((4096 * sin_q15(o->animation << 12)) >> 15);
    bseg->p_q8.y += ((128 * sin_q15((o->animation << 11) - 65536)) >> 15);

    // dampen the bottom segments -> more tangent
    bplant_seg_s *seg = b->seg;
    seg[0].p_q8.x /= 4;
    seg[1].p_q8.x /= 4;
    seg[2].p_q8.x /= 3;
    seg[3].p_q8.x /= 2;

    // verlet animation
    for (i32 k = 1; k < BPLANT_TENTACLE_N_SEG - 1; k++) {
        bplant_seg_s *s0  = &b->seg[k - 1];
        v2_i32        tmp = s0->p_q8;
        s0->p_q8.x += (s0->p_q8.x - s0->pp_q8.x);
        s0->p_q8.y += (s0->p_q8.y - s0->pp_q8.y);
        s0->pp_q8 = tmp;
    }

    // constrain verlet
    for (i32 i = 0; i < 3; i++) {
        for (i32 k = 1; k < BPLANT_TENTACLE_N_SEG; k++) {
            bplant_seg_s *s0  = &b->seg[k - 1];
            bplant_seg_s *s1  = &b->seg[k];
            v2_i32        p0  = s0->p_q8;
            v2_i32        p1  = s1->p_q8;
            v2_i32        dt  = v2_i32_sub(p1, p0);
            i32           len = v2_i32_len_appr(dt);
            i32           l   = boss_plant_tentacle_dat(k - 1);
            if (len <= l) continue;

            i32    new_l = l + ((len - l) >> 1);
            v2_i32 vadd  = v2_i32_setlenl(dt, len, new_l);

            if (1 < k) {
                s0->p_q8 = v2_i32_sub(p1, vadd);
            }
            if (k < BPLANT_TENTACLE_N_SEG - 1) {
                s1->p_q8 = v2_i32_add(p0, vadd);
            }
        }
    }
}

obj_s *boss_plant_tentacle_emerge(g_s *g, i32 x, i32 y, i32 t_emerge, i32 t_active)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_BOSS_PLANT_TENTACLE;
    o->w          = 24;
    o->h          = 90;
    o->pos.x      = g->boss.plant.x + x - o->w / 2;
    o->pos.y      = g->boss.plant.y + y - o->h;
    o->on_animate = boss_plant_tentacle_on_animate;
    o->on_update  = boss_plant_tentacle_on_update;
    o->state      = BPLANT_TENTACLE_ANTICIPATE;
    o->animation  = rng_i32() & 255;

    boss_plant_tentacle_s *b = (boss_plant_tentacle_s *)o->mem;
    b->t_emerge              = t_emerge;
    b->t_active              = t_active;
    i32 segp                 = 0;
    for (i32 n = 0; n < BPLANT_TENTACLE_N_SEG; n++) {
        b->seg[n].p_q8.y = segp;
        b->seg[n].pp_q8  = b->seg[n].p_q8;
        segp -= boss_plant_tentacle_dat(n) << 1;
    }
    return o;
}