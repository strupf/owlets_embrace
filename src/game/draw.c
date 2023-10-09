// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "draw.h"
#include "game.h"
#include "os/os.h"

void merge_layer(tex_s screen, tex_s layer)
{
        int l = screen.w_word * screen.h;
        ASSERT((l & 3) == 0);
        ASSERT(screen.w_word == layer.w_word && screen.h == layer.h);
        ASSERT(layer.mk && layer.px);
        u32 *sp = (u32 *)screen.px;
        u32 *lp = (u32 *)layer.px;
        u32 *lm = (u32 *)layer.mk;

        for (int n = 0; n < l; n++, sp++, lm++, lp++) {
                *sp = ((*sp) & ~(*lm)) | ((*lp) & (*lm));
        }
}

void draw_background(game_s *g, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(TEXID_LAYER_1));
        gfx_tex_clr(ctx.dst);
        ctx.pat = g_gfx_patterns[GFX_PATTERN_31];
        ctx.src = tex_get(TEXID_CLOUDS);

        backforeground_s *b = &g->backforeground;

        for (int n = 0; n < b->n_clouds; n++) {
                cloudbg_s c   = b->clouds[n];
                v2_i32    pos = v2_shr(c.p, 8);
                pos.x         = (pos.x + 1) & ~1;
                pos.y         = (pos.y + 1) & ~1;
                rec_i32 r     = {0};
                switch (c.cloudtype) {
                case 0:
                        r = (rec_i32){0, 0, 110, 90};
                        break;
                case 1:
                        r = (rec_i32){125, 15, 80, 60};
                        break;
                case 2:
                        r = (rec_i32){22, 110, 110, 60};
                        break;
                }

                gfx_sprite(ctx, pos, r, 0);
        }
}

void draw_foreground(game_s *g, v2_i32 camp)
{
        // wind animation
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.pat           = g_gfx_patterns[GFX_PATTERN_44];
        ctx.col           = 1;

        backforeground_s *b = &g->backforeground;

        for (int n = 0; n < b->n_windparticles; n++) {
                windparticle_s p  = b->windparticles[n];
                v2_i32         p1 = v2_add(camp, v2_shr(p.pos[p.n], 8));

                for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
                        int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
                        v2_i32 p2 = v2_add(v2_shr(p.pos[k], 8), camp);
                        gfx_line(ctx, p1, p2);
                        p1 = p2;
                }
        }
        ctx.pat = g_gfx_patterns[GFX_PATTERN_100];
        ctx.src = tex_get(TEXID_HERO);
        for (int n = 0; n < b->n_grass; n++) {
                grass_s *gr  = &b->grass[n];
                v2_i32   pos = v2_add(gr->pos, camp);

                for (int i = 0; i < 16; i++) {
                        v2_i32 p = pos;
                        p.y += i;
                        p.x += (gr->x_q8 * (15 - i)) >> 8;
                        rec_i32 rg = {288, 448 + i, 16, 1};
                        gfx_sprite(ctx, p, rg, 0);
                }
        }
}

void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, v2_i32 camp)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.src           = tex_get(TEXID_TILESET);

        int     tx, ty;
        rec_i32 r = {0, 0, 16, 16};
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                v2_i32 pos = v2_add((v2_i32){(x << 4), (y << 4)}, camp);
                u32    rID = g->rtiles[x + y * g->tiles_x];
                if (rID == 0xFFFFFFFFU) continue;
                int ID0, ID1;
                rtile_decode(rID, &ID0, &ID1);
                if (ID0 != TILEID_NULL) {
                        tileID_decode(ID0, &tx, &ty);
                        r.x = tx << 4;
                        r.y = ty << 4;
                        gfx_sprite_tile16(ctx, pos, r, 0);
                }

                if (ID1 != TILEID_NULL) {
                        tileID_decode(ID1, &tx, &ty);
                        r.x = tx << 4;
                        r.y = ty << 4;
                        gfx_sprite_tile16(ctx, pos, r, 0);
                }
        }
}

void draw_rope(rope_s *r, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.pat           = gfx_pattern_get(GFX_PATTERN_100);
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                gfx_line_thick(ctx, v2_add(r1->p, camp), v2_add(r2->p, camp), 3);
        }

        ctx.col = 0;
        ctx.pat = gfx_pattern_get(GFX_PATTERN_69);
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                gfx_line_thick(ctx, v2_add(r1->p, camp), v2_add(r2->p, camp), 1);
        }
}

void draw_particles(game_s *g, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));

        backforeground_s *b = &g->backforeground;

        for (int n = 0; n < b->n_particles; n++) {
                particle_s *p = &b->particles[n];
                ctx.col       = 1;
                ctx.pat       = gfx_pattern_get(16 * p->ticks / p->ticks_og);
                rec_i32 r     = {(p->p_q8.x >> 8) - p->size / 2 + camp.x,
                                 (p->p_q8.y >> 8) - p->size / 2 + camp.y,
                                 p->size, p->size};
                gfx_rec_fill(ctx, r);
        }
}

#define DRAW_WATER 0

static void draw_gameplay(game_s *g)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.pat           = gfx_pattern_get(GFX_PATTERN_100);
        v2_i32  camp      = {-(g->cam.pos.x - g->cam.wh),
                             -(g->cam.pos.y - g->cam.hh)};
        rec_i32 camr      = {-camp.x, -camp.y, g->cam.w, g->cam.h};

        backforeground_s *b = &g->backforeground;
        draw_background(g, camp);

        merge_layer(tex_get(0), tex_get(TEXID_LAYER_1));

        // parallax
        /*
        f32 bgx = (-g->cam.pos.x * (g->parallax_x - 1.f)) +
                  b->parallax_offx + camp.x;
        f32 bgy = (-g->cam.pos.y * (g->parallax_y - 1.f)) +
                  g->parallax_offy + camp.y;
                  */

        ctx.src = tex_get(TEXID_TILESET);

        /*
        ctx.pat = gfx_pattern_get(8);
        gfx_sprite(ctx, (v2_i32){(int)bgx, (int)bgy},
                   (rec_i32){0, 512, 512, 512}, 0);

        ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        */
#if DRAW_WATER
        gfx_context_s ctx2 = ctx;
        ctx2.col           = 0;
        for (int n = 1; n < g->ocean.ocean.nparticles; n++) {
                i32 yy1   = ocean_amplitude(&g->ocean, n - 1) + camp.y;
                i32 yy2   = ocean_amplitude(&g->ocean, n) + camp.y;
                int xx1   = (n - 1) * 4 + camp.x;
                int xx2   = n * 4 + camp.x;
                int y_max = MAX(yy1, yy2);

                v2_i32 wp0 = {xx1, yy1};
                v2_i32 wp1 = {xx2, yy2};
                v2_i32 wp2 = {yy1 < yy2 ? xx1 : xx2, y_max};

                gfx_tri_fill(ctx2, wp0, wp1, wp2);
                gfx_rec_fill(ctx2, (rec_i32){xx1, y_max, 4, 100});
        }
#endif

        i32 x1, y1, x2, y2;
        room_tilebounds_rec(g, camr, &x1, &y1, &x2, &y2);

        TIMING_BEGIN(TIMING_DRAW_TILES);
        draw_tiles(g, x1, y1, x2, y2, camp);
        TIMING_END();

        hero_s *hero = (hero_s *)obj_get_tagged(g, OBJ_TAG_HERO);
        if (hero->o.rope)
                draw_rope(hero->o.rope, camp);

        //   objset_sort(&(g->objbuckets[OBJ_BUCKET_RENDERABLE].set), obj_cmp_renderable);
        obj_listc_s orender = objbucket_list(g, OBJ_BUCKET_RENDERABLE);
        for (int n = 0; n < orender.n; n++) {
                obj_s *o   = orender.o[n];
                v2_i32 pos = v2_add(o->pos, camp);
        }

        obj_listc_s oalive = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < oalive.n; n++) {
                obj_s *o   = oalive.o[n];
                v2_i32 pos = v2_add(o->pos, camp);
                switch (o->ID) {
                case OBJ_ID_ARROW: {
                        ctx.src   = tex_get(TEXID_SOLID);
                        int   tx  = 0;
                        int   ty  = 80;
                        float ang = atan2f((float)-o->vel_q8.y, (float)o->vel_q8.x);
                        pos.x -= 16;
                        pos.y -= 8;
                        gfx_sprite_rotated_(ctx, pos,
                                            (rec_i32){tx, ty, 32, 32},
                                            (v2_i32){16, 16}, ang);
                } break;
                case OBJ_ID_HERO: {
                        // just got hit flashing
                        if (o->invincibleticks > 0 && (o->invincibleticks & 15) < 8)
                                break;
                        ctx.src   = tex_get(TEXID_HERO);
                        int flags = o->facing == 1 ? 0 : SPRITE_FLIP_X;

                        if (hero_using_whip(&hero->o)) {
                                int whipframe = lerp_i32(2, 0, hero->whip_ticks, HERO_C_WHIP_TICKS);
                                gfx_sprite(ctx, (v2_i32){pos.x - 10, pos.y - 16}, (rec_i32){192 + whipframe * 48, 256, 48, 32}, flags);
                        }

                        pos.x -= 28;
                        pos.y = pos.y + o->h - 64;

                        int animx = 0;
                        int animy = 0;
                        switch (hero->anim) {
                        case HERO_ANIM_IDLE:
                                animy = (hero->animstate / 30) % 2;
                                break;
                        case HERO_ANIM_WALKING:
                                if (hero->animstate <= 15) {
                                        animy = 2;
                                        animx = min_i(hero->animstate / 5, 1);
                                } else {
                                        animx = (hero->animstate / 10) % 4;
                                }

                                break;
                        }

                        gfx_sprite(ctx, pos, (rec_i32){animx * 64, animy * 64, 64, 64}, flags);
                        // gfx_rec_fill(ctx, translate_rec(obj_aabb(o), camp));
                } break;
                case OBJ_ID_SIGN: {
                        ctx.src = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite(ctx, pos, (rec_i32){416, 0, 64, 64}, 0);
                } break;
                case OBJ_ID_BOAT: {
                        /*
                        ctx.src = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y -= 90;
                        v2_i32 boatc = obj_aabb_center(o);
                        // int    oceany1 = ocean_amplitude(&g->ocean, boatc.x - 16);
                        // int    oceany2 = ocean_amplitude(&g->ocean, boatc.x + 16);
                        float  ang   = atan2f(oceany2 - oceany1, 32.f * 8.f);

                        int texy = (os_tick() % 60 < 30 ? 64 : 64 + 128);

                        gfx_sprite_rotated_(ctx, pos, (rec_i32){288, texy, 128, 128},
                                            (v2_i32){64, 100}, ang);
                        // gfx_sprite(ctx, pos, (rec_i32){288, 64, 128, 128}, 0);
                        rec_i32 aabb = obj_aabb(o);
                        aabb         = translate_rec(aabb, camp);
                        // gfx_rec_fill(ctx, aabb);
                        */
                } break;
                case OBJ_ID_CRUMBLEBLOCK: {
                        if (o->state == 2) break;
                        ctx.src = tex_get(TEXID_SOLID);
                        if (o->state == 1) {
                                pos.x += rng_range(-1, +1);
                        }

                        gfx_sprite(ctx, pos, (rec_i32){80, 0, 16, 16}, 0);
                } break;
                default: {
                        rec_i32 aabb = translate_rec(obj_aabb(o), camp);
                        gfx_rec_fill(ctx, aabb);
                } break;
                }
        }

        if (!os_low_fps()) { // skip particle drawing if FPS are low
                draw_particles(g, camp);
        }

        gfx_context_s wctx = gfx_context_create(tex_get(0));
        wctx.col           = 1;
#if 0
        for (int n = 1; n < g->water.nparticles; n++) {
                i32 yy1 = water_amplitude(&g->water, n - 1);
                i32 yy2 = water_amplitude(&g->water, n);
                yy1 += 48 * 16 + camp.y;
                yy2 += 48 * 16 + camp.y;

                int xx1   = (n - 1) * 4 + camp.x;
                int xx2   = n * 4 + camp.x;
                int y_max = MAX(yy1, yy2);

                v2_i32 wp0 = {xx1, yy1};
                v2_i32 wp1 = {xx2, yy2};
                v2_i32 wp2 = {yy1 < yy2 ? xx1 : xx2, y_max};
                wctx.pat   = gfx_pattern_get(12);

                gfx_tri_fill(wctx, wp0, wp1, wp2);
                gfx_rec_fill(wctx, (rec_i32){xx1, y_max, 4, 100});
                wctx.pat = gfx_pattern_get(GFX_PATTERN_100);
                gfx_line_thick(wctx, (v2_i32){xx1, yy1}, (v2_i32){xx2, yy2}, 2);
        }
#endif

#if DRAW_WATER
        wctx.col = 1;
        wctx.dst = tex_get(TEXID_LAYER_1);
        gfx_tex_clr(wctx.dst);

        for (int n = 1; n < g->ocean.ocean.nparticles; n++) {
                int yy1   = ocean_amplitude(&g->ocean, n - 1) + camp.y;
                int yy2   = ocean_amplitude(&g->ocean, n + 0) + camp.y;
                int xx1   = (n - 1) * 4 + camp.x;
                int xx2   = (n + 0) * 4 + camp.x;
                int y_max = MAX(yy1, yy2);

                v2_i32 wp0 = {xx1, yy1};
                v2_i32 wp1 = {xx2, yy2};
                v2_i32 wp2 = {yy1 < yy2 ? xx1 : xx2, y_max};
                gfx_tri_fill(wctx, wp0, wp1, wp2);
                gfx_rec_fill(wctx, (rec_i32){xx1, y_max, 4, 200});
        }

        gfx_context_s ctx3 = wctx;
        ctx3.dst           = tex_get(0);
        ctx3.sprmode       = SPRITE_NXR;
        ctx3.src           = tex_get(TEXID_LAYER_1);
        gfx_sprite(ctx3, (v2_i32){0}, (rec_i32){0, 0, 400, 240}, 0);

        ctx3.sprmode = SPRITE_B_F;
        ctx3.pat     = gfx_pattern_get(12);
        gfx_sprite(ctx3, (v2_i32){0}, (rec_i32){0, 0, 400, 240}, 0);
#endif

#if 0 // some debug drawing
        for (pathnode_s *pn1 = &g->pathmover.nodes[0], *pn2 = &g->pathmover.nodes[1];
             pn1 && pn2; pn1 = pn2, pn2 = pn2->next) {
                gfx_line(pn1->p.x, pn1->p.y, pn2->p.x, pn2->p.y, 1);
                if (pn2 == &g->pathmover.nodes[0]) break;
        }
        v2_i32 pmpos = path_pos(&g->pathmover);
        gfx_rec_fill((rec_i32){pmpos.x, pmpos.y, 16, 16}, 1);
#endif

        if (!os_low_fps()) { // skip foreground drawing if FPS are low

                draw_foreground(g, camp);
        }

        draw_UI(g, camp);
}

void game_draw(game_s *g)
{
        gfx_tex_clr(tex_get(0));
#if DRAW_BENCHMARK
        draw_benchmark();
        return;
#endif

        switch (g->state) {
        case GAMESTATE_GAMEPLAY: {
                draw_gameplay(g);
        } break;
        case GAMESTATE_TITLE: {
                draw_title(g, &g->mainmenu);
        } break;
        }

#if 0 // test for flipped sprites
        gfx_context_s ctxtest = gfx_context_create(tex_get(0));
        ctxtest.src           = tex_get(TEXID_HERO);
        gfx_sprite(ctxtest, (v2_i32){(os_tick() % 200), (os_tick() * 3) % 200}, (rec_i32){10, 12, 200, 200}, SPRITE_FLIP_X);
#endif
        fading_s *f = &g->global_fade;
        if (fading_phase(f) != 0) {
                gfx_context_s ctx = gfx_context_create(tex_get(0));
                int           p   = GFX_PATTERN_0;
                switch (fading_phase(f)) {
                case FADE_PHASE_OUT:
                        p = fading_lerp(f, GFX_PATTERN_0, GFX_PATTERN_100);
                        break;
                case FADE_PHASE_PAUSE:
                        p = GFX_PATTERN_100;
                        break;
                case FADE_PHASE_IN:
                        p = fading_lerp(f, GFX_PATTERN_100, GFX_PATTERN_0);
                        break;
                }

                ctx.pat = gfx_pattern_get(p);
                ctx.col = 1;
                gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240});
        }
}