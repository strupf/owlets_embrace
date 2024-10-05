// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gfx.h"
#include "util/mathfunc.h"

#ifndef SPRBLIT_FUNCNAME
#define SPRBLIT_FUNCNAME gfx_spr_func
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#endif

#if SPRBLIT_FLIPPEDX
#define SPRBLIT_GET_WORD(X)              brev32(X)
#define SPRBLIT_CHECK_PS(PS, PS_L, PS_R) ((PS_L) < (PS))
#else
#define SPRBLIT_GET_WORD(X)              (X)
#define SPRBLIT_CHECK_PS(PS, PS_L, PS_R) ((PS) < (PS_R))
#endif
#if SPRBLIT_DST_MASK
#define SPRBLIT_FUNCTION(DP, DM, SP, SM, PT, M) spr_blit_pm(DP, DM, SP, (SM) & (PT), M)
#else
#define SPRBLIT_FUNCTION(DP, DM, SP, SM, PT, M) spr_blit_p(DP, SP, (SM) & (PT), M)
#endif

#define SPR_USE_ASSERT 1

#if SPR_USE_ASSERT
#ifndef spr_assert
#define spr_assert assert
#endif
#else
#define spr_assert(X)
#endif

void SPRBLIT_FUNCNAME(gfx_ctx_s ctx, texrec_s trec, v2_i32 psrc, i32 flip, i32 mode)
{
    i32 rx = trec.r.x;
    i32 ry = trec.r.y;
    i32 rw = trec.r.w;
    i32 rh = trec.r.h;
    i32 px = psrc.x;
    i32 py = psrc.y;
    i32 x1 = max_i32(ctx.clip_x1, px);
    i32 y1 = max_i32(ctx.clip_y1, py);
    i32 x2 = min_i32(ctx.clip_x2, px + rw - 1);
    i32 y2 = min_i32(ctx.clip_y2, py + rh - 1);
    if (x2 < x1 || y2 < y1) return;

    tex_s dst  = ctx.dst;
    tex_s src  = trec.t;
    i32   sy   = flip & SPR_FLIP_Y ? -1 : +1;
    i32   sx   = 1 - 2 * SPRBLIT_FLIPPEDX;
    i32   di   = +1 << SPRBLIT_DST_MASK;
    i32   si   = sx << SPRBLIT_SRC_MASK;
    i32   wd   = dst.wword >> SPRBLIT_DST_MASK;
    i32   ws   = src.wword >> SPRBLIT_SRC_MASK;
    i32   id   = dst.wword;
    i32   is   = src.wword * sy;
    i32   ww   = (x2 >> 5) - (x1 >> 5); // number of destination words - 1
    u32   cl   = bswap32(0xFFFFFFFFU >> (31 & x1));
    u32   cr   = bswap32(0xFFFFFFFFU << (31 & (u32)(-x2 - 1)));
    i32   u1   = rx - sx * px + (SPRBLIT_FLIPPEDX ? rw - (x2 + 1) : x1); // first bit index in src row
    i32   os   = 31 & (sx * u1 - SPRBLIT_FLIPPEDX * (x2 - x1 + 1));
    i32   of   = os - (x1 & 31);
    i32   sn   = ((os + x2 - x1) >> 5) << SPRBLIT_SRC_MASK;
    i32   ll   = 31 & of;
    i32   rr   = 32 - ll;
    i32   ys   = ry + sy * (y1 - py) + (sy < 0) * (rh - 1);
    u32  *pt_y = ctx.pat.p;
    u32  *pd_y = &((u32 *)dst.px)[((x1 >> 5) + y1 * wd) << SPRBLIT_DST_MASK];
    u32  *ps_y = &((u32 *)src.px)[((u1 >> 5) + ys * ws) << SPRBLIT_SRC_MASK];

    spr_assert(0 <= (u1 >> 5) && ((u1 >> 5) + (sn >> SPRBLIT_SRC_MASK)) < ws);

    for (i32 yd = y1; yd <= y2; yd++, ys += sy, pd_y += id, ps_y += is) {
        spr_assert(0 <= ys && ys < src.h);
        u32 *ps_l = ps_y;
        u32 *ps_r = ps_y + sn;
        u32 *ps   = ps_y + sn * SPRBLIT_FLIPPEDX;
        u32 *pd   = pd_y;
#if SPR_USE_ASSERT
        u32 *ps_a = &((u32 *)src.px)[(0 + ys * ws) << SPRBLIT_SRC_MASK];
        u32 *ps_b = &((u32 *)src.px)[(ws - 1 + ys * ws) << SPRBLIT_SRC_MASK];
        spr_assert(ps_a <= ps && ps <= ps_b);
#endif

        u32 pt = pt_y[yd & 7];
        u32 zp = SPRBLIT_GET_WORD(bswap32(*(ps + 0)));
        u32 sp = zp << ll;
#if SPRBLIT_SRC_MASK
        u32 zm = SPRBLIT_GET_WORD(bswap32(*(ps + 1)));
        u32 sm = zm << ll;
#else
        u32 sm = cl;
#endif

        if (0 < sn && 0 <= of) {
            ps += si;
            spr_assert(ps_a <= ps && ps <= ps_b);
            zp = SPRBLIT_GET_WORD(bswap32(*(ps + 0)));
#if SPRBLIT_SRC_MASK
            zm = SPRBLIT_GET_WORD(bswap32(*(ps + 1)));
#endif
        }

        sp |= (u32)((u64)zp >> rr); // shifting 32 bit types by 32+ undefined
        sp = bswap32(sp);
#if SPRBLIT_SRC_MASK
        sm |= (u32)((u64)zm >> rr);
        sm = bswap32(sm) & cl; // apply left clip
#endif

        if (ww) { // bitmap spans over 2+ dst words
            SPRBLIT_FUNCTION(pd, pd + 1, sp, sm, pt, mode);
            pd += di;

            for (u32 *pi = pd + ((ww - 1) << SPRBLIT_DST_MASK); pd != pi; pd += di) {
                sp = zp << ll;
                ps += si;
                spr_assert(ps_a <= ps && ps <= ps_b);
                zp = SPRBLIT_GET_WORD(bswap32(*(ps + 0)));
                sp |= (u32)((u64)zp >> rr);
                sp = bswap32(sp);
#if SPRBLIT_SRC_MASK
                sm = zm << ll;
                zm = SPRBLIT_GET_WORD(bswap32(*(ps + 1)));
                sm |= (u32)((u64)zm >> rr);
                sm = bswap32(sm);
#endif
                SPRBLIT_FUNCTION(pd, pd + 1, sp, sm, pt, mode);
            }
            sp = zp << ll;
#if SPRBLIT_SRC_MASK
            sm = zm << ll;
#else
            sm = 0xFFFFFFFFU;
#endif

            if (SPRBLIT_CHECK_PS(ps, ps_l, ps_r)) {
                ps += si;
                spr_assert(ps_a <= ps && ps <= ps_b);
                zp = SPRBLIT_GET_WORD(bswap32(*(ps + 0)));
                sp |= (u32)((u64)zp >> rr);
#if SPRBLIT_SRC_MASK
                zm = SPRBLIT_GET_WORD(bswap32(*(ps + 1)));
                sm |= (u32)((u64)zm >> rr);
#endif
            }
            sp = bswap32(sp);
#if SPRBLIT_SRC_MASK
            sm = bswap32(sm);
#endif
        }
        sm &= cr; // apply right clip
        SPRBLIT_FUNCTION(pd, pd + 1, sp, sm, pt, mode);
    }
}

#undef SPRBLIT_GET_WORD
#undef SPRBLIT_CHECK_PS
#undef SPRBLIT_FUNCTION