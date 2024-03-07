// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "area.h"
#include "game.h"
#include "render/render.h"

enum {
    AREA_ID_NONE,
    AREA_ID_WHITE,
    AREA_ID_BLACK,
    AREA_ID_MOUNTAIN,
    AREA_ID_MOUNTAIN_RAINY,
    AREA_ID_CAVE,
    AREA_ID_FOREST,
    //
    NUM_AREA_ID
};

static flags32 g_areafx[NUM_AREA_ID] = {
    0,                     // none
    0,                     // white
    0,                     // black
    AFX_CLOUDS,            // mountain
    AFX_CLOUDS | AFX_RAIN, // mountain rainy
    0,                     // cave
    AFX_WIND,              // forest
};

static v2_i32 area_parallax(v2_i32 cam, int x_q8, int y_q8, int ax, int ay)
{
    v2_i32 p = {((cam.x * x_q8) >> 8) & ~ax,
                ((cam.y * y_q8) >> 8) & ~ay};
    return p;
}

void area_setup(game_s *g, area_s *a, int ID)
{
    *a    = (area_s){0};
    a->ID = ID;
    a->ID = AREA_ID_CAVE;

    if (g_areafx[a->ID] & AFX_RAIN) {
        areafx_rain_setup(g, &a->fx.rain);
    }
    if ((g_areafx[a->ID] & AFX_WIND)) {
        areafx_wind_setup(g, &a->fx.wind);
    }
    if ((g_areafx[a->ID] & AFX_HEAT)) {
        areafx_heat_setup(g, &a->fx.heat);
    }
    if ((g_areafx[a->ID] & AFX_CLOUDS)) {
        areafx_clouds_setup(g, &a->fx.clouds);
    }
    if ((g_areafx[a->ID] & AFX_LEAVES)) {
        areafx_leaves_setup(g, &a->fx.leaves);
    }
}

void area_update(game_s *g, area_s *a)
{
    a->tick++;

    if ((g_areafx[a->ID] & AFX_RAIN)) {

        areafx_rain_update(g, &a->fx.rain);
    }
    if ((g_areafx[a->ID] & AFX_WIND)) {
        areafx_wind_update(g, &a->fx.wind);
    }

    if (a->tick & 1) {
        if ((g_areafx[a->ID] & AFX_HEAT)) {
            areafx_heat_update(g, &a->fx.heat);
        }
        if ((g_areafx[a->ID] & AFX_CLOUDS)) {
            areafx_clouds_update(g, &a->fx.clouds);
        }
    } else {
    }
}

void area_draw_bg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s tdisplay = asset_tex(0);
    int   clip_y2  = min_i(g->ocean.y_max, tdisplay.h);

    const gfx_ctx_s ctx = gfx_ctx_clip_bot(gfx_ctx_default(tdisplay), clip_y2);

    switch (a->ID) {
    case AREA_ID_CAVE:
    case AREA_ID_BLACK: {
        gfx_pattern_s pat = gfx_pattern_bayer_4x4(0);
        gfx_fill_rows(tdisplay, pat, 0, clip_y2);
        break;
    }
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY:
    case AREA_ID_WHITE:
    default: {
        gfx_pattern_s pat = gfx_pattern_bayer_4x4(GFX_PATTERN_MAX);
        gfx_fill_rows(tdisplay, pat, 0, clip_y2);
        break;
    }
    }

    if (g_areafx[a->ID] & AFX_CLOUDS) {
        areafx_clouds_draw(g, &a->fx.clouds, cam);
    }

    switch (a->ID) {
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 0, 1024, 256);
        texrec_s tr_near  = asset_texrec(TEXID_BG_MOUNTAINS, 0, 256, 1024, 256);
        v2_i32   pos_far  = area_parallax(cam, 50, 0, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 100, 0, 1, 1);
        pos_far.y -= 20;
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    case AREA_ID_CAVE: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 1024, 1024, 512);
        texrec_s tr_mid   = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 512, 1024, 512);
        texrec_s tr_near  = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 0, 1024, 512);
        v2_i32   pos_far  = area_parallax(cam, 25, 0, 3, 3);
        v2_i32   pos_mid  = area_parallax(cam, 75, 0, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 100, 0, 1, 1);
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    }
}

void area_draw_mg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    if (g_areafx[a->ID] & AFX_RAIN) {
        areafx_rain_draw(g, &a->fx.rain, cam);
    }
}

void area_draw_fg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s           tdisplay = asset_tex(0);
    const gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);

    switch (a->ID) {
    default: {
        // set new origin
        v2_i32   cmax    = cam_offset_max(g, &g->cam);
        v2_i32   cc      = {cam.x, cam.y + cmax.y};
        texrec_s tr_far  = asset_texrec(TEXID_MISCOBJ, 448, 0, 32, 80);
        v2_i32   pos_far = area_parallax(cc, 300, 300, 1, 1);
        pos_far.x += 400;
        pos_far.y += SYS_DISPLAY_H - tr_far.r.h;
        // gfx_spr_tiled(ctx, tr_far, pos_far, 0, 0, 100, 0);
        break;
    }
    }

    if (g_areafx[a->ID] & AFX_HEAT) {
        areafx_heat_draw(g, &a->fx.heat, cam);
    }
}
