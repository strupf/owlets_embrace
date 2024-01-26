// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gfx.h"
#include "spm.h"
#include "sys/sys.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/mem.h"
#include "util/str.h"

static void spr_blit(u32 *restrict dp, u32 *restrict dm,
                     u32 sp, u32 sm, int mode)
{
    switch (mode) {
    case SPR_MODE_INV: sp = ~sp; // fallthrough
    case SPR_MODE_COPY: *dp = (*dp & ~sm) | (sp & sm); break;
    case SPR_MODE_XOR: sp = ~sp; // fallthrough
    case SPR_MODE_NXOR: *dp = (*dp & ~sm) | ((*dp ^ sp) & sm); break;
    case SPR_MODE_WHITE_ONLY: sm &= sp; // fallthrough
    case SPR_MODE_WHITE: *dp |= sm; break;
    case SPR_MODE_BLACK_ONLY: sm &= ~sp; // fallthrough
    case SPR_MODE_BLACK: *dp &= ~sm; break;
    }

    if (dm) *dm |= sm;
}

static void spr_blit_fwd(u32 *restrict DP, const u32 *restrict SP,
                         u32 pt, int sa, int da, int l, int r,
                         u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
    u32 *restrict dp       = DP;
    const u32 *restrict sp = SP;
    u32 p                  = 0;
    u32 m                  = 0;

    if (l == 0) { // same alignment, fast path
        p = *sp;
        if (sa == 2) {
            m = *(sp + 1) & ml;
        } else {
            m = ml;
        }
        sp += sa;
        for (int i = 0; i < dm; i++) {
            spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
            p = *sp;
            if (sa == 2) {
                m = *(sp + 1);
            } else {
                m = 0xFFFFFFFFU;
            }
            sp += sa;
            dp += da;
        }
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
        return;
    }

    if (0 < offs) { // first word
        p = bswap32(*sp) << l;
        if (sa == 2) {
            m = bswap32(*(sp + 1)) << l;
        } else {
            m = 0xFFFFFFFFU;
        }
        if (0 < sm) {
            sp += sa;
        }
    }

    p = bswap32(p | (bswap32(*sp) >> r));
    if (sa == 2) {
        m = bswap32(m | (bswap32(*(sp + 1)) >> r)) & ml;
    } else {
        m = ml;
    }
    if (dm == 0) {
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
        return; // only one word long
    }

    spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
    dp += da;

    for (int i = 1; i < dm; i++) { // middle words without first and last word
        p = bswap32(*sp) << l;
        p = bswap32(p | (bswap32(*(sp + sa)) >> r));
        if (sa == 2) {
            m = bswap32(*(sp + 1)) << l;
            m = bswap32(m | (bswap32(*(sp + 3)) >> r));
        } else {
            m = 0xFFFFFFFFU;
        }
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
        sp += sa;
        dp += da;
    }

    p = bswap32(*sp) << l; // last word
    if (sa == 2) {
        m = bswap32(*(sp + 1)) << l;
    } else {
        m = 0xFFFFFFFFU;
    }
    if (sp < SP + sm) { // this is different for reversed blitting!
        sp += sa;
        p |= bswap32(*sp) >> r;
        if (sa == 2) {
            m |= bswap32(*(sp + 1)) >> r;
        }
    }
    p = bswap32(p);
    m = bswap32(m);
    spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
}

static void spr_blit_rev(u32 *restrict DP, const u32 *restrict SP,
                         u32 pt, int sa, int da, int l, int r,
                         u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
    u32 *restrict dp       = DP;
    const u32 *restrict sp = SP + sm;
    u32 p                  = 0;
    u32 m                  = 0;

    if (l == 0) { // same alignment, fast path
        p = brev32(*sp);
        if (sa == 2) {
            m = brev32(*(sp + 1)) & ml;
        } else {
            m = ml;
        }
        sp -= sa;
        for (int i = 0; i < dm; i++) {
            spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
            p = brev32(*sp);
            if (sa == 2) {
                m = brev32(*(sp + 1));
            } else {
                m = 0xFFFFFFFFU;
            }
            sp -= sa;
            dp += da;
        }
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
        return;
    }

    if (0 < offs) { // first word
        p = brev32(bswap32(*sp)) << l;
        if (sa == 2) {
            m = brev32(bswap32(*(sp + 1))) << l;
        } else {
            m = 0xFFFFFFFFU;
        }
        if (0 < sm) {
            sp -= sa;
        }
    }

    p = bswap32(p | (brev32(bswap32(*sp)) >> r));
    if (sa == 2) {
        m = bswap32(m | (brev32(bswap32(*(sp + 1))) >> r)) & ml;
    } else {
        m = ml;
    }
    if (dm == 0) {
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
        return; // only one word long
    }

    spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
    dp += da;

    for (int i = 1; i < dm; i++) { // middle words without first and last word
        p = brev32(bswap32(*sp)) << l;
        p = bswap32(p | (brev32(bswap32(*(sp - sa))) >> r));
        if (sa == 2) {
            m = brev32(bswap32(*(sp + 1))) << l;
            m = bswap32(m | (brev32(bswap32(*(sp - 1))) >> r));
        } else {
            m = 0xFFFFFFFFU;
        }
        spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
        sp -= sa;
        dp += da;
    }

    p = brev32(bswap32(*sp)) << l; // last word
    if (sa == 2) {
        m = brev32(bswap32(*(sp + 1))) << l;
    } else {
        m = 0xFFFFFFFFU;
    }
    if (SP < sp) {
        sp -= sa;
        p |= brev32(bswap32(*sp)) >> r;
        if (sa == 2) {
            m |= brev32(bswap32(*(sp + 1))) >> r;
        }
    }
    p = bswap32(p);
    m = bswap32(m);
    spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
}

static void spr_blit_X(u32 *restrict dp, u32 sp, u32 sm, int mode)
{
    switch (mode) {
    case SPR_MODE_INV: sp = ~sp; // fallthrough
    case SPR_MODE_COPY: *dp = (*dp & ~sm) | (sp & sm); break;
    case SPR_MODE_XOR: sp = ~sp; // fallthrough
    case SPR_MODE_NXOR: *dp = (*dp & ~sm) | ((*dp ^ sp) & sm); break;
    case SPR_MODE_WHITE_ONLY: sm &= sp; // fallthrough
    case SPR_MODE_WHITE: *dp |= sm; break;
    case SPR_MODE_BLACK_ONLY: sm &= ~sp; // fallthrough
    case SPR_MODE_BLACK: *dp &= ~sm; break;
    }
}

// special case: drawing from masked to opaque
static void spr_blit_rev_X(u32 *restrict DP, const u32 *restrict SP,
                           u32 pt, int l, int r,
                           u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
    u32 *restrict dp       = DP;
    const u32 *restrict sp = SP + sm;
    u32 p                  = 0;
    u32 m                  = 0;

    if (l == 0) { // same alignment, fast path
        p = brev32(*sp);
        m = brev32(*(sp + 1)) & ml;
        sp -= 2;
        for (int i = 0; i < dm; i++) {
            spr_blit_X(dp, p, m & pt, mode);
            p = brev32(*sp);
            m = brev32(*(sp + 1));
            sp -= 2;
            dp += 1;
        }
        spr_blit_X(dp, p, m & (pt & mr), mode);
        return;
    }

    if (0 < offs) { // first word
        p = brev32(bswap32(*sp)) << l;
        m = brev32(bswap32(*(sp + 1))) << l;
        if (0 < sm) {
            sp -= 2;
        }
    }

    p = bswap32(p | (brev32(bswap32(*sp)) >> r));
    m = bswap32(m | (brev32(bswap32(*(sp + 1))) >> r)) & ml;
    if (dm == 0) {
        spr_blit_X(dp, p, m & (pt & mr), mode);
        return; // only one word long
    }

    spr_blit_X(dp, p, m & pt, mode);
    dp += 1;

    for (int i = 1; i < dm; i++) { // middle words without first and last word
        p = brev32(bswap32(*sp)) << l;
        p = bswap32(p | (brev32(bswap32(*(sp - 2))) >> r));
        m = brev32(bswap32(*(sp + 1))) << l;
        m = bswap32(m | (brev32(bswap32(*(sp - 1))) >> r));
        spr_blit_X(dp, p, m & pt, mode);
        sp -= 2;
        dp += 1;
    }

    p = brev32(bswap32(*sp)) << l; // last word
    m = brev32(bswap32(*(sp + 1))) << l;
    if (SP < sp) {
        sp -= 2;
        p |= brev32(bswap32(*sp)) >> r;
        m |= brev32(bswap32(*(sp + 1))) >> r;
    }
    p = bswap32(p);
    m = bswap32(m);
    spr_blit_X(dp, p, m & (pt & mr), mode);
}

// special case: drawing from masked to opaque
static void spr_blit_fwd_X(u32 *restrict DP, const u32 *restrict SP, u32 pt, int l,
                           int r, u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
    u32 *restrict dp       = DP;
    const u32 *restrict sp = SP;
    u32 p                  = 0;
    u32 m                  = 0;

    if (l == 0) { // same alignment, fast path
        p = *sp;
        m = *(sp + 1) & ml;
        sp += 2;
        for (int i = 0; i < dm; i++) {
            spr_blit_X(dp, p, m & pt, mode);
            p = *sp;
            m = *(sp + 1);
            sp += 2;
            dp++;
        }
        spr_blit_X(dp, p, m & (pt & mr), mode);
        return;
    }

    if (0 < offs) { // first word
        p = bswap32(*sp) << l;
        m = bswap32(*(sp + 1)) << l;
        if (0 < sm) {
            sp += 2;
        }
    }

    p = bswap32(p | (bswap32(*sp) >> r));
    m = bswap32(m | (bswap32(*(sp + 1)) >> r)) & ml;
    if (dm == 0) {
        spr_blit_X(dp, p, m & (pt & mr), mode);
        return; // only one word long
    }

    spr_blit_X(dp, p, m & pt, mode);
    dp++;

    for (int i = 1; i < dm; i++) { // middle words without first and last word
        p = bswap32(*sp) << l;
        p = bswap32(p | (bswap32(*(sp + 2)) >> r));
        m = bswap32(*(sp + 1)) << l;
        m = bswap32(m | (bswap32(*(sp + 3)) >> r));
        spr_blit_X(dp, p, m & pt, mode);
        sp += 2;
        dp++;
    }

    p = bswap32(*sp) << l; // last word
    m = bswap32(*(sp + 1)) << l;
    if (sp < SP + sm) { // this is different for reversed blitting!
        sp += 2;
        p |= bswap32(*sp) >> r;
        m |= bswap32(*(sp + 1)) >> r;
    }
    p = bswap32(p);
    m = bswap32(m);
    spr_blit_X(dp, p, m & (pt & mr), mode);
}

void gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int m)
{
    // area bounds on canvas [x1/y1, x2/y2)
    int x1 = max_i(pos.x, ctx.clip_x1);               // inclusive
    int y1 = max_i(pos.y, ctx.clip_y1);               // inclusive
    int x2 = min_i(pos.x + src.r.w - 1, ctx.clip_x2); // inclusive
    int y2 = min_i(pos.y + src.r.h - 1, ctx.clip_y2); // inclusive
    if (x2 < x1) return;

    tex_s  dtex = ctx.dst;
    tex_s  stex = src.t;
    bool32 fx   = (flip & SPR_FLIP_X) != 0;                              // flipped x
    bool32 fy   = (flip & SPR_FLIP_Y) != 0;                              // flipped y
    int    sy   = fy ? -1 : +1;                                          // sign flip x
    int    sx   = fx ? -1 : +1;                                          // sign flip y
    int    nb   = (x2 + 1) - x1;                                         // number of bits in a row
    int    od   = x1 & 31;                                               // bitoffset in dst
    int    dm   = (od + nb - 1) >> 5;                                    // number of touched dst words -1
    u32    ml   = bswap32(0xFFFFFFFFU >> (31 & od));                     // mask to cut off boundary left
    u32    mr   = bswap32(0xFFFFFFFFU << (31 & (uint)(-od - nb)));       // mask to cut off boundary right
    int    u1   = src.r.x - sx * pos.x + (fx ? src.r.w - (x2 + 1) : x1); // first bit index in src row
    int    os   = (uint)(sx * u1 - fx * nb) & 31;                        // bitoffset in src
    int    da   = 1 + (dtex.fmt == TEX_FMT_MASK);                        // number of words to next logical pixel word in dst
    int    sa   = 1 + (stex.fmt == TEX_FMT_MASK);                        // number of words to next logical pixel word in src
    int    sm   = ((os + nb - 1) >> 5) * sa;                             // number of touched src words -1
    int    of   = os - od;                                               // alignment difference
    int    sl   = of & 31;                                               // word left shift amount
    int    sr   = 32 - sl;                                               // word rght shift amound

    u32 *dp = &dtex.px[((x1 >> 5) << (dtex.fmt == TEX_FMT_MASK)) + y1 * dtex.wword];                      // dst pixel words
    u32 *sp = &stex.px[(u1 >> 5) * sa + stex.wword * (src.r.y + sy * (y1 - pos.y) + fy * (src.r.h - 1))]; // src pixel words

    if (da == 1 && sa == 2) {
        for (int y = y1; y <= y2; y++, sp += stex.wword * sy, dp += dtex.wword) {
            u32 pt = ctx.pat.p[y & 7];
            if (pt == 0) continue;
            if (fx) {
                spr_blit_rev_X(dp, sp, pt, sl, sr, ml, mr, m, sm, dm, of);
            } else {
                spr_blit_fwd_X(dp, sp, pt, sl, sr, ml, mr, m, sm, dm, of);
            }
        }
    } else {
        for (int y = y1; y <= y2; y++, sp += stex.wword * sy, dp += dtex.wword) {
            u32 pt = ctx.pat.p[y & 7];
            if (pt == 0) continue;
            if (fx) {
                spr_blit_rev(dp, sp, pt, sa, da, sl, sr, ml, mr, m, sm, dm, of);
            } else {
                spr_blit_fwd(dp, sp, pt, sa, da, sl, sr, ml, mr, m, sm, dm, of);
            }
        }
    }
}
