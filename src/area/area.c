// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "area.h"
#include "game.h"
#include "render.h"

static i32 coord_parallax(i32 p, i32 v_q8, i32 a)
{
    return (((((p * v_q8) >> 8) & ~a)));
}

static v2_i32 area_parallax(v2_i32 cam, i32 x_q8, i32 y_q8, i32 ax, i32 ay)
{
    v2_i32 p = {-(((-cam.x * x_q8) >> 8) & ~ax),
                -(((-cam.y * y_q8) >> 8) & ~ay)};
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

#include "app.h"
void area_draw_bg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);

    // return; // <==== RETURN

    i32 alx = APP->opt ? 3 : 1;
    i32 aly = APP->opt ? 3 : 1;
    if (APP->opt == 1) {
        cam_al = cam;
    }
    alx = 3;
    aly = 3;
    //  cam_al  = cam;
    // alx     = 3;
    // aly     = 3;
    switch (a->ID) {
    case 9: {
        texrec_s tr_bg = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 512, 256);
        // gfx_fill_rows(tdisplay, gfx_pattern_bayer_4x4(1), 0, ctx.clip_y2);

        texrec_s tr_far  = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_mid  = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);
        v2_i32   pos_far = {coord_parallax(cam_al.x + g->bg_offx, 192, alx),
                            coord_parallax(cam_al.y + g->bg_offy, 192, aly)};
        v2_i32   pos_mid = {coord_parallax(cam_al.x + g->bg_offx, 128, alx),
                            coord_parallax(cam_al.y + g->bg_offy, 128, aly)};

#if 0 
        static i32 p1;
        static i32 p2;
        pltf_log("%i | %i\n", abs_i32(p1 - pos_far.x), abs_i32(p2 - cam.x));
        p1 = pos_far.x;
        p2 = cam.x;
#endif
        gfx_spr_copy(ctx, tr_bg, (v2_i32){0}, 0);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        break;
    }
    case AREA_ID_MOUNTAIN:
    case AREA_ID_MOUNTAIN_RAINY: {

        break;
    }
    case AREA_ID_CAVE:
    case AREA_ID_CAVE_DEEP: {

        break;
    }
    case AREA_ID_FOREST: {
#define USE_DARK_FOREST 0

#if USE_DARK_FOREST

        gfx_fill_rows(tdisplay, gfx_pattern_bayer_4x4(15), 0, ctx.clip_y2);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 1024, 512);
#else
        texrec_s tr_bg = asset_texrec(TEXID_BG_PARALLAX, 0, 1536, 512, 256);
        gfx_spr_copy(ctx, tr_bg, (v2_i32){0}, 0);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);
#endif

        v2_i32 pos_far = {coord_parallax(cam_al.x + g->bg_offx, 192, alx),
                          coord_parallax(cam_al.y + g->bg_offy, 192, aly)};
        v2_i32 pos_mid = {coord_parallax(cam_al.x + g->bg_offx, 128, alx),
                          coord_parallax(cam_al.y + g->bg_offy, 128, aly)};
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        break;
    }
    case AREA_ID_SAVE: {
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 400, 230);
        gfx_spr(ctx, tr_far, (v2_i32){0}, 0, 0);

        break;
    }
    }
}