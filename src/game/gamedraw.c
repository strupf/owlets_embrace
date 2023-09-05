// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "draw.h"
#include "game.h"
#include "os/os.h"

static void merge_layer(tex_s screen, tex_s layer)
{
        int l = screen.w_word * screen.h;
        ASSERT((l & 3) == 0);
        ASSERT(screen.w_word == layer.w_word && screen.h == layer.h);
        ASSERT(layer.mk && layer.px);
        u32 *sp = (u32 *)screen.px;
        u32 *lp = (u32 *)layer.px;
        u32 *lm = (u32 *)layer.mk;

        for (int n = 0; n < l; n += 4, sp += 4, lm += 4, lp += 4) {
                sp[0] = (sp[0] & ~lm[0]) | (lp[0] & lm[0]);
                sp[1] = (sp[1] & ~lm[1]) | (lp[1] & lm[1]);
                sp[2] = (sp[2] & ~lm[2]) | (lp[2] & lm[2]);
                sp[3] = (sp[3] & ~lm[3]) | (lp[3] & lm[3]);
        }
}

static void draw_textbox(gfx_context_s ctx, textbox_s *tb)
{
        int state = textbox_state(tb);
        switch (state) {
        case TEXTBOX_STATE_OPENING: {
                int patternID = lerp_i32(GFX_PATTERN_0,
                                         GFX_PATTERN_100,
                                         tb->animationticks,
                                         TEXTBOX_ANIMATION_TICKS);
                ctx.pat       = gfx_pattern_get(patternID);
        } break;
        case TEXTBOX_STATE_CLOSING: {
                int patternID = lerp_i32(GFX_PATTERN_100,
                                         GFX_PATTERN_0,
                                         tb->animationticks,
                                         TEXTBOX_ANIMATION_TICKS);
                ctx.pat       = gfx_pattern_get(patternID);
        } break;
        }

        fnt_s   font = fnt_get(FNTID_DEFAULT);
        rec_i32 r    = (rec_i32){0, 0, 400, 240};
        ctx.src      = tex_get(TEXID_TEXTBOX);
        ctx.offset.x = 0;
        ctx.offset.y = 0;
        gfx_sprite(ctx, (v2_i32){0, 0}, r, 0);

        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(ctx, &font, line->chars, line->n_shown, (v2_i32){20, 150 + 21 * l});
        }

        if (state == TEXTBOX_STATE_WRITING) return;

        ctx.col = 1;
        if (tb->n_choices) {
                rec_i32 rr               = (rec_i32){0, 128, 200, 128};
                ctx.src                  = tex_get(TEXID_TEXTBOX);
                // gfx_sprite(ctx, (v2_i32){243, 48}, rr, 0);
                //  draw choices if any
                static const int spacing = 21;
                for (int n = 0; n < tb->n_choices; n++) {
                        textboxchoice_s *tc = &tb->choices[n];

                        gfx_text_glyphs(ctx, &font, tc->label, tc->labellen,
                                        (v2_i32){264, 70 + spacing * n});
                }

                gfx_rec_fill(ctx, (rec_i32){250, 73 + spacing * tb->cur_choice, 10, 10});
        } else {
                // "next page" animated marker in corner
                tb->page_animation_state += 10000;
                tb->page_animation_state &= (Q16_ANGLE_TURN - 1);
                int yy = sin_q16(tb->page_animation_state) >> 14;
                gfx_rec_fill(ctx, (rec_i32){370, 205 + yy, 10, 10});
        }
}

static void draw_background(gfx_context_s ctx, backforeground_s *bg)
{
        ctx.pat      = g_gfx_patterns[GFX_PATTERN_31];
        ctx.src      = tex_get(TEXID_CLOUDS);
        ctx.offset.x = 0;
        ctx.offset.y = 0;
        for (int n = 0; n < bg->nclouds; n++) {
                cloudbg_s c   = bg->clouds[n];
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

static void draw_foreground(gfx_context_s ctx, backforeground_s *bg)
{
        // wind animation
        ctx.pat = g_gfx_patterns[GFX_PATTERN_44];
        ctx.col = 1;
        for (int n = 0; n < bg->nparticles; n++) {
                particlebg_s p  = bg->particles[n];
                v2_i32       p1 = v2_shr(p.pos[p.n], 8);

                for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
                        int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
                        v2_i32 p2 = v2_shr(p.pos[k], 8);
                        gfx_line(ctx, p1, p2);
                        p1 = p2;
                }
        }
}

static void draw_tiles(gfx_context_s ctx, game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, int l)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);
        TIMING_BEGIN(TIMING_DRAW_TILES);
        ctx.src = tex_get(TEXID_TILESET);
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                v2_i32  pos = {(x << 4), (y << 4)};
                rtile_s rt  = g->rtiles[x + y * g->tiles_x];
                int     ID  = g_tileIDs[rt.ID[l]]; // tile animations
                if (ID == TILEID_NULL) continue;
                int tx, ty;
                tileID_decode(ID, &tx, &ty);
                rec_i32 r = {tx << 4, ty << 4, 16, 16};
                gfx_sprite(ctx, pos, r, 0);
        }
        TIMING_END();
}

/* maps texture generated above onto a cylinder
 * 1) point on circle
 * 2) calc angle on circle
 * 3) calc radians on circle, getting pos in image texture
 *    image is "wrapped" onto cylinder
 */
static void item_selection_redraw(game_s *g, hero_s *h)
{
        // interpolator based on crank position
        int ii = -((ITEM_SIZE * os_inp_crank()) >> 16);
        if (os_inp_crank() >= 0x8000) {
                ii += ITEM_SIZE;
        }

        int itemIDs[3] = {h->selected_item_prev,
                          h->selected_item,
                          h->selected_item_next};

        gfx_context_s ctx = gfx_context_create(g->itemselection_cache);
        ctx.src           = tex_get(TEXID_ITEMS);
        gfx_tex_clr(g->itemselection_cache);

        for (int y = -ITEM_BARREL_R; y <= +ITEM_BARREL_R; y++) {
                int     a_q16   = (y << 16) / ITEM_BARREL_R;
                int     arccos  = (acos_q16(a_q16) * ITEM_SIZE) >> (16 + 1);
                int     loc     = arccos + ITEM_SIZE - ii;
                int     itemi   = loc / ITEM_SIZE;
                int     yy      = ITEM_SIZE * itemIDs[itemi] + loc % ITEM_SIZE;
                int     uu      = ITEM_BARREL_R - y + ITEM_Y_OFFS;
                rec_i32 itemrow = {ITEM_SIZE, yy, ITEM_SIZE, 1};
                gfx_sprite(ctx, (v2_i32){ITEM_X_OFFS, uu}, itemrow, 0);
        }

        gfx_sprite(ctx, (v2_i32){0, 0}, (rec_i32){64, 0, 64, 64}, 0);
}

static void draw_item_selection(gfx_context_s ctx, game_s *g)
{
        hero_s *h = &g->hero;
        if (h->aquired_items == 0) return;
        if (g->itemselection_dirty) {
                item_selection_redraw(g, h);
                g->itemselection_dirty = 0;
        }
        ctx.src      = g->itemselection_cache;
        ctx.offset.x = 0;
        ctx.offset.y = 0;
        gfx_sprite(ctx,
                   (v2_i32){400 - ITEM_FRAME_SIZE,
                            240 - ITEM_FRAME_SIZE},
                   (rec_i32){0, 0,
                             ITEM_FRAME_SIZE,
                             ITEM_FRAME_SIZE},
                   SPRITE_CPY);
}

static void draw_transition(gfx_context_s ctx, game_s *g)
{
        transition_s *t = &g->transition;

        int ticks     = TRANSITION_FADE_TICKS - t->ticks;
        int patternID = lerp_fast_i32(GFX_PATTERN_100,
                                      GFX_PATTERN_0,
                                      MAX(ticks, 0),
                                      TRANSITION_FADE_TICKS);
        ctx.pat       = gfx_pattern_get(patternID);
        ctx.col       = 1;
        ctx.offset.x  = 0;
        ctx.offset.y  = 0;
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240});
}

static void draw_rope(gfx_context_s ctx, rope_s *r)
{
        ctx.col = 1;
        ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                gfx_line_thick(ctx, r1->p, r2->p, 3);
        }

        ctx.pat = gfx_pattern_get(GFX_PATTERN_69);
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                gfx_line_thick(ctx, r1->p, r2->p, 1);
        }
}

static void draw_particles(gfx_context_s ctx, game_s *g)
{
        ctx.src           = tex_get(TEXID_PARTICLE);
        rec_i32 rparticle = {0, 0, 4, 4};
        for (int n = 0; n < g->n_particles; n++) {
                particle_s *p = &g->particles[n];
                gfx_sprite(ctx, v2_shr(p->p_q8, 8), rparticle, 0);
        }
}

static void ___animation()
{
        // animation...
        static int hookpos  = -1500;
        static int slomo    = 0;
        static int wasslomo = 0;
        static int hdir     = 1;
        if (debug_inp_space()) {
                wasslomo = 0;
                hookpos  = -1500;
                slomo    = 0;
                hdir     = 1;
                return;
        }
        tex_s thook = tex_get(TEXID_HOOK);
        if (slomo > 0) {

                slomo += hdir;
                if (slomo == 20) {
                        hdir = -1;
                }
                int ss = slomo;
                int vv = ss * ss;
                hookpos += MIN(MAX((20 - ((vv * 20) / (20 * 20))) / 8, 1), 4);
        } else {
                hookpos += 20;
        }

        int yyy = (os_tick() / (slomo ? 3000 : 2)) % 6;
        if (!wasslomo && hookpos >= 400) {
                wasslomo = 1;
                slomo    = 1;
        }

        // gfx_sprite_fast(thook, (v2_i32){hookpos - 512, 50}, (rec_i32){0, yyy * 128, 512, 128});
        for (int n = 1; n < 10; n++) {
                // gfx_sprite_fast(thook, (v2_i32){hookpos - 512 - n * 84, 50}, (rec_i32){0, yyy * 128, 84, 128});
        }
}

#include <math.h>
static void ___titlescreen()
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.pat           = gfx_pattern_get(GFX_PATTERN_100);
        ctx.src           = tex_get(TEXID_TITLESCREEN);
        gfx_sprite(ctx, (v2_i32){(400 - 256) / 2 - 10, 30}, (rec_i32){0, 0, 256, 128}, 0);

        // ctx.pat = gfx_pattern_get(3);
        gfx_sprite(ctx, (v2_i32){0, 110}, (rec_i32){0, 64 * 3, 512, 128}, 0);
        ctx.pat = gfx_pattern_get(GFX_PATTERN_100);

        static float time;
        static float yg = -300.f;

        time += 0.05f;
        float xsin = sinf(time);
        time += (1.f - fabs(xsin)) * 0.05f;
        xsin     = sinf(time);
        float xx = xsin * 20.f;
        yg += 0.5f;
        float yy    = yg + (cosf(time * 2.f)) * 8.f;
        int   frame = (int)(((xsin + 1.f) / 2.f) * 6.9f);

        gfx_sprite(ctx, (v2_i32){(int)xx + 315, (int)yy}, (rec_i32){64 * frame, 128, 64, 64}, 0);
}

void game_draw(game_s *g)
{
#if 0
        ___titlescreen();
        return;
#endif
#if 0
        ___animation();
        return;
#endif

        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.pat           = gfx_pattern_get(GFX_PATTERN_100);

        v2_i32  camp = {-(g->cam.pos.x - g->cam.wh),
                        -(g->cam.pos.y - g->cam.hh)};
        rec_i32 camr = {-camp.x, -camp.y, g->cam.w, g->cam.h};
        ctx.offset   = camp;

        ctx.dst = tex_get(TEXID_LAYER_1);
        gfx_tex_clr(tex_get(TEXID_LAYER_1));
        draw_background(ctx, &g->backforeground);
        merge_layer(tex_get(0), tex_get(TEXID_LAYER_1));
        ctx.dst = tex_get(0);

        i32 x1, y1, x2, y2;
        tilegrid_bounds_rec(g, camr, &x1, &y1, &x2, &y2);

        ctx.pat     = gfx_pattern_get(GFX_PATTERN_50);
        ctx.sprmode = 0;
        draw_tiles(ctx, g, x1, y1, x2, y2, TILE_LAYER_BG);

        ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        draw_tiles(ctx, g, x1, y1, x2, y2, TILE_LAYER_MAIN);
        if (objhandle_is_valid(g->hero.hook))
                draw_rope(ctx, &g->hero.rope);

        obj_listc_s oalive = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < oalive.n; n++) {
                obj_s *o = oalive.o[n];

                switch (o->ID) {
                case 1: {
                        ctx.src = tex_get(TEXID_SOLID);
                        gfx_sprite(ctx, o->pos, (rec_i32){0, 0, 64, 48}, 0);
                } break;
                case 2: {
                        ctx.src = tex_get(TEXID_SOLID);
                        int tx  = 1;
                        int ty  = 96 + (o->facing == -1 ? 16 : 0);
                        if (o->vel_q8.y < -150) tx = 0;
                        if (o->vel_q8.y > +150) tx = 2;
                        gfx_sprite(ctx, o->pos, (rec_i32){tx * 16, ty, 16, 16}, 0);
                } break;
                case 3: {

                        if (g->hero.swordticks > 0) {
                                rec_i32 hero_sword_hitbox(obj_s * o, hero_s * h);
                                rec_i32 swordbox = hero_sword_hitbox(o, &g->hero);
                                gfx_rec_fill(ctx, swordbox);
                        }

                        // just got hit flashing
                        if (o->invincibleticks > 0 && (o->invincibleticks & 15) < 8)
                                break;
                        v2_i32 pos = o->pos;
                        ctx.src    = tex_get(TEXID_HERO);
                        pos.x -= 22;
                        pos.y = pos.y + o->h - 64;

                        int animy = 0;
                        if (o->vel_q8.x == 0 && game_area_blocked(g, obj_rec_bottom(o)) &&
                            (o->animation % 400) < 200) {
                                animy += 64;
                                o->animframe = 0;
                        }

                        int flags = o->facing == 1 ? 0 : SPRITE_FLIP_X;
                        gfx_sprite(ctx, pos, (rec_i32){o->animframe * 64, 64 * 2, 64, 64}, flags);
                        gfx_sprite(ctx, pos, (rec_i32){o->animframe * 64, animy, 64, 64}, flags);
                } break;
                case 5: {
                        v2_i32 pos = o->pos;
                        ctx.src    = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite(ctx, pos, (rec_i32){64, 96, 64, 64}, 0);
                } break;
                case 6: {
                        v2_i32 pos = o->pos;
                        ctx.src    = tex_get(TEXID_SOLID);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite(ctx, pos, (rec_i32){0, 144, 32, 32}, 0);
                } break;
                default: {
                        gfx_rec_fill(ctx, obj_aabb(o));
                } break;
                }

                for (int i = 0; i < o->nspriteanim; i++) {
                        obj_sprite_anim_s *sa   = &o->spriteanim[i];
                        texregion_s        texr = sprite_anim_get(&sa->a);
                        v2_i32             pos  = v2_add(o->pos, sa->offset);
                        ctx.src                 = texr.t;
                        gfx_sprite(ctx, pos, texr.r, 0);
                }
        }

        if (!os_low_fps()) { // skip particle drawing if FPS are low
                draw_particles(ctx, g);
        }

#if 0 // some debug drawing
        for (int n = 1; n < g->water.nparticles; n++) {
                i32 yy1 = water_amplitude(&g->water, n - 1);
                i32 yy2 = water_amplitude(&g->water, n);
                yy1 += 100 + camp.y;
                yy2 += 100 + camp.y;

                int xx1   = (n - 1) * 4 + camp.x;
                int xx2   = n * 4 + camp.x;
                int y_max = MAX(yy1, yy2);

                v2_i32 wp0 = {xx1, yy1};
                v2_i32 wp1 = {xx2, yy2};
                v2_i32 wp2 = {yy1 < yy2 ? xx1 : xx2, y_max};
                gfx_set_pattern(gfx_pattern_get(14));
                gfx_tri_fill(wp0, wp1, wp2, 1);
                gfx_rec_fill((rec_i32){xx1, y_max, 4, 100}, 1);
                gfx_reset_pattern();
                gfx_line(xx1, yy1, xx2, yy2, 1);
                gfx_line(xx1, yy1 + 1, xx2, yy2 + 1, 1);
                gfx_line(xx1, yy1 + 2, xx2, yy2 + 2, 1);
        }

        for (pathnode_s *pn1 = &g->pathmover.nodes[0], *pn2 = &g->pathmover.nodes[1];
             pn1 && pn2; pn1 = pn2, pn2 = pn2->next) {
                gfx_line(pn1->p.x, pn1->p.y, pn2->p.x, pn2->p.y, 1);
                if (pn2 == &g->pathmover.nodes[0]) break;
        }
        v2_i32 pmpos = path_pos(&g->pathmover);
        gfx_rec_fill((rec_i32){pmpos.x, pmpos.y, 16, 16}, 1);
#endif

        if (!os_low_fps()) { // skip foreground drawing if FPS are low
                draw_foreground(ctx, &g->backforeground);
        }
        draw_item_selection(ctx, g);

        obj_s *ohero;
        if (g->hero.caninteract && try_obj_from_handle(g->hero.obj, &ohero)) {
                v2_i32 heroc        = obj_aabb_center(ohero);
                obj_s *interactable = interactable_closest(g, heroc);

                if (interactable) {
                        v2_i32 pp = obj_aabb_center(interactable);
                        pp.y -= 32;
                        pp.x -= 8;
                        ctx.src = tex_get(TEXID_INPUT_EL);
                        int yy  = (os_tick() % 60) < 30 ? 32 : 0;
                        gfx_sprite(ctx, pp, (rec_i32){32 * 0, yy, 32, 32}, 0);
                }
        }

        if (g->transition.phase)
                draw_transition(ctx, g);
        if (textbox_state(&g->textbox) != TEXTBOX_STATE_INACTIVE)
                draw_textbox(ctx, &g->textbox);

        if (g->areaname_display_ticks > 0) {
                fnt_s areafont  = fnt_get(FNTID_DEFAULT);
                int   fadeticks = MIN(g->areaname_display_ticks, AREA_NAME_FADE_TICKS);
                int   patternID = lerp_fast_i32(GFX_PATTERN_0,
                                                GFX_PATTERN_100,
                                                fadeticks,
                                                AREA_NAME_FADE_TICKS);
                ctx.offset.x    = 0;
                ctx.offset.y    = 0;
                ctx.pat         = gfx_pattern_get(patternID);
                gfx_text_ascii(ctx, &areafont, g->areaname, (v2_i32){10, 10});
                ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        }
}