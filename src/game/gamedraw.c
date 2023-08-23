// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "os/os.h"

static void draw_textbox(textbox_s *tb)
{
        fnt_s   font = fnt_get(FNTID_DEFAULT);
        rec_i32 r    = (rec_i32){0, 0, 400, 128};
        gfx_sprite_fast(tex_get(TEXID_TEXTBOX), (v2_i32){0, 128}, r);
        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(&font, line->chars, line->n_shown, 20, 150 + 21 * l);
        }

        if (!tb->shows_all) return;

        if (tb->n_choices) {
                // draw choices if any
                gfx_rec_fill((rec_i32){230, 60, 70, 100}, 0);
                for (int n = 0; n < tb->n_choices; n++) {
                        textboxchoice_s *tc      = &tb->choices[n];
                        static const int spacing = 21;

                        gfx_text_glyphs(&font, tc->label, tc->labellen,
                                        250, 70 + spacing * n);
                        gfx_rec_fill((rec_i32){235, 70 + spacing * tb->cur_choice, 10, 10}, 1);
                }
        } else {
                // "next page" animated marker in corner
                tb->page_animation_state += 10000;
                tb->page_animation_state &= (Q16_ANGLE_TURN - 1);
                int yy = sin_q16(tb->page_animation_state) >> 14;
                gfx_rec_fill((rec_i32){370, 205 + yy, 10, 10}, 1);
        }
}

static void draw_background(backforeground_s *bg, v2_i32 camp)
{
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
}

static void wind_particle_line(v2_i32 p0, v2_i32 p1)
{
        int dx = +ABS(p1.x - p0.x), sx = p0.x < p1.x ? 1 : -1;
        int dy = -ABS(p1.y - p0.y), sy = p0.y < p1.y ? 1 : -1;
        int er = dx + dy;
        int xi = p0.x;
        int yi = p0.y;
        while (1) {
                if ((xi & 1) ^ (yi & 1))
                        gfx_px(xi, yi, 1, 0);
                if (xi == p1.x && yi == p1.y) break;
                int e2 = er * 2;
                if (e2 >= dy) er += dy, xi += sx;
                if (e2 <= dx) er += dx, yi += sy;
        }
}

static void draw_foreground(backforeground_s *bg, v2_i32 camp)
{
        // wind animation
        for (int n = 0; n < bg->nparticles; n++) {
                particlebg_s p = bg->particles[n];

                v2_i32 p1 = v2_add(v2_shr(p.pos[p.n], 8), camp);
                for (int i = 1; i < 16; i++) {
                        int    k  = (p.n + i) & (BG_WIND_PARTICLE_N - 1);
                        v2_i32 p2 = v2_add(v2_shr(p.pos[k], 8), camp);

                        wind_particle_line(p1, p2);
                        p1 = p2;
                }
        }
}

static void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, v2_i32 camp)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);
        float timet   = os_time();
        tex_s tileset = tex_get(TEXID_TILESET);
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                rtile_s rt = g->rtiles[x + y * g->tiles_x];
                int     ID = g_tileIDs[rt.ID];
                if (ID == TILEID_NULL) continue;
                int tx, ty;
                tileID_decode(ID, &tx, &ty);
                v2_i32 pos  = {(x << 4) + camp.x, (y << 4) + camp.y};
                v2_i32 tpos = {tx, ty};
                gfx_sprite_tile_16(tileset, pos, tpos);
        }
        os_debug_time(TIMING_DRAW_TILES, os_time() - timet);
}

/* maps texture generated above onto a cylinder
 * 1) point on circle
 * 2) calc angle on circle
 * 3) calc radians on circle, getting pos in image texture
 *    image is "wrapped" onto cylinder
 */
static void draw_item_selection(game_s *g)
{
        hero_s *h       = &g->hero;
        tex_s   titem   = tex_get(TEXID_ITEMS);
        int     itemID0 = h->c_item == 0 ? h->n_items - 1 : h->c_item - 1;
        int     itemID1 = h->c_item;
        int     itemID2 = (h->c_item + 1) % h->n_items;

        os_spmem_push();

        u8   *px  = os_spmem_allocz(sizeof(u8) * 64 * 128);
        tex_s tt  = {0};
        tt.px     = px;
        tt.w      = 64;
        tt.h      = 128;
        tt.mk     = px + tt.w * tt.h;
        tt.w_byte = tt.w / 8;
        tt.w_word = tt.w / 32;

        int ii = -((32 * os_inp_crank()) >> 16);
        if (os_inp_crank() >= 0x8000) {
                ii += 32;
        }

        gfx_draw_to(tt);
        gfx_sprite_fast(titem, (v2_i32){0, ii},
                        (rec_i32){32, itemID0 * 32, 32, 32});
        gfx_sprite_fast(titem, (v2_i32){0, ii + 32},
                        (rec_i32){32, itemID1 * 32, 32, 32});
        gfx_sprite_fast(titem, (v2_i32){0, ii + 64},
                        (rec_i32){32, itemID2 * 32, 32, 32});
        gfx_draw_to(tex_get(0));

        for (int j = -16; j <= +16; j++) {
                float l = acosf((float)j / 16.f) * 16.f;
                gfx_sprite_fast(tt, (v2_i32){400 - 32, 8 + (16 - j)},
                                (rec_i32){0, 24 + (int)l, 32, 1}); // y pos add is kinda magic number
        }

        gfx_sprite_fast(titem, (v2_i32){400 - 32 - 16, -8},
                        (rec_i32){64, 0, 64, 64});

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
        switch (t->phase) {
        case TRANSITION_FADE_IN: {
                int x = lerp_i32(0, 410, t->ticks, TRANSITION_TICKS);
                gfx_rec_fill((rec_i32){0, 0, x, 240}, 1);
        } break;
        case TRANSITION_FADE_OUT: {
                int x = lerp_i32(410, 0, t->ticks, TRANSITION_TICKS);
                gfx_rec_fill((rec_i32){0, 0, x, 240}, 1);
        } break;
        }
}

static void draw_rope(rope_s *r, v2_i32 camp)
{
        for (ropenode_s *r1 = r->head; r1 && r1->next; r1 = r1->next) {
                ropenode_s *r2 = r1->next;
                v2_i32      p1 = v2_add(r1->p, camp);
                v2_i32      p2 = v2_add(r2->p, camp);
                gfx_line_thick(p1.x, p1.y, p2.x, p2.y, 1, 1);
        }
}

static void draw_particles(game_s *g, v2_i32 camp)
{
        tex_s   tparticle = tex_get(TEXID_PARTICLE);
        rec_i32 rparticle = {0, 0, 4, 4};
        for (int n = 0; n < g->n_particles; n++) {
                particle_s *p   = &g->particles[n];
                v2_i32      pos = {(p->p_q8.x >> 8) + camp.x, (p->p_q8.y >> 8) + camp.y};
                gfx_sprite_(tparticle, pos, rparticle, 0);
        }
}

void game_draw(game_s *g)
{
        v2_i32  camp = {-(g->cam.pos.x - g->cam.wh),
                        -(g->cam.pos.y - g->cam.hh)};
        rec_i32 camr = {-camp.x, -camp.y, g->cam.w, g->cam.h};
        i32     x1, y1, x2, y2;
        tilegrid_bounds_rec(g, camr, &x1, &y1, &x2, &y2);

        draw_background(&g->backforeground, camp);

        obj_listc_s oalive = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < oalive.n; n++) {
                obj_s *o = oalive.o[n];
                switch (o->ID) {
                case 1: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_SOLID);
                        gfx_sprite_(ttex, pos, (rec_i32){0, 0, 64, 48}, 0);
                } break;
                case 2: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_SOLID);
                        int    tx   = 1;
                        int    ty   = 96;
                        if (o->vel_q8.x < 0) ty += 16;
                        if (o->vel_q8.y < -150) tx = 0;
                        if (o->vel_q8.y > +150) tx = 2;
                        gfx_sprite_(ttex, pos, (rec_i32){tx * 16, ty, 16, 16}, 0);
                } break;
                case 3: {
                        v2_i32 pos  = v2_add(o->pos, camp);
                        tex_s  ttex = tex_get(TEXID_HERO);
                        pos.x -= 8;
                        pos.y -= 18;
                        gfx_sprite_(ttex, pos, (rec_i32){0, 0, 32, 48}, 0);
                } break;
                default: {
                        rec_i32 r = translate_rec(obj_aabb(o), camp);
                        gfx_rec_fill(r, 1);
                } break;
                }
        }

        if (objhandle_is_valid(g->hero.hook))
                draw_rope(&g->hero.rope, camp);

        draw_tiles(g, x1, y1, x2, y2, camp);

        draw_particles(g, camp);
        draw_foreground(&g->backforeground, camp);

        draw_item_selection(g);

#if 1 // some debug drawing
        for (int n = 1; n < g->water.nparticles; n++) {
                i32 yy1 = water_amplitude(&g->water, n - 1);
                i32 yy2 = water_amplitude(&g->water, n);
                yy1 += 150;
                yy2 += 150;
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

        obj_s *ohero;
        if (try_obj_from_handle(g->hero.obj, &ohero)) {
                v2_i32 heroc        = obj_aabb_center(ohero);
                obj_s *interactable = interactable_closest(g, heroc);

                if (interactable) {
                        v2_i32 pp = obj_aabb_center(interactable);
                        pp.y -= 8 + 24;
                        pp.x -= 8;
                        v2_i32 pp2  = v2_add(pp, camp);
                        tex_s  tint = tex_get(TEXID_SOLID);
                        int    yy   = (os_tick() % 60) < 30 ? 16 : 0;
                        gfx_sprite_fast(tint, pp2, (rec_i32){160, yy, 16, 16});
                }
        }

        if (g->transition.phase)
                draw_transition(g);
        if (g->textbox.active)
                draw_textbox(&g->textbox);
}