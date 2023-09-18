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

        for (int n = 0; n < l; n += 4, sp += 4, lm += 4, lp += 4) {
                sp[0] = (sp[0] & ~lm[0]) | (lp[0] & lm[0]);
                sp[1] = (sp[1] & ~lm[1]) | (lp[1] & lm[1]);
                sp[2] = (sp[2] & ~lm[2]) | (lp[2] & lm[2]);
                sp[3] = (sp[3] & ~lm[3]) | (lp[3] & lm[3]);
        }
}

void draw_background(backforeground_s *bg, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(TEXID_LAYER_1));
        gfx_tex_clr(ctx.dst);
        ctx.pat = g_gfx_patterns[GFX_PATTERN_31];
        ctx.src = tex_get(TEXID_CLOUDS);
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

void draw_foreground(backforeground_s *bg, v2_i32 camp)
{
        // wind animation
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.pat           = g_gfx_patterns[GFX_PATTERN_44];
        ctx.col           = 1;
        for (int n = 0; n < bg->nparticles; n++) {
                particlebg_s p  = bg->particles[n];
                v2_i32       p1 = v2_add(camp, v2_shr(p.pos[p.n], 8));

                for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
                        int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
                        v2_i32 p2 = v2_add(v2_shr(p.pos[k], 8), camp);
                        gfx_line(ctx, p1, p2);
                        p1 = p2;
                }
        }
}

void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, int l, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
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
                gfx_sprite_tile16(ctx, v2_add(pos, camp), r, 0);
        }
        TIMING_END();
}

void draw_transition(game_s *g)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        transition_s *t   = &g->transition;

        int ticks     = TRANSITION_FADE_TICKS - t->ticks;
        int patternID = lerp_fast_i32(GFX_PATTERN_100,
                                      GFX_PATTERN_0,
                                      MAX(ticks, 0),
                                      TRANSITION_FADE_TICKS);
        ctx.pat       = gfx_pattern_get(patternID);
        ctx.col       = 1;
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240});
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
        ctx.src           = tex_get(TEXID_PARTICLE);
        rec_i32 rparticle = {0, 0, 4, 4};
        for (int n = 0; n < g->n_particles; n++) {
                particle_s *p = &g->particles[n];
                gfx_sprite(ctx, v2_add(v2_shr(p->p_q8, 8), camp), rparticle, 0);
        }
}

static void draw_textbox(textbox_s *tb, v2_i32 camp)
{
        gfx_context_s ctx   = gfx_context_create(tex_get(0));
        int           state = textbox_state(tb);
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

        fnt_s font  = fnt_get(FNTID_DEFAULT);
        int   textx = 0;
        int   texty = 0;

        // TODO
        tb->type = 0;

        switch (tb->type) {
        case TEXTBOX_TYPE_STATIC_BOX: {
                rec_i32 r = (rec_i32){0, 0, 400, 240};
                ctx.src   = tex_get(TEXID_TEXTBOX);
                gfx_sprite(ctx, (v2_i32){0, 0}, r, 0);
                textx = 20;
                texty = 146;
        } break;
        case TEXTBOX_TYPE_SPEECH_BUBBLE: {
                textx     = 50;
                texty     = 10;
                ctx.col   = 0;
                rec_i32 r = (rec_i32){0, 240, 400, 240};
                ctx.src   = tex_get(TEXID_TEXTBOX);
                gfx_sprite(ctx, (v2_i32){0, 0}, r, 0);
                ctx.col = 1;
                v2_i32 tr0;
                v2_i32 tr1;
                v2_i32 tr2;
                gfx_tri_fill(ctx, (v2_i32){10, 10}, (v2_i32){40, 0}, (v2_i32){0, 50});
        } break;
        }
        ctx.col = 1;

        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(ctx, &font, line->chars, line->n_shown, (v2_i32){textx, texty + 21 * l});
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

/* maps texture generated above onto a cylinder
 * 1) point on circle
 * 2) calc angle on circle
 * 3) calc radians on circle, getting pos in image texture
 *    image is "wrapped" onto cylinder
 */
static void item_selection_redraw(hero_s *h)
{
        // interpolator based on crank position
        int ii = -((ITEM_SIZE * os_inp_crank()) >> 16);
        if (os_inp_crank() >= 0x8000) {
                ii += ITEM_SIZE;
        }

        int itemIDs[3] = {h->selected_item_prev,
                          h->selected_item,
                          h->selected_item_next};

        tex_s         texcache = tex_get(TEXID_ITEM_SELECT_CACHE);
        gfx_context_s ctx      = gfx_context_create(texcache);
        ctx.src                = tex_get(TEXID_ITEMS);
        gfx_tex_clr(texcache);

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

void draw_game_UI(game_s *g, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        hero_s *h         = &g->hero;

        if (h->aquired_items > 0) {
                if (h->itemselection_dirty) {
                        item_selection_redraw(h);
                        h->itemselection_dirty = 0;
                }
                ctx.src = tex_get(TEXID_ITEM_SELECT_CACHE);
                gfx_sprite(ctx,
                           (v2_i32){400 - ITEM_FRAME_SIZE + 16,
                                    -16},
                           (rec_i32){0, 0,
                                     ITEM_FRAME_SIZE,
                                     ITEM_FRAME_SIZE},
                           SPRITE_CPY);
        }

        obj_s *ohero;
        if (g->hero.caninteract && try_obj_from_handle(h->obj, &ohero)) {
                v2_i32 heroc        = obj_aabb_center(ohero);
                obj_s *interactable = interactable_closest(g, heroc);

                if (interactable) {
                        v2_i32 pp = obj_aabb_center(interactable);

                        pp = v2_add(pp, camp);
                        pp.y -= 32;
                        pp.x -= 8;
                        ctx.src = tex_get(TEXID_INPUT_EL);
                        int yy  = (os_tick() % 60) < 30 ? 32 : 0;
                        gfx_sprite(ctx, pp, (rec_i32){32 * 0, yy, 32, 32}, 0);
                }
        }

        if (textbox_state(&g->textbox) != TEXTBOX_STATE_INACTIVE)
                draw_textbox(&g->textbox, camp);

        if (g->areaname_display_ticks > 0) {
                fnt_s areafont  = fnt_get(FNTID_DEFAULT);
                int   fadeticks = MIN(g->areaname_display_ticks, AREA_NAME_FADE_TICKS);
                int   patternID = lerp_fast_i32(GFX_PATTERN_0,
                                                GFX_PATTERN_100,
                                                fadeticks,
                                                AREA_NAME_FADE_TICKS);
                ctx.pat         = gfx_pattern_get(patternID);
                gfx_text_ascii(ctx, &areafont, g->areaname, (v2_i32){10, 10});
                ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        }
}

#define DRAW_WATER 0

static void draw_gameplay(game_s *g)
{

        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        ctx.pat           = gfx_pattern_get(GFX_PATTERN_100);

        v2_i32  camp = {-(g->cam.pos.x - g->cam.wh),
                        -(g->cam.pos.y - g->cam.hh)};
        rec_i32 camr = {-camp.x, -camp.y, g->cam.w, g->cam.h};

        draw_background(&g->backforeground, camp);

        ctx.src = tex_get(TEXID_HERO);
        // ctx.dst =
        // gfx_sprite(ctx, (v2_i32){camp.x / 3 + 450, 110}, (rec_i32){368, 320, 144, 200}, 0);

        merge_layer(tex_get(0), tex_get(TEXID_LAYER_1));

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
        tilegrid_bounds_rec(g, camr, &x1, &y1, &x2, &y2);
        draw_tiles(g, x1, y1, x2, y2, TILE_LAYER_BG, camp);
        draw_tiles(g, x1, y1, x2, y2, TILE_LAYER_MAIN, camp);

        if (objhandle_is_valid(g->hero.hook))
                draw_rope(&g->hero.rope, camp);

        objset_sort(&(g->objbuckets[OBJ_BUCKET_RENDERABLE].set),
                    obj_cmp_renderable);
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
                case 1: {
                        ctx.src = tex_get(TEXID_SOLID);
                        gfx_sprite(ctx, v2_add(o->pos, camp), (rec_i32){0, 0, 64, 48}, 0);
                } break;
                case 2: {
                        ctx.src   = tex_get(TEXID_SOLID);
                        int   tx  = 0;
                        int   ty  = 80;
                        float ang = atan2f(-o->vel_q8.y, o->vel_q8.x);
                        pos.x -= 16;
                        pos.y -= 8;
                        gfx_sprite_rotated_(ctx, pos,
                                            (rec_i32){tx, ty, 32, 32},
                                            (v2_i32){16, 16}, ang);
                } break;
                case 3: {

                        if (g->hero.swordticks > 0) {
                                rec_i32 hero_sword_hitbox(obj_s * o, hero_s * h);
                                rec_i32 swordbox = hero_sword_hitbox(o, &g->hero);
                                swordbox.x += camp.x;
                                swordbox.y += camp.y;
                                gfx_rec_fill(ctx, swordbox);
                        }

                        // just got hit flashing
                        if (o->invincibleticks > 0 && (o->invincibleticks & 15) < 8)
                                break;

                        ctx.src = tex_get(TEXID_HERO);
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
                        ctx.src = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite(ctx, pos, (rec_i32){64, 96, 64, 64}, 0);
                } break;
                case 6: {
                        ctx.src = tex_get(TEXID_SOLID);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite(ctx, pos, (rec_i32){0, 144, 32, 32}, 0);
                } break;
                case 10: {
                        ctx.src = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y -= 90;
                        v2_i32 boatc   = obj_aabb_center(o);
                        int    oceany1 = ocean_amplitude(&g->ocean, boatc.x - 16);
                        int    oceany2 = ocean_amplitude(&g->ocean, boatc.x + 16);
                        float  ang     = atan2f(oceany2 - oceany1, 32.f * 8.f);

                        int texy = (os_tick() % 60 < 30 ? 64 : 64 + 128);

                        gfx_sprite_rotated_(ctx, pos, (rec_i32){288, texy, 128, 128},
                                            (v2_i32){64, 100}, ang);
                        // gfx_sprite(ctx, pos, (rec_i32){288, 64, 128, 128}, 0);
                        rec_i32 aabb = obj_aabb(o);
                        aabb         = translate_rec(aabb, camp);
                        // gfx_rec_fill(ctx, aabb);
                } break;
                case 11: {
                        if (o->state == 2) break;
                        ctx.src = tex_get(TEXID_SOLID);
                        if (o->state == 1) {
                                pos.x += rng_range(-1, +1);
                        }

                        gfx_sprite(ctx, pos, (rec_i32){80, 0, 16, 16}, 0);
                } break;
                case 13: {
                        ctx.src = tex_get(TEXID_HERO);
                        int yyy = os_tick() % 60 < 30 ? 304 : 304 + 32;
                        pos.y -= 12;
                        gfx_sprite(ctx, pos, (rec_i32){yyy, 0, 32, 32}, 0);
                } break;
                default: {
                        rec_i32 aabb = translate_rec(obj_aabb(o), camp);
                        gfx_rec_fill(ctx, aabb);
                } break;
                }

                for (int i = 0; i < o->nspriteanim; i++) {
                        obj_sprite_anim_s *sa   = &o->spriteanim[i];
                        texregion_s        texr = sprite_anim_get(&sa->a);
                        pos                     = v2_add(pos, sa->offset);
                        ctx.src                 = texr.t;
                        gfx_sprite(ctx, pos, texr.r, 0);
                }
        }

        if (!os_low_fps()) { // skip particle drawing if FPS are low
                draw_particles(g, camp);
        }

        gfx_context_s wctx = gfx_context_create(tex_get(0));
        wctx.col           = 1;
        if (debug_inp_space()) {
                water_impact(&g->water, 100, 20, 30000);
        }
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

        if (!os_low_fps() || 1) { // skip foreground drawing if FPS are low
                draw_foreground(&g->backforeground, camp);
        }

        draw_game_UI(g, camp);

        if (g->transition.phase)
                draw_transition(g);
}

static void draw_title(game_s *g)
{
}

void game_draw(game_s *g)
{
        switch (g->state) {
        case GAMESTATE_GAMEPLAY: {
                draw_gameplay(g);
        } break;
        case GAMESTATE_TITLE: {
                draw_title(g);
        } break;
        }
}