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
        for (int n = 0; n < bg->nclouds; n++) {
                cloudbg_s c = bg->clouds[n];
        }

        gfx_sprite_fast(tex_get(TEXID_CLOUDS), (v2_i32){50, 50}, (rec_i32){0, 0, 256, 256});
}

static void particle_line(v2_i32 p0, v2_i32 p1)
{
        int dx = +ABS(p1.x - p0.x);
        int dy = -ABS(p1.y - p0.y);
        int sx = p0.x < p1.x ? 1 : -1;
        int sy = p0.y < p1.y ? 1 : -1;
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

                v2_i32 pos1 = v2_add(v2_shr(p.pos[p.n], 8), camp);
                for (int i = 1; i < 16; i++) {
                        v2_i32 p2   = p.pos[(p.n + i) & 15];
                        v2_i32 pos2 = v2_add(v2_shr(p2, 8), camp);

                        particle_line(pos1, pos2);
                        pos1 = pos2;
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
                v2_i32  pos     = {(x << 4) + camp.x, (y << 4) + camp.y};
                rec_i32 tilerec = {tx << 4, ty << 4, 16, 16};
                gfx_sprite_fast(tileset, pos, tilerec);
        }
        os_debug_time(TIMING_DRAW_TILES, os_time() - timet);
}

static void draw_item_selection(game_s *g)
{
        tex_s titem = tex_get(TEXID_ITEMS);
        gfx_sprite_fast(titem, (v2_i32){400 - 32, 0}, (rec_i32){0, 0, 32, 32});
        int itemID = g->hero.c_item;
        gfx_sprite_fast(titem, (v2_i32){400 - 32, 0}, (rec_i32){0, (itemID + 1) * 32, 32, 32});
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
                default: {
                        rec_i32 r = translate_rec(obj_aabb(o), camp);
                        gfx_rec_fill(r, 1);
                } break;
                }
                if (o->ID == 1) {

                } else {
                }
        }

        if (objhandle_is_valid(g->hero.hook))
                draw_rope(&g->hero.rope, camp);

        draw_tiles(g, x1, y1, x2, y2, camp);

        draw_particles(g, camp);
        draw_foreground(&g->backforeground, camp);

        draw_item_selection(g);

        obj_s *ohero;
        if (try_obj_from_handle(g->hero.obj, &ohero)) {
                v2_i32 heroc        = obj_aabb_center(ohero);
                obj_s *interactable = interactable_closest(g, heroc);

                if (interactable) {
                        v2_i32 pp = obj_aabb_center(interactable);
                        if (v2_distancesq(pp, heroc) < 100) {
                                pp.y -= 8 + 24;
                                pp.x -= 8;
                                v2_i32 pp2  = v2_add(pp, camp);
                                tex_s  tint = tex_get(TEXID_SOLID);
                                int    yy   = (os_tick() % 60) < 30 ? 16 : 0;
                                gfx_sprite_fast(tint, pp2, (rec_i32){160, yy, 16, 16});
                        }
                }
        }

        if (g->transition.phase)
                draw_transition(g);
        if (g->textbox.active)
                draw_textbox(&g->textbox);
}