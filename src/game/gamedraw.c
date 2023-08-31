// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "os/os.h"

static void draw_textbox(textbox_s *tb)
{
        if (tb->closeticks > 0) {
                gfx_pattern_s tpat;

                int npat = (TEXTBOX_CLOSE_TICKS - tb->closeticks) /
                           TEXBOX_CLOSE_DIV;
                switch (npat) {
                case 0:
                        tpat = gfx_pattern_set_8x8(B8(11111111),
                                                   B8(10101010),
                                                   B8(11111111),
                                                   B8(10101010),
                                                   B8(11111111),
                                                   B8(10101010),
                                                   B8(11111111),
                                                   B8(10101010));
                        break;
                case 1:
                        tpat = gfx_pattern_set_8x8(B8(01010101),
                                                   B8(10101010),
                                                   B8(01010101),
                                                   B8(10101010),
                                                   B8(01010101),
                                                   B8(10101010),
                                                   B8(01010101),
                                                   B8(10101010));
                        break;
                case 2:
                        tpat = gfx_pattern_set_8x8(B8(00000000),
                                                   B8(01010101),
                                                   B8(00000000),
                                                   B8(01010101),
                                                   B8(00000000),
                                                   B8(01010101),
                                                   B8(00000000),
                                                   B8(01010101));
                        break;
                case 3:
                        tpat = gfx_pattern_set_8x8(B8(00000000),
                                                   B8(01000100),
                                                   B8(00000000),
                                                   B8(00000000),
                                                   B8(00000000),
                                                   B8(01000100),
                                                   B8(00000000),
                                                   B8(00000000));
                        break;
                }

                gfx_set_pattern(tpat);
        }
        fnt_s   font = fnt_get(FNTID_DEFAULT);
        rec_i32 r    = (rec_i32){0, 0, 400, 128};
        gfx_sprite_fast(tex_get(TEXID_TEXTBOX), (v2_i32){0, 128}, r);

        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(&font, line->chars, line->n_shown, 20, 150 + 21 * l);
        }

        if (!tb->shows_all) {
                gfx_reset_pattern();
                return;
        }

        if (tb->n_choices) {
                rec_i32 rr = (rec_i32){0, 128, 200, 128};
                gfx_sprite_fast(tex_get(TEXID_TEXTBOX), (v2_i32){243, 48}, rr);
                // draw choices if any
                static const int spacing = 21;
                for (int n = 0; n < tb->n_choices; n++) {
                        textboxchoice_s *tc = &tb->choices[n];

                        gfx_text_glyphs(&font, tc->label, tc->labellen,
                                        264, 70 + spacing * n);
                }
                gfx_rec_fill((rec_i32){250, 73 + spacing * tb->cur_choice, 10, 10}, 1);
        } else {
                // "next page" animated marker in corner
                tb->page_animation_state += 10000;
                tb->page_animation_state &= (Q16_ANGLE_TURN - 1);
                int yy = sin_q16(tb->page_animation_state) >> 14;
                gfx_rec_fill((rec_i32){370, 205 + yy, 10, 10}, 1);
        }
        gfx_reset_pattern();
}

static void draw_background(backforeground_s *bg, v2_i32 camp)
{
        gfx_set_pattern(g_gfx_patterns[5]);
        tex_s tclouds = tex_get(TEXID_CLOUDS);
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

                gfx_sprite_fast(tclouds, pos, r);
        }
        gfx_reset_pattern();
}

static void wind_particle_line(v2_i32 p0, v2_i32 p1)
{

        int dx = +ABS(p1.x - p0.x), sx = p0.x < p1.x ? 1 : -1;
        int dy = -ABS(p1.y - p0.y), sy = p0.y < p1.y ? 1 : -1;
        int er = dx + dy;
        int xi = p0.x;
        int yi = p0.y;
        while (1) {
                gfx_px(xi, yi, 1);
                if (xi == p1.x && yi == p1.y) break;
                int e2 = er * 2;
                if (e2 >= dy) er += dy, xi += sx;
                if (e2 <= dx) er += dx, yi += sy;
        }
}

static void draw_foreground(backforeground_s *bg, v2_i32 camp)
{
        gfx_set_pattern(g_gfx_patterns[7]);
        // wind animation
        for (int n = 0; n < bg->nparticles; n++) {
                particlebg_s p = bg->particles[n];

                v2_i32 p1 = v2_add(v2_shr(p.pos[p.n], 8), camp);
                for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
                        int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
                        v2_i32 p2 = v2_add(v2_shr(p.pos[k], 8), camp);

                        wind_particle_line(p1, p2);
                        p1 = p2;
                }
        }
        gfx_reset_pattern();
}

static void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, v2_i32 camp)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);
        TIMING_BEGIN(TIMING_DRAW_TILES);
        tex_s tileset = tex_get(TEXID_TILESET);
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                rtile_s rt = g->rtiles[x + y * g->tiles_x];
                int     ID = g_tileIDs[rt.ID];
                if (ID == TILEID_NULL) continue;
                int tx, ty;
                tileID_decode(ID, &tx, &ty);
                v2_i32  pos = {(x << 4) + camp.x, (y << 4) + camp.y};
                rec_i32 r   = {tx << 4, ty << 4, 16, 16};
                gfx_sprite_fast(tileset, pos, r);
        }
        TIMING_END();
}

/* maps texture generated above onto a cylinder
 * 1) point on circle
 * 2) calc angle on circle
 * 3) calc radians on circle, getting pos in image texture
 *    image is "wrapped" onto cylinder
 */
static void draw_item_selection(game_s *g)
{
        hero_s *h = &g->hero;
        if (h->aquired_items == 0) return;

        os_spmem_push();

        tex_s tt  = {0};
        tt.w      = 32;
        tt.h      = 128;
        tt.w_byte = tt.w / 8;
        tt.w_word = tt.w / 32;
        tt.px     = os_spmem_allocz(sizeof(u8) * tt.w_byte * tt.h);
        tt.mk     = NULL;

        int ii = -((32 * os_inp_crank()) >> 16);
        if (os_inp_crank() >= 0x8000) {
                ii += 32;
        }

        tex_s   titem   = tex_get(TEXID_ITEMS);
        rec_i32 itemsrc = {32, 0, 32, 32};

        gfx_draw_to(tt);
        {
                itemsrc.y = h->selected_item_prev * 32;
                gfx_sprite_fast(titem, (v2_i32){0, ii}, itemsrc);
                itemsrc.y = h->selected_item * 32;
                gfx_sprite_fast(titem, (v2_i32){0, ii + 32}, itemsrc);
                itemsrc.y = h->selected_item_next * 32;
                gfx_sprite_fast(titem, (v2_i32){0, ii + 64}, itemsrc);
        }
        gfx_draw_to_ID(0);

        v2_i32  itemframepos = {400 - 32 - 16, -8};
        rec_i32 itemframerec = {64, 0, 64, 64};
        for (int y = -16; y <= +16; y++) {
                float l = acosf((float)y / 16.f) * 16.f;
                gfx_sprite_fast(tt, (v2_i32){400 - 32, 8 + (16 - y)},
                                (rec_i32){0, 24 + (int)l, 32, 1}); // y pos add is kinda magic number
        }
        gfx_sprite_mode(titem, itemframepos, itemframerec, GFX_SPRITE_COPY);
        os_spmem_pop();
#if 0
        char cc[2] = {0};
        cc[0]      = itemID1 + '0';
        fnt_s font = fnt_get(FNTID_DEFAULT);
        gfx_text_ascii(&font, cc, 400 - 32 - 16, 8);
#endif
}

static void draw_transition(game_s *g)
{
        transition_s *t = &g->transition;

        int f = lerp_i32(0, NUM_GFX_PATTERN - 1, t->ticks, TRANSITION_TICKS);
        gfx_set_pattern(g_gfx_patterns[f]);
        gfx_rec_fill((rec_i32){0, 0, 400, 240}, 1);
        gfx_set_pattern(g_gfx_patterns[GFX_PATTERN_FULL]);
}

static void draw_rope(rope_s *r, v2_i32 camp)
{
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                v2_i32      p1 = v2_add(r1->p, camp);
                v2_i32      p2 = v2_add(r2->p, camp);
                gfx_line_thick(p1.x, p1.y, p2.x, p2.y, 3, 1);
        }

        gfx_set_pattern(g_gfx_patterns[8]);
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                v2_i32      p1 = v2_add(r1->p, camp);
                v2_i32      p2 = v2_add(r2->p, camp);
                gfx_line_thick(p1.x, p1.y, p2.x, p2.y, 1, 0);
        }
        gfx_reset_pattern();
}

static void draw_particles(game_s *g, v2_i32 camp)
{
        tex_s   tparticle = tex_get(TEXID_PARTICLE);
        rec_i32 rparticle = {0, 0, 4, 4};
        for (int n = 0; n < g->n_particles; n++) {
                particle_s *p   = &g->particles[n];
                v2_i32      pos = v2_add(v2_shr(p->p_q8, 8), camp);
                gfx_sprite_mode(tparticle, pos, rparticle, 0);
        }
}

void game_draw(game_s *g)
{
#if 0
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

        gfx_sprite_fast(thook, (v2_i32){hookpos - 512, 50}, (rec_i32){0, yyy * 128, 512, 128});
        for (int n = 1; n < 10; n++) {
                gfx_sprite_fast(thook, (v2_i32){hookpos - 512 - n * 84, 50}, (rec_i32){0, yyy * 128, 84, 128});
        }

        return;
#endif
        v2_i32  camp = {-(g->cam.pos.x - g->cam.wh),
                        -(g->cam.pos.y - g->cam.hh)};
        rec_i32 camr = {-camp.x, -camp.y, g->cam.w, g->cam.h};
        i32     x1, y1, x2, y2;
        tilegrid_bounds_rec(g, camr, &x1, &y1, &x2, &y2);
        draw_background(&g->backforeground, camp);

        if (objhandle_is_valid(g->hero.hook))
                draw_rope(&g->hero.rope, camp);

        obj_listc_s oalive = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < oalive.n; n++) {
                obj_s *o = oalive.o[n];

                switch (o->ID) {
                case 1: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_SOLID);
                        gfx_sprite_fast(ttex, pos, (rec_i32){0, 0, 64, 48});
                } break;
                case 2: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_SOLID);
                        int    tx   = 1;
                        int    ty   = 96 + (o->facing == -1 ? 16 : 0);
                        if (o->vel_q8.y < -150) tx = 0;
                        if (o->vel_q8.y > +150) tx = 2;
                        gfx_sprite_fast(ttex, pos, (rec_i32){tx * 16, ty, 16, 16});
                } break;
                case 3: {

                        if (g->hero.swordticks > 0) {
                                rec_i32 hero_sword_hitbox(obj_s * o, hero_s * h);
                                rec_i32 swordbox = hero_sword_hitbox(o, &g->hero);
                                swordbox.x += camp.x;
                                swordbox.y += camp.y;
                                gfx_rec_fill(swordbox, 1);
                        }

                        // just got hit flashing
                        if (o->invincibleticks > 0 && (o->invincibleticks & 15) < 8)
                                break;
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_HERO);
                        pos.x -= 22;
                        pos.y  = pos.y + o->h - 64;
                        int fl = o->facing == 1 ? 0 : 2;

                        int animy = 0;
                        if (o->vel_q8.x == 0 && game_area_blocked(g, obj_rec_bottom(o)) &&
                            (o->animation % 400) < 200) {
                                animy += 64;
                                o->animframe = 0;
                        }
#if 0
                        gfx_set_pattern(g_gfx_patterns[3]);
                        rec_i32 oaabb = obj_aabb(o);
                        oaabb.x += camp.x;
                        oaabb.y += camp.y;
                        oaabb.w += 30;
                        oaabb.h += 30;
                        gfx_rec_fill(oaabb, 1);
                        gfx_reset_pattern();
#else
                        gfx_sprite_flip(ttex, pos, (rec_i32){o->animframe * 64, animy, 64, 64}, fl);
#endif
                } break;
                case 5: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_HERO);
                        pos.x -= 10;
                        pos.y = pos.y + o->h - 32;
                        gfx_sprite_fast(ttex, pos, (rec_i32){64, 96, 64, 64});
                } break;
                default: {
                        rec_i32 r = translate_rec(obj_aabb(o), camp);
                        gfx_rec_fill(r, 1);
                } break;
                }

                for (int i = 0; i < o->nspriteanim; i++) {
                        obj_sprite_anim_s *sa   = &o->spriteanim[i];
                        texregion_s        texr = sprite_anim_get(sa);
                        gfx_tr_sprite_mode(texr, o->pos, 0);
                }
        }

        draw_particles(g, camp);
        draw_tiles(g, x1, y1, x2, y2, camp);

        draw_foreground(&g->backforeground, camp);
        draw_item_selection(g);

#if 0 // some debug drawing
        for (int n = 1; n < g->water.nparticles; n++) {
                i32 yy1 = water_amplitude(&g->water, n - 1);
                i32 yy2 = water_amplitude(&g->water, n);
                yy1 += 184;
                yy2 += 184;
                gfx_line_thick((n - 1) * 4, yy1, n * 4, yy2, 1, 1);
        }

        for (pathnode_s *pn1 = &g->pathmover.nodes[0], *pn2 = &g->pathmover.nodes[1];
             pn1 && pn2; pn1 = pn2, pn2 = pn2->next) {
                gfx_line(pn1->p.x, pn1->p.y, pn2->p.x, pn2->p.y, 1);
                if (pn2 == &g->pathmover.nodes[0]) break;
        }
        v2_i32 pmpos = path_pos(&g->pathmover);
        gfx_rec_fill((rec_i32){pmpos.x, pmpos.y, 16, 16}, 1);
#endif
        g->hero.caninteract = !g->textbox.active;
        obj_s *ohero;
        if (g->hero.caninteract && try_obj_from_handle(g->hero.obj, &ohero)) {
                v2_i32 heroc        = obj_aabb_center(ohero);
                obj_s *interactable = interactable_closest(g, heroc);

                if (interactable) {
                        v2_i32 pp = obj_aabb_center(interactable);
                        pp.y -= 32;
                        pp.x -= 8;
                        pp         = v2_add(pp, camp);
                        tex_s tint = tex_get(TEXID_INPUT_EL);
                        int   yy   = (os_tick() % 60) < 30 ? 32 : 0;
                        gfx_sprite_fast(tint, pp, (rec_i32){32 * 0, yy, 32, 32});
                }
        }

        if (g->transition.phase)
                draw_transition(g);
        if (g->textbox.active)
                draw_textbox(&g->textbox);

#if 0 // pattern test
        gfx_rec_fill((rec_i32){0, 0, 400, 240}, 1);
        static int dir   = 1;
        static int frame = 0;

        if ((os_tick() % 2) == 0) {
                frame += dir;
                if (frame == 0 || frame == NUM_GFX_PATTERN - 1) dir = -dir;
        }
        gfx_set_pattern(g_gfx_patterns[frame]);
        gfx_rec_fill((rec_i32){0, 0, 400, 240}, 0);
        gfx_reset_pattern();
#endif
}