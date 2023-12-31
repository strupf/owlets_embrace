// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render.h"
#include "game.h"

void game_draw(game_s *g)
{
    rec_i32 camrec = cam_rec_px(g, &g->cam);
    camrec.x &= ~1; // snap to multiples of 2 to avoid dither flickering
    camrec.y &= ~1;

    const v2_i32    camoffset = {-camrec.x, -camrec.y};
    const gfx_ctx_s ctx       = gfx_ctx_display();
    // render_parallax(g, camoffset);

    bounds_2D_s tilebounds = game_tilebounds_rec(g, camrec);
    // render_tilemap(g, TILELAYER_BG, tilebounds, camoffset);

    texrec_s tbackground = asset_texrec(TEXID_CLOUDS, 0, 0, 400, 256);
    gfx_spr(ctx, tbackground, (v2_i32){0, -16}, 0, 0);

    for (int n = 0; n < g->n_decal_bg; n++) {
        gfx_ctx_s ctx_decal = ctx;
        decal_s   decal     = g->decal_bg[n];
        texrec_s  decalrec  = {decal.tex,
                               {0, 0, decal.tex.w, decal.tex.h}};
        v2_i32    decalpos  = {decal.x, decal.y};
        gfx_spr(ctx_decal, decalrec, v2_add(decalpos, camoffset), 0, 0);
    }

    render_tilemap(g, TILELAYER_TERRAIN, tilebounds, camoffset);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && ohero->rope) {
        rope_s   *rope     = ohero->rope;
        gfx_ctx_s ctx_rope = ctx;

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
                gfx_cir_fill(ctx_rope, dd, 6, PRIM_MODE_BLACK);
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
#if 0
        gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK);
#endif

        if (o->flags & OBJ_FLAG_SPRITE) {
            for (int n = 0; n < o->n_sprites; n++) {
                sprite_simple_s sprite = o->sprites[n];
                if (sprite.trec.t.px == NULL) continue;
                v2_i32 sprpos = v2_add(ppos, sprite.offs);
                gfx_spr(ctx, sprite.trec, sprpos, sprite.flip, sprite.mode);
            }
        }

        switch (o->ID) {
        case OBJ_ID_BLOB:
            blob_on_draw(g, o, camoffset);
            break;
        case OBJ_ID_TOGGLEBLOCK:
            toggleblock_on_draw(g, o, camoffset);
            break;
        case OBJ_ID_CRAWLER:
            crawler_on_draw(g, o, camoffset);
            break;
        case OBJ_ID_HERO: {
            hero_s *hero = &g->herodata;
#if 0 // render hitboxes
            gfx_ctx_s ctxhb = ctx;
            for (int i = 0; i < hero->n_hitbox; i++) {
                hitbox_s hb = hero->hitbox_def[i];
                hb.r.x += camoffset.x;
                hb.r.y += camoffset.y;
                ctxhb.pat = gfx_pattern_interpolate(1, 2);
                gfx_rec_fill(ctxhb, hb.r, PRIM_MODE_BLACK_WHITE);
            }
#endif

#if 0 // air jump indicators
            hero_s *hero  = &g->herodata;
            bool32  inair = game_traversable(g, obj_rec_bottom(o));
            if (hero->n_airjumps > 0 && inair) {
                gfx_ctx_s ctx_airjump = gfx_ctx_default(asset_tex(0));

                for (int k = 0; k < hero->n_airjumps; k++) {
                    v2_i32 ajpos = ppos;
                    ajpos.x += -5 + k * 10;
                    ajpos.y -= 20;

                    gfx_cir_fill(ctx_airjump, ajpos, 6, PRIM_MODE_BLACK);
                    gfx_cir_fill(ctx_airjump, ajpos, 5, PRIM_MODE_WHITE);
                    if (o->n_airjumps > k) {
                        gfx_cir_fill(ctx_airjump, ajpos, 4, PRIM_MODE_BLACK);
                    }
                }
            }
#endif
        } break;
        }
    }

    for (int i = 0; i < g->particles.n; i++) {
        particle_s *p           = &g->particles.particles[i];
        v2_i32      ppos        = v2_add(v2_shr(p->p_q8, 8), camoffset);
        gfx_ctx_s   ctxparticle = ctx;
        ctxparticle.pat         = gfx_pattern_interpolate(p->ticks, p->ticks_max);

        switch (p->gfx) {
        case PARTICLE_GFX_CIR: {
            gfx_cir_fill(ctxparticle, ppos, p->size, PRIM_MODE_BLACK);
        } break;
        case PARTICLE_GFX_REC: {
            rec_i32 rr = {ppos.x, ppos.y, p->size, p->size};
            gfx_rec_fill(ctxparticle, rr, PRIM_MODE_BLACK);
        } break;
        case PARTICLE_GFX_SPR: {
            gfx_spr(ctxparticle, p->texrec, ppos, 0, 0);
        } break;
        }
    }

    for (int n = 0; n < g->n_decal_fg; n++) {
        gfx_ctx_s ctx_decal = ctx;
        decal_s   decal     = g->decal_fg[n];
        texrec_s  decalrec  = {decal.tex,
                               {0, 0, decal.tex.w, decal.tex.h}};
        v2_i32    decalpos  = {decal.x, decal.y};
        gfx_spr(ctx_decal, decalrec, v2_add(decalpos, camoffset), 0, 0);
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

    enveffect_wind_draw(ctx, &g->env_wind, camoffset);
    // enveffect_heat_draw(ctx, &g->env_heat, camoffset);

#define HORIZONT_X     2000                // distance horizont from screen plane
#define HORIZONT_X_EYE 600                 // distance eye from screen plane
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

        for (int k = 1; k < 10; k++) {
            v2_i32 p3 = p1;
            v2_i32 p4 = p2;
            p3.y -= (Y_HORIZONT * k) / 9;
            p4.y -= (Y_HORIZONT * k) / 9;
            gfx_lin_thick(ctx_ocean, p3, p4, PRIM_MODE_BLACK, 2);
        }
    }
#endif

    render_ui(g, camoffset);
    transition_draw(&g->transition);

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

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 camoffset)
{
    texrec_s tr = {0};
    switch (layer) {
    case TILELAYER_TERRAIN:
        tr.t = asset_tex(TEXID_TILESET_TERRAIN);
        break;
    case TILELAYER_BG:
        tr.t = asset_tex(TEXID_TILESET_BG);
        break;
    default: return;
    }
    gfx_ctx_s ctx = gfx_ctx_display();
    tr.r.w        = 16;
    tr.r.h        = 16;

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
#if 1
            if (g->tiles[x + y * g->tiles_x].collision == TILE_ONE_WAY) {
                rec_i32 rr = {x << 4, y << 4, 16, 4};
                rr         = translate_rec(rr, camoffset);
                gfx_rec_fill(ctx, rr, PRIM_MODE_BLACK);
            }

            rtile_s rt = g->rtiles[layer][x + y * g->tiles_x];
            // rt.u = g_animated_tiles[tiles[x + y * g->tiles_x].u];
            if (rt.u == 0) continue;

            tr.r.x   = rt.tx << 4;
            tr.r.y   = rt.ty << 4;
            v2_i32 p = {(x << 4) + camoffset.x, (y << 4) + camoffset.y};
            gfx_spr(ctx, tr, p, 0, 0);

#endif
#if defined(SYS_DEBUG) && 0
            {
                int t = g->tiles[x + y * g->tiles_x].collision;
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
    par.tr             = asset_texrec(TEXID_BACKGROUND, 0, 0, 618, 320);
    par.loopx          = 1;
    par.img_pos        = BG_IMG_POS_FIT_ROOM;

    if (!par.tr.t.px) return;
    cam_s *cam = &g->cam;

    int bgx = 0;
    int bgy = 0;

    switch (par.img_pos) {
    case BG_IMG_POS_TILED:
        bgx = (int)(cam->pos.x * (1.f - par.x) + par.offx) + camoffset.x;
        bgy = (int)(cam->pos.y * (1.f - par.y) + par.offy) + camoffset.y;
        break;
    case BG_IMG_POS_FIT_ROOM:
        bgx = -((par.tr.r.w - SYS_DISPLAY_W) * -camoffset.x) / (g->pixel_x - SYS_DISPLAY_W);
        bgy = -((par.tr.r.h - SYS_DISPLAY_H) * -camoffset.y) / (g->pixel_y - SYS_DISPLAY_H);
        break;
    }

    int nx = 1;
    int ny = 1;

    if (par.loopx) {
        nx  = (SYS_DISPLAY_W) / par.tr.r.w + 1;
        bgx = bgx % par.tr.r.w;
    }
    if (par.loopy) {
        ny  = (SYS_DISPLAY_H) / par.tr.r.h + 1;
        bgy = bgy % par.tr.r.h;
    }

    gfx_ctx_s ctx = gfx_ctx_display();

    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            v2_i32 pos = {bgx + x * par.tr.r.w, bgy + y * par.tr.r.h};
            gfx_spr(ctx, par.tr, pos, 0, 0);
        }
    }
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

    int offs = 15;
    if (!h->itemselection_decoupled) {
        int dtang = abs_i(inp_crank_calc_dt_q16(h->item_angle, inp_crank_q16()));
        offs      = min_i(pow2_i32(dtang) / 4000000, offs);
    }

#define ITEM_OVAL_Y 12

    int turn1 = (inp_crank_q16() + 0x4000) << 2;
    int turn2 = (h->item_angle + 0x4000) << 2;
    int sy1   = (sin_q16(turn1) * ITEM_OVAL_Y) >> 16;
    int sy2   = (sin_q16(turn2) * ITEM_OVAL_Y) >> 16;
    int sx1   = (cos_q16(turn1));
    int sx2   = (cos_q16(turn2));

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
        if (0 <= aimg && aimg < 0x8000) { // maps to [0, 180) deg (visible barrel)
            tr.r.y = h->selected_item * ITEM_SIZE + (aimg * ITEM_SIZE) / 0x8000;
            gfx_spr(ctx, tr, (v2_i32){16, ITEM_Y_OFFS + y}, 0, 0);
        } else {
            gfx_rec_fill(ctx, (rec_i32){16, ITEM_Y_OFFS + y, ITEM_SIZE, 1}, PRIM_MODE_WHITE);
        }
    }

    tr.r = (rec_i32){64, 0, 64, 64}; // frame
    gfx_spr(ctx, tr, (v2_i32){0, 0}, 0, 0);
    tr.r = (rec_i32){144, 0, 32, 64}; // disc
    gfx_spr(ctx, tr, (v2_i32){48 + 17 + offs, 0}, 0, 0);
    tr.r = (rec_i32){112, 64, 16, 16}; // hole
    gfx_spr(ctx, tr, (v2_i32){48, 32 - 8 - sy2}, 0, 0);
    tr.r = (rec_i32){112, 80, 16, 16}; // bolt
    gfx_spr(ctx, tr, (v2_i32){49 + offs, 32 - 8 - sy1}, 0, 0);
}

void render_ui(game_s *g, v2_i32 camoffset)
{
    gfx_ctx_s ctx_ui = gfx_ctx_display();

    fade_s *areaname = &g->areaname.fade;
    if (fade_phase(areaname) != 0) {
        int       fade_i       = fade_interpolate(areaname, 0, 100);
        fnt_s     areafont     = asset_fnt(FNTID_LARGE);
        gfx_ctx_s ctx_areafont = ctx_ui;
        ctx_areafont.pat       = gfx_pattern_interpolate(fade_i, 100);

        int areax = 10;
        int areay = 10;

        for (int yy = -2; yy <= +2; yy++) {
            for (int xx = -2; xx <= +2; xx++) {
                fnt_draw_ascii(ctx_areafont, areafont, (v2_i32){areax + xx, areay + yy},
                               g->areaname.label, SPR_MODE_WHITE);
            }
        }
        fnt_draw_ascii(ctx_areafont, areafont, (v2_i32){areax, areay}, g->areaname.label, SPR_MODE_BLACK);
    }

    if (g->herodata.aquired_items) {
        render_item_selection(&g->herodata);
        texrec_s tr_items = asset_texrec(TEXID_UI_ITEM_CACHE,
                                         0, 0, 128, ITEM_FRAME_SIZE);
        gfx_spr(ctx_ui, tr_items, (v2_i32){400 - 92, 240 - 64 + 16}, 0, 0);
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && g->textbox.state == TEXTBOX_STATE_INACTIVE) {
        v2_i32 posc         = obj_pos_center(ohero);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            v2_i32 posi = obj_pos_center(interactable);
            posi        = v2_add(posi, camoffset);
            posi.y -= 64 + 16;
            posi.x -= 32;
            int      btn_frame = tick_to_index_freq(g->tick, 2, 50);
            texrec_s tui       = asset_texrec(TEXID_UI, 64 + btn_frame * 64, 0, 64, 64);
            gfx_spr(ctx_ui, tui, posi, 0, 0);
        }
    }

    textbox_draw(&g->textbox, camoffset);
}

void render_pause(game_s *g)
{
    spm_push();
    tex_s     tex = tex_create(SYS_DISPLAY_W, SYS_DISPLAY_H, spm_allocator);
    gfx_ctx_s ctx = gfx_ctx_default(tex);

    for (int i = 0; i < SYS_DISPLAY_H * SYS_DISPLAY_WBYTES; i++) {
        tex.px[i] = rngr_i32(0, 255);
    }

    sys_set_menu_image(tex.px, tex.h, tex.wbyte);
    spm_pop();
}