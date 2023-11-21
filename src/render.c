// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "assets.h"
#include "game.h"
#include "gfx.h"

void render(game_s *g)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));

    rec_i32 camrec    = cam_rec_px(&g->cam);
    v2_i32  camoffset = {-camrec.x, -camrec.y};

    bounds_2D_s tilebounds = game_tilebounds_rec(g, camrec);
    render_tilemap(g, tilebounds, camoffset);

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s   *o    = g->obj_busy[i];
        texrec_s trec = {0};
        v2_i32   ppos = v2_add(o->pos, camoffset);
        rec_i32  aabb = {ppos.x, ppos.y, o->w, o->h};
#if 1
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
#endif

        switch (o->ID) {
        case OBJ_ID_HERO: {
            trec.t = asset_tex(TEXID_HERO);

            texrec_s trec1 = spriteanim_frame(&o->spriteanim[0]);

            int animID  = 0;
            trec.r      = (rec_i32){0, animID * 64, 64, 64};
            v2_i32 rpos = v2_add(obj_pos_bottom_center(o), camoffset);
            rpos.y -= trec1.r.h;
            rpos.x -= trec1.r.w / 2;

            int flip = o->facing == -1 ? SPR_FLIP_X : 0;

            gfx_spr(ctx, trec1, rpos, flip, 0);
        } break;
        }
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && ohero->rope) {
        rope_s   *rope     = ohero->rope;
        gfx_ctx_s ctx_rope = gfx_ctx_default(asset_tex(0));

        int         inode       = 0;
        int         lensofar_q4 = 0;
        ropenode_s *r1          = rope->tail;
        ropenode_s *r2          = r1->prev;

        while (r1 && r2) {
            v2_i32 p1        = v2_add(r1->p, camoffset);
            v2_i32 p2        = v2_add(r2->p, camoffset);
            v2_i32 dt12_q4   = v2_shl(v2_sub(p2, p1), 4);
            int    lenend_q4 = lensofar_q4 + v2_len(dt12_q4);

            while (inode * 80 < lenend_q4) {
                int dst = inode * 80 - lensofar_q4;
                inode++;
                assert(dst >= 0);
                v2_i32 dd = dt12_q4;
                dd        = v2_setlen(dd, dst);
                dd        = v2_shr(dd, 4);
                dd        = v2_add(dd, p1);
                gfx_cir_fill(ctx_rope, dd, 4, PRIM_MODE_BLACK);
                gfx_cir_fill(ctx_rope, dd, 1, PRIM_MODE_WHITE);
            }
            lensofar_q4 = lenend_q4;
            r1          = r2;
            r2          = r2->prev;
        }
    }

    for (int n = 0; n < g->n_windparticles; n++) {
        windparticle_s p  = g->windparticles[n];
        v2_i32         p1 = v2_add(camoffset, v2_shr(p.pos_q8[p.n], 8));

        for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
            int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
            v2_i32 p2 = v2_add(v2_shr(p.pos_q8[k], 8), camoffset);
            gfx_lin_thick(ctx, p1, p2, PRIM_MODE_BLACK, 1);
            p1 = p2;
        }
    }

    render_ui(g, camoffset);

#if 0 // speech bubble animation
    rec_i32 rec = {100, 50, 50, 30};

    gfx_rec_fill(ctx, translate_rec(rec, camoffset), 0);

    v2_i32 cc = {
        rec.x + rec.w / 2,
        rec.y + rec.h / 2};

    v2_i32 speaker = obj_pos_center(obj_get_tagged(g, OBJ_TAG_HERO));
    v2_i32 dt      = v2_sub(cc, speaker);

    v2_i32 dta, dtb;

    float ang   = 0.3f;
    dta.x       = cos_f(ang) * (f32)dt.x - sin_f(ang) * (f32)dt.y;
    dta.y       = sin_f(ang) * (f32)dt.x + cos_f(ang) * (f32)dt.y;
    ang         = -ang;
    dtb.x       = cos_f(ang) * (f32)dt.x - sin_f(ang) * (f32)dt.y;
    dtb.y       = sin_f(ang) * (f32)dt.x + cos_f(ang) * (f32)dt.y;
    tri_i32 tri = {speaker,
                   v2_add(speaker, dta),
                   v2_add(speaker, dtb)};
    tri         = translate_tri(tri, camoffset);
    gfx_tri_fill(ctx, tri, 0);
#endif
}

void render_tilemap(game_s *g, bounds_2D_s bounds, v2_i32 camoffset)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};

#if 1
            for (int i = 0; i < 2; i++) {
                int t = g_animated_tiles[g->rtiles[x + y * g->tiles_x].layer[i]];
                if (t == 0) continue;
                int tx, ty;
                rtile_unpack(t, &tx, &ty);
                rec_i32  r = {tx << 4, ty << 4, 16, 16};
                texrec_s tr;
                tr.t = asset_tex(TEXID_TILESET);
                tr.r = r;
                gfx_spr(ctx, tr, p, 0, 0);
            }
#endif
#if defined(SYS_DEBUG) && 0
            {
                int t = g->tiles[x + y * g->tiles_x];
                if (!(0 < t && t < NUM_TILE_BLOCKS)) continue;
                texrec_s tr;
                tr.t   = asset_tex(TEXID_COLLISION_TILES);
                tr.r.x = 0;
                tr.r.y = t * 16;
                tr.r.w = 16;
                tr.r.h = 16;
                gfx_spr(ctx, tr, p, 0, 0);
            }
#endif
        }
    }
}

void render_parallax(game_s *g, v2_i32 camoffset)
{
    parallax_img_s par = g->parallax;
    cam_s         *cam = &g->cam;

    int bgx = (int)(cam->pos.x * (1.f - par.x)) + par.offx + camoffset.x;
    int bgy = (int)(cam->pos.y * (1.f - par.y)) + par.offy + camoffset.y;
}

enum {
    ITEM_FRAME_SIZE = 64,
    ITEM_BARREL_R   = 16,
    ITEM_BARREL_D   = ITEM_BARREL_R * 2,
    ITEM_SIZE       = 32,
    ITEM_X_OFFS     = 16,
    ITEM_Y_OFFS     = 16,
};

static void item_selection_redraw(hero_s *h)
{
    // interpolator based on crank position
    h->itemselection_dirty = 0;
    int ii                 = -((ITEM_SIZE * inp_crank_q16()) >> 16);
    if (inp_crank_q16() >= 0x8000) {
        ii += ITEM_SIZE;
    }

    int itemIDs[3] = {h->selected_item_prev,
                      h->selected_item,
                      h->selected_item_next};

    tex_s     texcache = asset_tex(TEXID_UI_ITEM_CACHE);
    gfx_ctx_s ctx      = gfx_ctx_default(texcache);
    texrec_s  tr;
    tr.t = asset_tex(TEXID_UI_ITEMS);
    tex_clr(texcache, TEX_CLR_TRANSPARENT);

    for (int y = -ITEM_BARREL_R; y <= +ITEM_BARREL_R; y++) {
        int     a_q16   = (y << 16) / ITEM_BARREL_R;
        int     arccos  = (acos_q16(a_q16) * ITEM_SIZE) >> (16 + 1);
        int     loc     = arccos + ITEM_SIZE - ii;
        int     itemi   = loc / ITEM_SIZE;
        int     yy      = ITEM_SIZE * itemIDs[itemi] + loc % ITEM_SIZE;
        int     uu      = ITEM_BARREL_R - y + ITEM_Y_OFFS;
        rec_i32 itemrow = {ITEM_SIZE, yy, ITEM_SIZE, 1};
        tr.r            = itemrow;
        gfx_spr(ctx, tr, (v2_i32){ITEM_X_OFFS, uu}, 0, 0);
    }
    tr.r = (rec_i32){64, 0, 64, 64};
    gfx_spr(ctx, tr, (v2_i32){0, 0}, 0, 0);
}

void render_ui(game_s *g, v2_i32 camoffset)
{
    gfx_ctx_s ctx_ui = gfx_ctx_default(asset_tex(0));

    if (g->herodata.aquired_items) {
        if (g->herodata.itemselection_dirty) {
            item_selection_redraw(&g->herodata);
        }

        texrec_s tr_items;
        tr_items.t = asset_tex(TEXID_UI_ITEM_CACHE);
        tr_items.r = (rec_i32){0, 0, ITEM_FRAME_SIZE, ITEM_FRAME_SIZE};
        gfx_spr(ctx_ui, tr_items, (v2_i32){400 - 64 + 16, 240 - 64 + 16}, 0, 0);
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && g->textbox.state == TEXTBOX_STATE_INACTIVE) {
        v2_i32 posc         = obj_pos_center(ohero);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            v2_i32 posi = obj_pos_center(interactable);
            posi        = v2_add(posi, camoffset);
            posi.y -= 48;
            posi.x -= 16;
            int      btn_frame = tick_to_index_freq(g->tick, 4, 50);
            texrec_s tui;
            tui.t = asset_tex(TEXID_UI);
            tui.r = (rec_i32){btn_frame * 32 + 64, 32, 32, 32};
            gfx_spr(ctx_ui, tui, posi, 0, 0);
        }
    }

    textbox_draw(&g->textbox, camoffset);
}
