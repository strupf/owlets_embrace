// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"

void render(game_s *g)
{
    gfx_ctx_s ctx = gfx_ctx_default(asset_tex(0));

    rec_i32 camrec    = cam_rec_px(&g->cam);
    v2_i32  camoffset = {-camrec.x, -camrec.y};

    gfx_ctx_s ctx_clouds = gfx_ctx_default(asset_tex(0));
    texrec_s  trcloud    = asset_texrec(TEXID_CLOUDS, 0, 0, 256, 256);
    gfx_spr(ctx_clouds, trcloud, (v2_i32){0, 0}, 0, 0);

    bounds_2D_s tilebounds = game_tilebounds_rec(g, camrec);
    render_tilemap(g, tilebounds, camoffset);

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
                gfx_cir_fill(ctx_rope, dd, 2, PRIM_MODE_WHITE);
            }
            lensofar_q4 = lenend_q4;
            r1          = r2;
            r2          = r2->prev;
        }
    }

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
            trec.t         = asset_tex(TEXID_HERO);
            hero_s  *hero  = &g->herodata;
            texrec_s trec1 = spriteanim_frame(&o->spriteanim[0]);

            int animID   = 0;
            trec.r       = (rec_i32){0, animID * 64, 64, 64};
            v2_i32 rpos  = v2_add(obj_pos_bottom_center(o), camoffset);
            v2_i32 pwhip = rpos;
            rpos.y -= trec1.r.h;
            rpos.x -= trec1.r.w / 2;

            int flip = o->facing == -1 ? SPR_FLIP_X : 0;

            gfx_spr(ctx, trec1, rpos, flip, 0);

            if (hero->attack != HERO_ATTACK_NONE) {
                int times[] = {2,
                               4,
                               6,
                               8,
                               16,
                               17,
                               18,
                               20};

                int fr = 0;
                while (hero->attack_tick < times[7 - fr]) {
                    fr++;
                }
                fr = clamp_i(fr, 0, 7);
                texrec_s trwhip;
                trwhip.t   = asset_tex(TEXID_HERO_WHIP);
                trwhip.r.x = fr * 256;
                trwhip.r.y = (hero->attack - 1) * 256;
                trwhip.r.w = 256;
                trwhip.r.h = 256;

                pwhip.x -= 128 - o->facing * 10;
                pwhip.y -= 128;
                gfx_spr(ctx, trwhip, pwhip, flip, 0);
            }
        } break;
        }
    }

    texrec_s trgrass;
    trgrass.t = asset_tex(TEXID_PLANTS);
    for (int n = 0; n < g->n_grass; n++) {
        grass_s *gr  = &g->grass[n];
        v2_i32   pos = v2_add(gr->pos, camoffset);

        for (int i = 0; i < 16; i++) {
            v2_i32 p = pos;
            p.y += i;

            p.x += (gr->x_q8 * (15 - i)) >> 8;
            rec_i32 rg = {8, i + gr->type * 16, 16, 1};
            trgrass.r  = rg;
            // gfx_sprite(ctx, p, rg, 0);
            gfx_spr(ctx, trgrass, p, 0, 0);
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

#define HORIZONT_X     600                 // distance horizont from screen plane
#define HORIZONT_X_EYE 300                 // distance eye from screen plane
#define HORIZONT_Y_EYE (SYS_DISPLAY_H / 2) // height eye on screen plane (center)

    // calc height of horizont based on thales theorem
    int Y_HORIZONT = ((g->ocean.y + camoffset.y - HORIZONT_Y_EYE) * HORIZONT_X) /
                     (HORIZONT_X + HORIZONT_X_EYE);

#if 0
    for (int i = 1; i < g->ocean.water.nparticles; i++) {
        int       h1        = ocean_amplitude(&g->ocean, i - 1) + camoffset.y;
        int       h2        = ocean_amplitude(&g->ocean, i + 0) + camoffset.y;
        gfx_ctx_s ctx_ocean = gfx_ctx_default(asset_tex(0));

        v2_i32 p1 = {(i - 1) * 4, h1};
        v2_i32 p2 = {(i + 0) * 4, h2};

        gfx_lin_thick(ctx_ocean, p1, p2, PRIM_MODE_BLACK, 2);
        if (Y_HORIZONT <= 0) continue;

        v2_i32 p3 = p1;
        v2_i32 p4 = p2;
        p3.y -= Y_HORIZONT;
        p4.y -= Y_HORIZONT;
        gfx_lin_thick(ctx_ocean, p3, p4, PRIM_MODE_BLACK, 2);
    }
#endif

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

    texrec_s tr;
    tr.t = asset_tex(TEXID_TILESET);

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};

#if 1
            for (int i = 0; i < 2; i++) {
                int t = g_animated_tiles[g->rtiles[x + y * g->tiles_x].layer[i]];
                if (t == 0) continue;
                int tx, ty;
                rtile_unpack(t, &tx, &ty);
                rec_i32 r = {tx << 4, ty << 4, 16, 16};

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

#define ITEM_FRAME_SIZE 64
#define ITEM_BARREL_R   16
#define ITEM_SIZE       32
#define ITEM_X_OFFS     16
#define ITEM_Y_OFFS     16

static void render_item_selection(hero_s *h)
{
    tex_s     texcache = asset_tex(TEXID_UI_ITEM_CACHE);
    gfx_ctx_s ctx      = gfx_ctx_default(texcache);
    texrec_s  tr       = {0};
    tr.t               = asset_tex(TEXID_UI_ITEMS);
    tex_clr(texcache, TEX_CLR_TRANSPARENT);

    // barrel background
    tr.r = (rec_i32){144, 0, 64, 64};
    gfx_spr(ctx, tr, (v2_i32){0, 0}, 0, 0);

    int discoffset = 10;
    if (!h->itemselection_decoupled) {
        int dtang  = abs_i(inp_crank_calc_dt_q16(h->item_angle, inp_crank_q16()));
        discoffset = min_i(pow2_i32(dtang) / 5000000, discoffset);
    }

#define ITEM_OVAL_X 2
#define ITEM_OVAL_Y 12

    int turn1 = (inp_crank_q16() + 0x4000) << 2;
    int turn2 = (h->item_angle + 0x4000) << 2;
    int sx1   = (cos_q16(turn1) * ITEM_OVAL_X) >> 16;
    int sy1   = (sin_q16(turn1) * ITEM_OVAL_Y) >> 16;
    int sx2   = (cos_q16(turn2) * ITEM_OVAL_X) >> 16;
    int sy2   = (sin_q16(turn2) * ITEM_OVAL_Y) >> 16;

    // bolt
    tr.r = (rec_i32){208, 80, 16, 16};
    gfx_spr(ctx, tr, (v2_i32){33 - sx1 + discoffset, 32 - 8 - sy1}, 0, 0);

    tr.r = (rec_i32){208, 96, 16, 16};
    gfx_spr(ctx, tr, (v2_i32){39 - sx2, 32 - 8 - sy2}, 0, 0);

    // wraps the item image around a rotating barrel
    // map coordinate to angle to image coordinate
    //   |
    //   v_________
    //   /  \      \
    //  /    \      \
    //  |     \     |
    tr.r.x = 32;
    tr.r.w = ITEM_SIZE;
    tr.r.h = 1;
    for (int y = 0; y < 2 * ITEM_BARREL_R; y++) {
        int yy   = y - ITEM_BARREL_R;
        int abar = asin_q16((yy << 16) / ITEM_BARREL_R) >> 2; // asin returns TURN in Q18, one turn = 0x40000, shr by 2 for Q16
        int aimg = (abar + h->item_angle + 0x4000) & 0xFFFF;
        int offx = (cos_q16((yy << 16) / ITEM_BARREL_R) * 4) >> 16;
        offx     = clamp_i(offx, 0, 3);
        if (0 <= aimg && aimg < 0x8000) { // maps to [0, 180) deg (visible barrel)
            tr.r.y = h->selected_item * ITEM_SIZE + (aimg * ITEM_SIZE) / 0x8000;
            gfx_spr(ctx, tr, (v2_i32){6 - offx, ITEM_Y_OFFS + y}, 0, 0);
        } else {
            gfx_rec_fill(ctx, (rec_i32){6 - offx, ITEM_Y_OFFS + y, ITEM_SIZE, 1}, PRIM_MODE_WHITE);
        }
    }

    // black barrel frame
    tr.r = (rec_i32){144, 64, 64, 64};
    gfx_spr(ctx, tr, (v2_i32){0, 0}, 0, 0);

    // crank disc
    tr.r = (rec_i32){208, 0, 32, 64};
    gfx_spr(ctx, tr, (v2_i32){41 + discoffset, 0}, 0, 0);
    tr.r = (rec_i32){208, 112, 16, 16};
    gfx_spr(ctx, tr, (v2_i32){45 - sx1 + discoffset, 32 - 8 - sy1}, 0, 0);
}

void render_ui(game_s *g, v2_i32 camoffset)
{
    gfx_ctx_s ctx_ui = gfx_ctx_default(asset_tex(0));

    if (g->area_name_ticks > 0) {
        fnt_s     areafont     = asset_fnt(FNTID_DIALOG);
        gfx_ctx_s ctx_areafont = ctx_ui;

        int t1 = (AREA_NAME_TICKS * 1) / 8; // fade out
        int t2 = (AREA_NAME_TICKS * 7) / 8; // fade in
        if (g->area_name_ticks < t1) {
            int a            = g->area_name_ticks;
            int b            = t1;
            ctx_areafont.pat = gfx_pattern_interpolate(a, b);
        } else if (g->area_name_ticks > t2) {
            int a            = AREA_NAME_TICKS - g->area_name_ticks;
            int b            = AREA_NAME_TICKS - t2;
            ctx_areafont.pat = gfx_pattern_interpolate(a, b);
        }

        fnt_draw_ascii(ctx_areafont, areafont, (v2_i32){10, 10}, g->area_name, 0);
    }

    if (g->herodata.aquired_items) {
        render_item_selection(&g->herodata);
        texrec_s tr_items = asset_texrec(TEXID_UI_ITEM_CACHE,
                                         0, 0, ITEM_FRAME_SIZE, ITEM_FRAME_SIZE);
        gfx_spr(ctx_ui, tr_items, (v2_i32){400 - 64, 240 - 64 + 16}, 0, 0);
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && g->textbox.state == TEXTBOX_STATE_INACTIVE) {
        v2_i32 posc         = obj_pos_center(ohero);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            v2_i32 posi = obj_pos_center(interactable);
            posi        = v2_add(posi, camoffset);
            posi.y -= 64;
            posi.x -= 32;
            int      btn_frame = tick_to_index_freq(g->tick, 2, 50);
            texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
            gfx_spr(ctx_ui, tui, posi, 0, 0);
        }
    }

    textbox_draw(&g->textbox, camoffset);
}
