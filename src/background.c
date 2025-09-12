// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "game.h"
#include "render.h"

void background_perf_prepare(g_s *g);

void background_init_and_load_from_wad(g_s *g, i32 ID, void *f)
{
    tex_s *tbg = &APP.assets.tex[TEXID_BG_PARALLAX];
    switch (ID) {
    default: break;
    case BACKGROUND_ID_CAVE:
        tex_from_wad(f, 0, "T_BG_CAVE", game_per_room_allocator(g), tbg);
        break;
    case BACKGROUND_ID_FOREST_DARK:
    case BACKGROUND_ID_FOREST_BRIGHT:
        tex_from_wad(f, 0, "T_BG_FOREST", game_per_room_allocator(g), tbg);
        break;
    case BACKGROUND_ID_SNOW:
        tex_from_wad(f, 0, "T_BG_SNOW", game_per_room_allocator(g), tbg);
        break;
    case BACKGROUND_ID_WATERFALL:
        tex_from_wad(f, 0, "T_BG_WATERFALL", game_per_room_allocator(g), tbg);
        break;
    case BACKGROUND_ID_VERTICAL:
        tex_from_wad(f, 0, "T_BG_VERTICAL", game_per_room_allocator(g), tbg);
        break;
    case BACKGROUND_ID_MOUNTAIN:
        tex_from_wad(f, 0, "T_BG_MOUNTAIN", game_per_room_allocator(g), tbg);
        break;
    }
    background_perf_prepare(g);
}

void background_perf_prepare(g_s *g)
{
    tex_s t = asset_tex(TEXID_BG_PARALLAX_PERF);
    tex_clr(t, GFX_COL_WHITE);

    switch (g->background_ID) {
    case BACKGROUND_ID_SNOW: {

        break;
    }
    case BACKGROUND_ID_CAVE: {

        break;
    }
    case BACKGROUND_ID_WATERFALL: {

        break;
    }
    case BACKGROUND_ID_FOREST_DARK:
    case BACKGROUND_ID_FOREST_BRIGHT: {

        break;
    }
    }
}

static i32 coord_parallax(i32 p, i32 v_q8, i32 a)
{
    return (((p * v_q8) >> 8) & ~a);
}

void background_draw(g_s *g, v2_i32 cam_al, v2_i32 cam)
{
    tex_s     tdisplay = asset_tex(0);
    gfx_ctx_s ctx      = gfx_ctx_default(tdisplay);
    i32       alx      = 3;
    i32       aly      = 3;

    cam_al.y = cam.y;

    v2_i32 pos_mid  = {coord_parallax(cam_al.x + g->bg_offx, 192, alx),
                       coord_parallax(cam_al.y + g->bg_offy, 192, aly)};
    v2_i32 pos_far  = {coord_parallax(cam_al.x + g->bg_offx, 128, 3),
                       coord_parallax(cam_al.y + g->bg_offy, 128, aly)};
    v2_i32 pos_vfar = {coord_parallax(cam_al.x + g->bg_offx, 64, alx),
                       coord_parallax(cam_al.y + g->bg_offy, 64, aly)};

    switch (g->background_ID) {
    case BACKGROUND_ID_WHITE:
        tex_clr(tdisplay, GFX_COL_WHITE);
        break;
    case BACKGROUND_ID_BLACK:
        tex_clr(tdisplay, GFX_COL_BLACK);
        break;
    case BACKGROUND_ID_SNOW: {
        texrec_s tr_mirror = asset_texrec(TEXID_BG_PARALLAX_PERF, 0, 0, 1024, 512);
        texrec_s tr_bg     = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 512, 256);
        texrec_s tr_mid    = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_far    = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);

        gfx_spr_copy(ctx, tr_bg, (v2_i32){0}, 0);
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    case BACKGROUND_ID_CAVE: {
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);

        tex_clr(tdisplay, GFX_COL_BLACK);
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    case BACKGROUND_ID_WATERFALL: {
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);
        gfx_fill_rows(tdisplay, gfx_pattern_bayer_4x4(0), 0, ctx.clip_y2);
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    case BACKGROUND_ID_FOREST_DARK: {
        tex_clr(tdisplay, GFX_COL_BLACK);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 1024, 512);

        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    case BACKGROUND_ID_FOREST_BRIGHT: {
        texrec_s tr_bg = asset_texrec(TEXID_BG_PARALLAX, 0, 1536, 512, 256);
        gfx_spr_copy(ctx, tr_bg, (v2_i32){0}, 0);
        gfx_fill_rows(tdisplay, gfx_pattern_bayer_4x4(2), 0, ctx.clip_y2);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 1536, 1024, 512);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);

        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    case BACKGROUND_ID_VERTICAL: {
        gfx_fill_rows(tdisplay, gfx_pattern_bayer_4x4(14), 0, ctx.clip_y2);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 768, 512);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 768, 512);
        pos_mid.x       = coord_parallax(cam_al.x, 192, alx);
        pos_far.x       = coord_parallax(cam_al.x, 128, 3);
        pos_far.x -= 192;
        pos_mid.x -= 160;
        gfx_spr_tileds_copy(ctx, tr_far, pos_far, 0, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 0, 1);
        break;
    }
    case BACKGROUND_ID_MOUNTAIN: {
        tex_clr(tdisplay, GFX_COL_WHITE);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);

        gfx_spr_tileds_copy(ctx, tr_mid, pos_mid, 1, 1);
        break;
    }
    }
}