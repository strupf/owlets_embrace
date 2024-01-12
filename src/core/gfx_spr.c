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

// used for simple sprites
typedef struct {
    int           dmax; // count of dst words -1
    u32           ml;   // boundary mask left
    u32           mr;   // boundary mask right
    int           mode; // drawing mode
    int           doff; // bitoffset of first dst bit
    u32          *dp;   // pixel
    u32          *dm;   // mask
    u32          *ds;   // stencil
    int           y;
    gfx_pattern_s pat;
    int           shift; // amount of bitshift needed to align
    int           smax;  // count of src words -1
    int           soff;  // bitoffset of first src bit
    u32          *sp;
    u32          *sm;
} spr_blit_s;

static spr_blit_s spr_blit_gen(gfx_ctx_s ctx, int y, int x1, int x2, int mode)
{
    int nbit = (x2 + 1) - x1; // number of bits in a row to blit
    int dsti = (x1 >> 5) + y * ctx.dst.wword;

    spr_blit_s info = {0};
    info.y          = y;
    info.doff       = x1 & 31;
    info.dmax       = (info.doff + nbit - 1) >> 5;                        // number of touched dst words -1
    info.mode       = mode;                                               // sprite masking mode
    info.ml         = bswap32(0xFFFFFFFFU >> (31 & info.doff));           // mask to cut off boundary left
    info.mr         = bswap32(0xFFFFFFFFU << (31 & (-info.doff - nbit))); // mask to cut off boundary right
    info.dp         = &((u32 *)ctx.dst.px)[dsti];
    info.dm         = ctx.dst.mk ? &((u32 *)ctx.dst.mk)[dsti] : NULL;
    info.ds         = ctx.st ? &((u32 *)ctx.st)[dsti] : NULL;
    info.pat        = ctx.pat;
    return info;
}

static void spr_blit(u32 *restrict dp,
                     u32 *restrict dm,
                     u32 *restrict ds,
                     u32 sp, u32 sm, int mode)
{
    if (ds) {
        sm &= ~*ds;
    }

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
    if (ds) *ds |= sm;
}

static void spr_blit_row_rev(spr_blit_s info)
{
#define SPR_ADV_D \
    dp++;         \
    if (dm) dm++; \
    if (ds) ds++;

    u32 pt                 = info.pat.p[info.y & 7];
    u32 *restrict dp       = (u32 *restrict)(info.dp);
    u32 *restrict dm       = (u32 *restrict)(info.dm);
    u32 *restrict ds       = (u32 *restrict)(info.ds);
    const u32 *restrict sp = (u32 *restrict)(info.sp + info.smax);
    const u32 *restrict sm = (u32 *restrict)(info.sm ? info.sm + info.smax : NULL);
    int a                  = info.shift;
    int b                  = 32 - a;

    if (a == 0) { // same alignment, fast path
        u32 p = brev32(*sp--);
        u32 m = sm ? brev32(*sm--) & info.ml : info.ml;
        for (int i = 0; i < info.dmax; i++) {
            spr_blit(dp, dm, ds, p, m & pt, info.mode);
            p = brev32(*sp--);
            m = sm ? brev32(*sm--) : 0xFFFFFFFFU;
            SPR_ADV_D
        }
        spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
        return;
    }

    { // first word
        u32 p = 0, m = 0;
        if (info.soff > info.doff) {
            p = brev32(bswap32(*sp)) << a;
            m = sm ? brev32(bswap32(*sm)) << a : 0xFFFFFFFFU;
            if (info.smax > 0) {
                sp--;
                if (sm) sm--;
            }
        }

        p = bswap32(p | (brev32(bswap32(*sp)) >> b));
        m = sm ? bswap32(m | (brev32(bswap32(*sm)) >> b)) & info.ml : info.ml;

        if (info.dmax == 0) {
            spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
            return; // only one word long
        }

        spr_blit(dp, dm, ds, p, m & pt, info.mode);
        SPR_ADV_D
    }

    // middle words without first and last word
    for (int i = 1; i < info.dmax; i++) {
        u32 p = bswap32((brev32(bswap32(*sp)) << a) | (brev32(bswap32(*(sp - 1))) >> b));
        u32 m = sm ? bswap32((brev32(bswap32(*sm)) << a) | (brev32(bswap32(*(sm - 1))) >> b)) : 0xFFFFFFFFU;
        spr_blit(dp, dm, ds, p, m & pt, info.mode);
        sp--;
        if (sm) sm--;
        SPR_ADV_D
    }

    { // last word
        u32 p = brev32(bswap32(*sp)) << a;
        u32 m = sm ? brev32(bswap32(*sm)) << a : 0xFFFFFFFFU;
        if (sp > info.sp) {
            p |= brev32(bswap32(*--sp)) >> b;
            if (sm) m |= brev32(bswap32(*--sm)) >> b;
        }
        p = bswap32(p);
        m = bswap32(m);
        spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
    }
#undef SPR_ADV_D
}

static void spr_blit_row(spr_blit_s info)
{
#define SPR_ADV_D \
    dp++;         \
    if (dm) dm++; \
    if (ds) ds++;

    u32 pt                 = info.pat.p[info.y & 7];
    u32 *restrict dp       = (u32 *restrict)(info.dp);
    u32 *restrict dm       = (u32 *restrict)(info.dm);
    u32 *restrict ds       = (u32 *restrict)(info.ds);
    const u32 *restrict sp = (u32 *restrict)(info.sp);
    const u32 *restrict sm = (u32 *restrict)(info.sm);
    int a                  = info.shift;
    int b                  = 32 - a;

    if (a == 0) { // same alignment, fast path
        u32 p = *sp++;
        u32 m = sm ? *sm++ & info.ml : info.ml;
        for (int i = 0; i < info.dmax; i++) {
            spr_blit(dp, dm, ds, p, m & pt, info.mode);
            p = *sp++;
            m = sm ? *sm++ : 0xFFFFFFFFU;
            SPR_ADV_D;
        }
        spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
        return;
    }

    { // first word
        u32 p = 0, m = 0;
        if (info.soff > info.doff) {
            p = bswap32(*sp) << a;
            m = sm ? bswap32(*sm) << a : 0xFFFFFFFFU;
            if (info.smax > 0) {
                sp++;
                if (sm) sm++;
            }
        }

        p = bswap32(p | (bswap32(*sp) >> b));
        m = sm ? bswap32(m | (bswap32(*sm) >> b)) & info.ml : info.ml;
        if (info.dmax == 0) {
            spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
            return; // only one word long
        }

        spr_blit(dp, dm, ds, p, m & pt, info.mode);
        SPR_ADV_D;
    }

    // middle words without first and last word
    for (int i = 1; i < info.dmax; i++) {
        u32 p = bswap32((bswap32(*sp) << a) | (bswap32(*(sp + 1)) >> b));
        u32 m = sm ? bswap32((bswap32(*sm) << a) | (bswap32(*(sm + 1)) >> b)) & pt : pt;
        spr_blit(dp, dm, ds, p, m, info.mode);
        sp++;
        if (sm) sm++;
        SPR_ADV_D;
    }

    { // last word
        u32 p = bswap32(*sp) << a;
        u32 m = sm ? bswap32(*sm) << a : 0xFFFFFFFFU;
        if (sp < info.sp + info.smax) { // this is different for reversed blitting!
            p |= bswap32(*++sp) >> b;
            if (sm) m |= bswap32(*++sm) >> b;
        }
        p = bswap32(p);
        m = bswap32(m);
        spr_blit(dp, dm, ds, p, m & (pt & info.mr), info.mode);
    }
#undef SPR_ADV_D
}

void gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode)
{
    tex_s dtex = ctx.dst;                                 // area bounds on canvas [x1/y1, x2/y2)
    int   x1   = max_i(pos.x, ctx.clip_x1);               // inclusive
    int   y1   = max_i(pos.y, ctx.clip_y1);               // inclusive
    int   x2   = min_i(pos.x + src.r.w - 1, ctx.clip_x2); // inclusive
    int   y2   = min_i(pos.y + src.r.h - 1, ctx.clip_y2); // inclusive
    if (x1 > x2) return;
    tex_s stex = src.t;

    spr_blit_s info = spr_blit_gen(ctx, y1, x1, x2, mode);
    int        nbit = (x2 + 1) - x1; // number of bits in a row to blit
    int        u1;                   // first bit index in src row
    if (flip & SPR_FLIP_X) {         // flipping needs special care
        u1        = src.r.x + pos.x + src.r.w - (x2 + 1);
        info.soff = (-u1 - nbit) & 31;
    } else {
        u1        = src.r.x - pos.x + x1; // first bit index in src row
        info.soff = u1 & 31;
    }

    int srci; // first word index in src
    if (flip & SPR_FLIP_Y) {
        srci = (u1 >> 5) + stex.wword * (src.r.y + pos.y - y1 + src.r.h - 1);
    } else {
        srci = (u1 >> 5) + stex.wword * (src.r.y - pos.y + y1);
    }

    info.sp    = &((u32 *restrict)stex.px)[srci];                  // src black/white
    info.sm    = stex.mk ? &((u32 *restrict)stex.mk)[srci] : NULL; // src opaque/transparent
    info.shift = (info.soff - info.doff) & 31;                     // word shift amount
    info.smax  = (info.soff + nbit - 1) >> 5;                      // number of touched src words -1

    for (info.y = y1; info.y <= y2; info.y++) {
        if (flip & SPR_FLIP_X) {
            spr_blit_row_rev(info);
        } else {
            spr_blit_row(info);
        }

        if (flip & SPR_FLIP_Y) {
            info.sp -= stex.wword;
            if (info.sm) info.sm -= stex.wword;
        } else {
            info.sp += stex.wword;
            if (info.sm) info.sm += stex.wword;
        }

        info.dp += dtex.wword;
        if (info.dm) info.dm += dtex.wword;
        if (info.ds) info.ds += dtex.wword;
    }
}
