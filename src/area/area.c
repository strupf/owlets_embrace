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

static u32 g_areafx[NUM_AREA_ID] = {
    0,                     // none
    0,                     // white
    0,                     // black
    AFX_CLOUDS | AFX_WIND, // mountain
    AFX_CLOUDS | AFX_RAIN, // mountain rainy
    0,                     // cave
    0,                     // cave deep
    AFX_WIND,              // forest
};

static v2_i32 area_parallax(v2_i32 cam, i32 x_q8, i32 y_q8, i32 ax, i32 ay)
{
    v2_i32 p = {((cam.x * x_q8) >> 8) & ~ax,
                ((cam.y * y_q8) >> 8) & ~ay};
    return p;
}

void area_setup(g_s *g, area_s *a, i32 ID)
{
    mclr(a, sizeof(area_s));
    a->ID = ID;
    pltf_log("area ID: %i\n", ID);

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
    if ((g_areafx[a->ID] & AFX_PARTICLES_CALM)) {
        areafx_particles_calm_setup(g, &a->fx.particles_calm);
    }
}

void area_update(g_s *g, area_s *a)
{
    if ((g_areafx[a->ID] & AFX_RAIN)) {

        areafx_rain_update(g, &a->fx.rain);
    }
    if ((g_areafx[a->ID] & AFX_WIND)) {
        areafx_wind_update(g, &a->fx.wind);
    }

    if (g->tick & 1) {
        if ((g_areafx[a->ID] & AFX_HEAT)) {
            areafx_heat_update(g, &a->fx.heat);
        }
        if ((g_areafx[a->ID] & AFX_CLOUDS)) {
            areafx_clouds_update(g, &a->fx.clouds);
        }
        if ((g_areafx[a->ID] & AFX_PARTICLES_CALM)) {
            areafx_particles_calm_update(g, &a->fx.particles_calm);
        }
    } else {
    }
}

void area_draw_bg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);

    switch (a->ID) {
    case AREA_ID_CAVE:
    case AREA_ID_CAVE_DEEP:
    case AREA_ID_BLACK:
        gfx_fill_rows(tdisplay, gfx_pattern_black(), 0, ctx.clip_y2);
        break;
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY:

        gfx_fill_rows(tdisplay, gfx_pattern_2x2(B2(11), B2(11)), 0, ctx.clip_y2);
        break;
    case AREA_ID_FOREST:
    case AREA_ID_WHITE:
    default:
        gfx_fill_rows(tdisplay, gfx_pattern_white(), 0, ctx.clip_y2);
        break;
    }

    if (g_areafx[a->ID] & AFX_CLOUDS) {
        areafx_clouds_draw(g, &a->fx.clouds, cam);
    }

#if defined(PLTF_PD_HW) || 0
#define ALIGN_PATTERN 3
#else
#define ALIGN_PATTERN 1
#endif

    switch (a->ID) {
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 0, 1024, 256);
        texrec_s tr_near  = asset_texrec(TEXID_BG_MOUNTAINS, 0, 256, 1024, 256);
        v2_i32   pos_far  = area_parallax(cam, 50, 0, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 100, 0, 1, 1);
        pos_far.y -= 20;
        pos_far.y += 20;
        pos_near.y += 20;

        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    case AREA_ID_CAVE: {
        texrec_s tr_far  = asset_texrec(TEXID_BG_CAVE, 0, 0, 1024, 256);
        texrec_s tr_mid  = asset_texrec(TEXID_BG_CAVE, 0, 256, 1024, 256);
        v2_i32   pos_far = area_parallax(cam, 25, 0, 1, 1);
        v2_i32   pos_mid = area_parallax(cam, 50, 0, 1, 1);
        pos_far.y += 30;
        pos_mid.y += 30;
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        break;
    }
    case AREA_ID_CAVE_DEEP: {
        texrec_s tr_far  = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 1024, 448, 512);
        texrec_s tr_mid  = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 512, 448, 512);
        texrec_s tr_near = asset_texrec(TEXID_BG_CAVE_DEEP, 0, 0, 448, 512);

        v2_i32 pos_far  = area_parallax(cam, 32, 0, ALIGN_PATTERN, ALIGN_PATTERN);
        v2_i32 pos_mid  = area_parallax(cam, 64, 0, ALIGN_PATTERN, ALIGN_PATTERN);
        v2_i32 pos_near = area_parallax(cam, 128, 0, 1, 1);
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);

        v2_i32 golempos = pos_mid;
        golempos.x += 250;
        golempos.y += 80;
        // golempos.x          = 100;
        // golempos.y          = 100;
        i32 timet        = pltf_cur_tick();
        i32 golemframe_1 = ((timet - 1) >> 5) % 4;
        i32 golemframe_2 = ((timet + 0) >> 5) % 4;
        golemframe_2     = max_i32(golemframe_2 - 1, 0);
        golemframe_1     = max_i32(golemframe_1 - 1, 0);
        if (golemframe_1 == 1 && golemframe_2 == 2) {
            // cam_screenshake_xy(&g->cam, 60, 2, 4);
        }

        texrec_s tigolem = asset_texrec(TEXID_BGOLEM, golemframe_2 * 128, 0, 128, 96);
        gfx_spr(ctx, tigolem, golempos, 0, 0);
        break;
    }
    case AREA_ID_FOREST: {
        texrec_s tr_far   = asset_texrec(TEXID_BG_FOREST, 0, 256 + 0, 416, 256);
        texrec_s tr_mid   = asset_texrec(TEXID_BG_FOREST, 0, 256 + 512, 416, 256);
        texrec_s tr_near  = asset_texrec(TEXID_BG_FOREST, 0, 256 + 1024, 416, 256);
        v2_i32   pos_far  = area_parallax(cam, 25, 0, 1, 1);
        v2_i32   pos_mid  = area_parallax(cam, 50, 0, 1, 1);
        v2_i32   pos_near = area_parallax(cam, 75, 0, 1, 1);
        gfx_spr_tileds(ctx, tr_far, pos_far, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_mid, pos_mid, 0, 0, 1, 0);
        gfx_spr_tileds(ctx, tr_near, pos_near, 0, 0, 1, 0);
        break;
    }
    }
}

void area_draw_mg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    if (g_areafx[a->ID] & AFX_RAIN) {
        areafx_rain_draw(g, &a->fx.rain, cam);
    }
    if (g_areafx[a->ID] & AFX_WIND) {
        // areafx_wind_draw(g, &a->fx.wind, cam);
    }
}

void area_draw_fg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s           tdisplay = asset_tex(0);
    const gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);

    switch (a->ID) {
    case 1:
    default: {
        // set new origin
        v2_i32   cmax    = cam_offset_max(g, &g->cam);
        v2_i32   cc      = {cam.x, cam.y + cmax.y};
        texrec_s tr_far  = asset_texrec(TEXID_MISCOBJ, 448, 0, 32, 80);
        v2_i32   pos_far = area_parallax(cc, 300, 300, 1, 1);
        pos_far.x += 400;
        pos_far.y += PLTF_DISPLAY_H - tr_far.h;
        // gfx_spr_tiled(ctx, tr_far, pos_far, 0, 0, 100, 0);
        break;
    }
    }
    if (g_areafx[a->ID] & AFX_RAIN) {
        areafx_rain_draw_lightning(g, &a->fx.rain, cam);
    }
    if (g_areafx[a->ID] & AFX_HEAT) {
        areafx_heat_draw(g, &a->fx.heat, cam);
    }
    if ((g_areafx[a->ID] & AFX_PARTICLES_CALM)) {
        areafx_particles_calm_draw(g, &a->fx.particles_calm, cam);
    }
}