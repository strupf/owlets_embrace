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

        if (tb->shows_all) {
                tb->page_animation_state += 10000;
                tb->page_animation_state &= (Q16_ANGLE_TURN - 1);
                int yy = sin_q16(tb->page_animation_state) >> 14;
                gfx_rec_fill((rec_i32){370, 205 + yy, 10, 10}, 1);
        }
}

static void draw_background(game_s *g)
{

        gfx_sprite_fast(tex_get(TEXID_CLOUDS), (v2_i32){50, 50}, (rec_i32){0, 0, 256, 256});
}

static void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, v2_i32 camp)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);
        float timet   = os_time();
        tex_s tileset = tex_get(TEXID_TILESET);
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                rtile_s *rtiles = g->rtiles[x + y * g->tiles_x];
                rtile_s  rt     = rtiles[0]; // todo
                int      ID     = g_tileIDs[rt.ID];
                if (ID == TILEID_NULL) continue;
                int tx, ty;
                tileID_decode(ID, &tx, &ty);
                v2_i32  pos     = {(x << 4) + camp.x, (y << 4) + camp.y};
                rec_i32 tilerec = {tx << 4, ty << 4, 16, 16};
                gfx_sprite_fast(tileset, pos, tilerec);
        }
        os_debug_time(TIMING_DRAW_TILES, os_time() - timet);
}

static void draw_transition(game_s *g)
{
        switch (g->transitionphase) {
        case TRANSITION_FADE_IN: {
                int x = lerp_i32(0, 410, g->transitionticks, TRANSITION_TICKS);
                gfx_rec_fill((rec_i32){0, 0, x, 240}, 1);
        } break;
        case TRANSITION_FADE_OUT: {
                int x = lerp_i32(410, 0, g->transitionticks, TRANSITION_TICKS);
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
        game_tile_bounds_rec(g, camr, &x1, &y1, &x2, &y2);

        draw_background(g);
        draw_tiles(g, x1, y1, x2, y2, camp);

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

        draw_particles(g, camp);

        if (g->transitionphase)
                draw_transition(g);
        if (g->textbox.active)
                draw_textbox(&g->textbox);
}