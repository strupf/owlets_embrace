// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "area.h"
#include "game.h"
#include "render.h"

enum {
    AREA_ID_NONE,
    AREA_ID_WHITE,
    AREA_ID_BLACK,
    AREA_ID_MOUNTAIN,
    AREA_ID_MOUNTAIN_RAINY,
    AREA_ID_CAVE,
    AREA_ID_CAVE_DEEP,
    AREA_ID_FOREST,
    //
    NUM_AREA_ID
};

static v2_i32 area_parallax(v2_i32 cam, i32 x_q8, i32 y_q8, i32 ax, i32 ay)
{
    v2_i32 p = {((cam.x * x_q8) >> 8) & ~ax,
                ((cam.y * y_q8) >> 8) & ~ay};
    return p;
}

void area_setup(g_s *g, area_s *a, i32 ID, i32 areafx)
{
    mclr(a, sizeof(area_s));
    a->ID = ID;

    switch (areafx) {
    case AFX_NONE: {
        a->fx_type = 0;
        break;
    }
    case AFX_SNOW: {
        a->fx_type = AFX_SNOW;
        areafx_snow_setup(g, &a->fx.snow);
        break;
    }
    case AFX_HEAT: {
        a->fx_type = AFX_HEAT;
        areafx_heat_setup(g, &a->fx.heat);
        break;
    }
    }
}

void area_update(g_s *g, area_s *a)
{
    switch (a->fx_type) {
    case AFX_RAIN:
        areafx_rain_update(g, &a->fx.rain);
        break;
    case AFX_HEAT:
        if (g->tick & 1) break;
        areafx_heat_update(g, &a->fx.heat);
        break;
    }
}

void area_draw_bg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);

    gfx_fill_rows(tdisplay, gfx_pattern_white(), 0, ctx.clip_y2);
    // return; // <==== RETURN

    switch (a->ID) {
    case AREA_ID_CAVE:
    case AREA_ID_CAVE_DEEP:
    case AREA_ID_BLACK:
        gfx_fill_rows(tdisplay, gfx_pattern_black(), 0, ctx.clip_y2);
        break;
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY:
        gfx_fill_rows(tdisplay, gfx_pattern_2x2(B2(10), B2(11)), 0, ctx.clip_y2);
        break;
    case AREA_ID_FOREST:
    case AREA_ID_WHITE:
    default:
        gfx_fill_rows(tdisplay, gfx_pattern_white(), 0, ctx.clip_y2);
        break;
    }

#if defined(PLTF_PD_HW) || 0
#define ALIGN_PATTERN 3
#else
#define ALIGN_PATTERN 1
#endif

    switch (a->ID) {
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 256);
        texrec_s tr_near  = asset_texrec(TEXID_BG_PARALLAX, 0, 256, 1024, 256);
        v2_i32   pos_far  = area_parallax(cam, 50, 0, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 100, 0, 1, 1);
        pos_near.y += 20;

        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    case AREA_ID_CAVE:
    case AREA_ID_CAVE_DEEP: {
        texrec_s tr_far  = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 448, 512);
        texrec_s tr_mid  = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 448, 512);
        texrec_s tr_near = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 448, 512);

        v2_i32 pos_far  = area_parallax(cam, 32, 0, ALIGN_PATTERN, ALIGN_PATTERN);
        v2_i32 pos_mid  = area_parallax(cam, 64, 0, ALIGN_PATTERN, ALIGN_PATTERN);
        v2_i32 pos_near = area_parallax(cam, 128, 0, 1, 1);
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    case AREA_ID_FOREST: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_PARALLAX, 0, 256 + 0, 512, 256);
        texrec_s tr_mid   = asset_texrec(TEXID_BG_PARALLAX, 0, 256 + 512, 512, 256);
        texrec_s tr_near  = asset_texrec(TEXID_BG_PARALLAX, 0, 256 + 1024, 512, 256);
        v2_i32   pos_far  = area_parallax(cam, 25, -5, 1, 1);
        v2_i32   pos_mid  = area_parallax(cam, 50, -10, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 75, -15, 1, 1);
        pos_far.y -= 24;
        pos_mid.y -= 10;
        pos_near.y -= 0;
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    }
}