// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/gfx.h"
#include "util/mathfunc.h"

// template for a rectangular sprite blitting function

// user input for inclusion:
// ---
// #define SPRBLIT_FUNCNAME <- name of function
// #define SPRBLIT_FLIPPEDX <- 0/1: draw sprites mirrored x
// #define SPRBLIT_SRC_MASK <- 0/1: src texture has transparency
// #define SPRBLIT_DST_MASK <- 0/1: dst texture has transparency
// #define SPRBLIT_COPYMODE <- 0/1: fast version applying only the copy mode

static_assert(SPRBLIT_FLIPPEDX <= 1, "SPRBLIT_FLIPPEDX value");
static_assert(SPRBLIT_SRC_MASK <= 1, "SPRBLIT_SRC_MASK value");
static_assert(SPRBLIT_DST_MASK <= 1, "SPRBLIT_DST_MASK value");

// define reading function of source words
#if SPRBLIT_FLIPPEDX
#define SPRBLIT_GET_WORD(ADDR) brev32(bswap32(ADDR)) // mirror bit order
#else
#define SPRBLIT_GET_WORD(ADDR) bswap32(ADDR)
#endif

// choose a pixel blit function depending on if destination contains transparency info
// ---
// DP: pointer to destination pixel word (black/white)
// DM: pointer to destination mask word (opaque/transparent)
// SP: assembled pixel word from source to blit to destination (black/white)
// SM: assembled mask word from source to blit to destination (opaque/transparent) - boundary clipping already applied
// PT: drawing pattern bits in screen space
// MD: blitting logic enum
#if SPRBLIT_DST_MASK
#if SPRBLIT_COPYMODE // force simplest blitting mode
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_pm_copy(DP, DM, SP, SM, PT)
#else
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_pm(DP, DM, SP, SM, PT, MD)
#endif
#else
#if SPRBLIT_COPYMODE // force simplest blitting mode
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_p_copy(DP, SP, SM, PT)
#else
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_p(DP, SP, SM, PT, MD)
#endif
#endif
ATTRIBUTE_SECTION(".text.spr")
void SPRBLIT_FUNCNAME(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode)
{
    i32 x1 = max_i32(ctx.clip_x1, pos.x);
    i32 y1 = max_i32(ctx.clip_y1, pos.y);
    i32 x2 = min_i32(ctx.clip_x2, pos.x + src.w - 1);
    i32 y2 = min_i32(ctx.clip_y2, pos.y + src.h - 1);
    if (x2 < x1 || y2 < y1) return; // not visible

#if SPRBLIT_FLIPPEDX
    i32 a_x = src.x + pos.x + src.w - 1; // cached offset value
    i32 shl = 31 & (u32)(-a_x - 1);      // left shift value for word assembly
#else
    i32 a_x = src.x - pos.x;   // cached offset value
    i32 shl = 31 & (u32)(a_x); // left shift value for word assembly
#endif
    i32   shr = 32 - shl; // right shift value for word assembly
    u32   c_l = bswap32(0xFFFFFFFF >> (31 & x1));
    u32   c_r = bswap32(0xFFFFFFFF << (31 - (x2 & 31))); // clipping mask right, << (31 & (u32)(-x2 - 1))
    i32   s_y = flip & SPR_FLIP_Y ? -1 : +1;
    i32   a_y = 0 < s_y ? src.y - pos.y : src.y + pos.y + src.h - 1;
    i32   dw1 = x1 >> 5;
    i32   dw2 = x2 >> 5;
    tex_s t_d = ctx.dst;
    tex_s t_s = src.t;
    assert(t_d.px);
    assert(t_s.px);

    // for every affected row of target texture
    for (i32 y_d = y1; y_d <= y2; y_d++) {
        i32 y_s = s_y * y_d + a_y; // row index in source texture
        u32 pat = ctx.pat.p[y_d & 7];
        u32 c_m = c_l; // clipping word (first dst word is left clipped)

        // for every affected word in this row of the target texture
        // set clipping word to "non clipping" after the first word which has to be left clipped
        for (i32 d_w = dw1; d_w <= dw2; d_w++, c_m = 0xFFFFFFFF) {

            // calculate the 2 source words to pull pixel data from
#if SPRBLIT_FLIPPEDX
            i32 sx1 = a_x - ((d_w << 5) + 0);  // right most pixel needed from this row of source texture
            i32 sx2 = a_x - ((d_w << 5) + 31); // left most pixel needed from this row of source texture
            sx1     = min_i32(sx1, t_s.w - 1); // clamp to not read out of bounds
            sx2     = max_i32(sx2, 0);         // clamp to not read out of bounds
#else
            i32 sx1 = a_x + ((d_w << 5) + 0);  // left most pixel needed from this row of source texture
            i32 sx2 = a_x + ((d_w << 5) + 31); // right most pixel needed from this row of source texture
            sx1     = max_i32(sx1, 0);         // clamp to not read out of bounds
            sx2     = min_i32(sx2, t_s.w - 1); // clamp to not read out of bounds
#endif
            assert(0 <= sx2 && sx2 < t_s.w);
            assert(0 <= sx1 && sx1 < t_s.w);
            if (d_w == dw2) {
                c_m &= c_r; // apply right clipping mask if end of the row
            }

            // assemble pixel color word to blit out of 2 source words
            u32 sp1 = SPRBLIT_GET_WORD(t_s.px[((sx1 >> 5) << SPRBLIT_SRC_MASK) + 0 + y_s * t_s.wword]);
            u32 sp2 = SPRBLIT_GET_WORD(t_s.px[((sx2 >> 5) << SPRBLIT_SRC_MASK) + 0 + y_s * t_s.wword]);
            u32 spp = bswap32((u32)((u64)sp1 << shl) | (u32)((u64)sp2 >> shr));
#if SPRBLIT_SRC_MASK
            // assemble transparency word to blit out of 2 source words
            u32 sm1 = SPRBLIT_GET_WORD(t_s.px[((sx1 >> 5) << 1) + 1 + y_s * t_s.wword]);
            u32 sm2 = SPRBLIT_GET_WORD(t_s.px[((sx2 >> 5) << 1) + 1 + y_s * t_s.wword]);
            u32 smm = c_m & bswap32((u32)((u64)sm1 << shl) | (u32)((u64)sm2 >> shr));
#else
            // opaque sprite, just use clipping bits for transparency
            u32 smm = c_m;
#endif
            u32 *dpp = &t_d.px[(d_w << SPRBLIT_DST_MASK) + 0 + y_d * t_d.wword];
#if SPRBLIT_DST_MASK
            u32 *dmm = &t_d.px[(d_w << SPRBLIT_DST_MASK) + 1 + y_d * t_d.wword];
            SPRBLIT_BLITFUNC(dpp, dmm, spp, smm, pat, mode);
#else
            SPRBLIT_BLITFUNC(dpp, 0, spp, smm, pat, mode);
#endif
        }
    }
}
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_GET_WORD
#undef SPRBLIT_BLITFUNC
#undef SPRBLIT_COPYMODE
