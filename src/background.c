// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "game.h"
#include "render.h"

void background_set(g_s *g, i32 ID)
{
    background_s *bg = &g->background;
    mclr(bg, sizeof(background_s));

    bg->ID     = ID;
    tex_s *tbg = asset_texptr(TEXID_BG_PARALLAX);

    switch (ID) {
    default: break;
    case BACKGROUND_ID_CAVE:
        tex_from_wad_ext("T_BG_CAVE", game_allocator_room(g), tbg);
        break;
    case BACKGROUND_ID_FOREST_DARK:
    case BACKGROUND_ID_FOREST_BRIGHT:
        tex_from_wad_ext("T_BG_FOREST", game_allocator_room(g), tbg);
        break;
    case BACKGROUND_ID_SNOW:
        tex_from_wad_ext("T_BG_SNOW", game_allocator_room(g), tbg);
        break;
    case BACKGROUND_ID_WATERFALL:
        tex_from_wad_ext("T_BG_WATERFALL", game_allocator_room(g), tbg);
        break;
    case BACKGROUND_ID_VERTICAL:
        tex_from_wad_ext("T_BG_VERTICAL", game_allocator_room(g), tbg);
        break;
    case BACKGROUND_ID_MOUNTAIN:
        tex_from_wad_ext("T_BG_MOUNTAIN", game_allocator_room(g), tbg);
        break;
    }
}

static i32 coord_parallax(i32 p, i32 v_q8, i32 a)
{
    return (((p * v_q8) >> 8) & ~a);
}

void background_draw(g_s *g, v2_i32 cam_al, v2_i32 cam)
{
    background_s *bg       = &g->background;
    tex_s         tdisplay = asset_tex(0);
    gfx_ctx_s     ctx      = gfx_ctx_from_tex(tdisplay);
    i32           alx      = 3;
    i32           aly      = 3;

    cam_al.y = cam.y;

    v2_i32 pos_mid  = {coord_parallax(cam_al.x + bg->offx, 192, alx),
                       coord_parallax(cam_al.y + bg->offy, 192, aly)};
    v2_i32 pos_far  = {coord_parallax(cam_al.x + bg->offx, 128, 3),
                       coord_parallax(cam_al.y + bg->offy, 128, aly)};
    v2_i32 pos_vfar = {coord_parallax(cam_al.x + bg->offx, 64, alx),
                       coord_parallax(cam_al.y + bg->offy, 64, aly)};

    // check if faded to 100% black or white
    i32 bgID = bg->ID;
    if (bg->fade_pt == -BACKGROUND_PT_MAX) {
        bgID = BACKGROUND_ID_BLACK;
    }
    if (bg->fade_pt == +BACKGROUND_PT_MAX) {
        bgID = BACKGROUND_ID_WHITE;
    }

    switch (bgID) {
    default: break;
    case BACKGROUND_ID_WHITE:
        tex_clr(tdisplay, GFX_COL_WHITE);
        break;
    case BACKGROUND_ID_BLACK:
        tex_clr(tdisplay, GFX_COL_BLACK);
        break;
    case BACKGROUND_ID_SNOW: {
        texrec_s tr_bg  = asset_texrec(TEXID_BG_PARALLAX, 0, 1024, 512, 256);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);

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
        texrec_s tr_bg  = asset_texrec(TEXID_BG_PARALLAX, 128, 512 * 2 + 128, 512, 256);
        // gfx_spr_copy(ctx, tr_bg, (v2_i32){0}, 0);
        texrec_s tr_mid = asset_texrec(TEXID_BG_PARALLAX, 0, 0, 1024, 512);
        texrec_s tr_far = asset_texrec(TEXID_BG_PARALLAX, 0, 512, 1024, 512);

        v2_i32 pbg = {coord_parallax(cam_al.x + bg->offx, 32, 1), 0};

        i32    off_clouds = g->tick_animation >> 1;
        v2_i32 pclouds    = {coord_parallax(cam_al.x + bg->offx + off_clouds, 64, 1),
                             coord_parallax(cam_al.y + bg->offy, 64, 1)};
        v2_i32 procks     = {coord_parallax(cam_al.x + bg->offx, 192, 1),
                             coord_parallax(cam_al.y + bg->offy, 192, 1)};
        gfx_spr_tileds_copy(ctx, tr_bg, pbg, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_far, pclouds, 1, 1);
        gfx_spr_tileds_copy(ctx, tr_mid, procks, 1, 1);
        break;
    }
    }

    if (abs_i32(bg->fade_pt) != BACKGROUND_PT_MAX) {
        rec_i32   rfill   = {0, 0, 400, 240};
        gfx_ctx_s ctxfill = ctx;
        ctxfill.pat       = gfx_pattern_bayer_8x8(abs_i32(bg->fade_pt));
        i32 col_fill      = bg->fade_pt < 0 ? PRIM_MODE_BLACK : PRIM_MODE_WHITE;
        gfx_rec_fill_opaque(ctxfill, rfill, col_fill);
    }
}

void background_fade_to(g_s *g, i32 f_q6, i32 ticks)
{
    background_s *bg = &g->background;

    i32 p = clamp_i32(f_q6, -64, +64) & ~1;
    if (p != bg->fade_pt) {
        bg->fade_ticks       = 0;
        bg->fade_ticks_total = max_i32(1, ticks);
        bg->fade_pt_src      = bg->fade_pt;
        bg->fade_pt_dst      = p;
    }
}

void background_update(g_s *g)
{
    background_s *bg = &g->background;

    if (bg->fade_ticks_total) {
        bg->fade_ticks++;

        if (bg->fade_ticks_total <= bg->fade_ticks) {
            bg->fade_pt          = bg->fade_pt_dst;
            bg->fade_ticks_total = 0;
            bg->fade_ticks       = 0;
        } else {
            bg->fade_pt = lerp_i32(bg->fade_pt_src, bg->fade_pt_dst, bg->fade_ticks, bg->fade_ticks_total) & ~1;
        }
    }
}