// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gfx.h"
#include "util/mathfunc.h"

// #define SPR_FUNC_PF  xxx
// #define SPR_FUNC_SM 1
// #define SPR_FUNC_DM 1
// #define SPR_FUNC_FX       0

#if SPR_FUNC_DM
#define SPR_DST_INCR                     2
#define SPR_FUNC_BLIT(DP, DM, SP, SM, M) spr_blit_pm(DP, DM, SP, SM, M)
#else
#define SPR_DST_INCR                     1
#define SPR_FUNC_BLIT(DP, DM, SP, SM, M) spr_blit_p(DP, SP, SM, M)
#endif // dst mask

#if SPR_FUNC_FX
#define SPR_FUNC_CMP(A, B) ((A) > (B))
#define SPR_FUNC_GET(A)    brev32(A)
#if SPR_FUNC_SM
#define SPR_SRC_INCR -2
#else
#define SPR_SRC_INCR -1
#endif // mask
#else
#define SPR_FUNC_CMP(A, B) ((A) < (B))
#define SPR_FUNC_GET(A)    (A)
#if SPR_FUNC_SM
#define SPR_SRC_INCR +2
#else
#define SPR_SRC_INCR +1
#endif // mask
#endif // fx

void SPR_FUNC_PF(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode)
{
#if SPR_FUNC_SM
    assert(src.t.fmt == TEX_FMT_MASK);
#else
    assert(src.t.fmt == TEX_FMT_OPAQUE);
#endif
#if SPR_FUNC_DM
    assert(ctx.dst.fmt == TEX_FMT_MASK);
#else
    assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
#endif

    // area bounds on canvas [x1/y1, x2/y2)
    i32 x1 = max_i32(pos.x, ctx.clip_x1);               // inclusive
    i32 y1 = max_i32(pos.y, ctx.clip_y1);               // inclusive
    i32 x2 = min_i32(pos.x + src.r.w - 1, ctx.clip_x2); // inclusive
    i32 y2 = min_i32(pos.y + src.r.h - 1, ctx.clip_y2); // inclusive
    if (x2 < x1) return;

#if SPR_FUNC_FX
    i32 sx = -1;
#else
    i32 sx = +1;
#endif
    tex_s dtex    = ctx.dst;
    tex_s stex    = src.t;
    i32   sy      = (flip & SPR_FLIP_Y) ? -1 : +1;                             // sign flip y
    i32   nb      = (x2 + 1) - x1;                                             // number of bits in a row
    i32   od      = x1 & 31;                                                   // bitoffset in dst
    i32   dm      = (od + nb - 1) >> 5;                                        // number of touched dst words -1
    u32   ml      = bswap32(0xFFFFFFFFU >> (31 & od));                         // mask to cut off boundary left
    u32   mr      = bswap32(0xFFFFFFFFU << (31 & (u32)(-od - nb)));            // mask to cut off boundary right
    i32   u1      = src.r.x - sx * pos.x + (sx < 0 ? src.r.w - (x2 + 1) : x1); // first bit index in src row
    i32   os      = (u32)(sx * u1 - (sx < 0) * nb) & 31;                       // bitoffset in src
    i32   da      = 1 + (dtex.fmt == TEX_FMT_MASK);                            // number of words to next logical pixel word in dst
    i32   sa      = 1 + (stex.fmt == TEX_FMT_MASK);                            // number of words to next logical pixel word in src
    i32   sm      = ((os + nb - 1) >> 5) * sa;                                 // number of touched src words -1
    i32   of      = os - od;                                                   // alignment difference
    i32   l       = of & 31;                                                   // word left shift amount
    i32   r       = 32 - l;                                                    // word rght shift amound
    i32   src_wy1 = src.r.y + sy * (y1 - pos.y) + (sy < 0) * (src.r.h - 1);
    u32  *dst_p   = &dtex.px[(x1 >> 5) * da + dtex.wword * y1];      // dst pixel words
    u32  *src_p   = &stex.px[(u1 >> 5) * sa + stex.wword * src_wy1]; // src pixel words
    i32   incr_sp = stex.wword * sy;
    i32   incr_dp = dtex.wword;

    assert(da == SPR_DST_INCR);
    assert(sa == sx * SPR_SRC_INCR);

    for (i32 y = y1; y <= y2; y++, src_p += incr_sp, dst_p += incr_dp) {
        u32 pt = ctx.pat.p[y & 7];
        if (pt == 0) continue;

#if SPR_FUNC_FX
        const u32 *restrict sp     = src_p + sm;
        const u32 *restrict sp_end = src_p;
#else
        const u32 *restrict sp     = src_p;
        const u32 *restrict sp_end = src_p + sm;
#endif
        u32 *restrict dp = dst_p;
        u32 p            = 0;
        u32 m            = 0;
        if (l == 0) { // same alignment, fast path
            p = SPR_FUNC_GET(*sp);
#if SPR_FUNC_SM
            m = SPR_FUNC_GET(*(sp + 1)) & ml;
#else
            m = ml;
#endif
            sp += SPR_SRC_INCR;
            for (i32 i = 0; i < dm; i++, sp += SPR_SRC_INCR, dp += SPR_DST_INCR) {
                SPR_FUNC_BLIT(dp, dp + 1, p, m & pt, mode);
                p = SPR_FUNC_GET(*sp);
#if SPR_FUNC_SM
                m = SPR_FUNC_GET(*(sp + 1));
#else
                m = 0xFFFFFFFFU;
#endif
            }
            SPR_FUNC_BLIT(dp, dp + 1, p, m & (pt & mr), mode);
            continue;
        }
        if (0 < of) { // first word
            p = SPR_FUNC_GET(bswap32(*sp)) << l;

#if SPR_FUNC_SM
            m = SPR_FUNC_GET(bswap32(*(sp + 1))) << l;
#else
            m = 0xFFFFFFFFU;
#endif
            if (0 < sm) {
                sp += SPR_SRC_INCR;
            }
        }

        p = bswap32(p | (SPR_FUNC_GET(bswap32(*sp)) >> r));
#if SPR_FUNC_SM
        m = bswap32(m | (SPR_FUNC_GET(bswap32(*(sp + 1))) >> r)) & ml;
#else
        m = ml;
#endif
        if (dm == 0) { // only one word long
            SPR_FUNC_BLIT(dp, dp + 1, p, m & (pt & mr), mode);
            continue;
        }
        SPR_FUNC_BLIT(dp, dp + 1, p, m & pt, mode);
        dp += SPR_DST_INCR;

        for (i32 i = 1; i < dm; i++, sp += SPR_SRC_INCR, dp += SPR_DST_INCR) { // middle words
            p = SPR_FUNC_GET(bswap32(*sp)) << l;
            p = bswap32(p | (SPR_FUNC_GET(bswap32(*(sp + SPR_SRC_INCR))) >> r));
#if SPR_FUNC_SM
            m = SPR_FUNC_GET(bswap32(*(sp + 1))) << l;
            m = bswap32(m | (SPR_FUNC_GET(bswap32(*(sp + 1 + SPR_SRC_INCR))) >> r));
#else
            m = 0xFFFFFFFFU;
#endif
            SPR_FUNC_BLIT(dp, dp + 1, p, m & pt, mode);
        }

        p = SPR_FUNC_GET(bswap32(*sp)) << l; // last word
#if SPR_FUNC_SM
        m = SPR_FUNC_GET(bswap32(*(sp + 1))) << l;
#else
        m = 0xFFFFFFFFU;
#endif
        if (SPR_FUNC_CMP(sp, sp_end)) { // this is different for reversed blitting!
            sp += SPR_SRC_INCR;
            p |= SPR_FUNC_GET(bswap32(*sp)) >> r;
#if SPR_FUNC_SM
            m |= SPR_FUNC_GET(bswap32(*(sp + 1))) >> r;
#endif
        }
        p = bswap32(p);
        m = bswap32(m);
        SPR_FUNC_BLIT(dp, dp + 1, p, m & (pt & mr), mode);
    }
}

#undef SPR_FUNC_CMP
#undef SPR_FUNC_GET
#undef SPR_SRC_INCR
#undef SPR_DST_INCR
#undef SPR_FUNC_BLIT