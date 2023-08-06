// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "os/os.h"

static void draw_textbox(textbox_s *tb)
{
        fnt_s   font = fnt_get(FNTID_DEFAULT);
        rec_i32 r    = (rec_i32){0, 0, 400, 128};
        gfx_sprite(tex_get(TEXID_TEXTBOX), (v2_i32){0, 128}, r, 0);
        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(&font, line->chars, line->n_shown, 20, 150 + 21 * l);
        }

        static i32 npage = 0;
        if (tb->shows_all) {
                npage += 10000;
                int yy = sin_q16((npage % 0x40000)) / 10000;
                gfx_rec_fill((rec_i32){370, 205 + yy, 10, 10}, 1);
        }
}

static void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, i32 camx1, i32 camy1)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);

        tex_s tileset = tex_get(TEXID_TILESET);
        for (int y = y1; y <= y2; y++) {
                v2_i32 pos;
                pos.y = (uint)y * 16 - camy1;
                for (int x = x1; x <= x2; x++) {
                        pos.x           = (uint)x * 16 - camx1;
                        int      tID    = x + y * g->tiles_x;
                        rtile_s *rtiles = g->rtiles[tID];

                        for (int n = 0; n < 1; n++) {   // actually for both layers
                                rtile_s rt = rtiles[n]; // todo
                                if (rt.flags == 0xFF) continue;
                                rec_i32 tilerec = {rt.tx * 16, rt.ty * 16, 16, 16};
                                // gfx_sprite(tileset, pos, tilerec, rt.flags);
                                gfx_sprite_fast(tileset, pos, tilerec);
                        }
                }
        }
}

void game_draw(game_s *g)
{
        i32     camx1 = g->cam.pos.x - g->cam.wh;
        i32     camy1 = g->cam.pos.y - g->cam.hh;
        rec_i32 camr  = {camx1, camy1, g->cam.w, g->cam.h};
        i32     tx1, ty1, tx2, ty2;
        game_tile_bounds_rec(g, camr, &tx1, &ty1, &tx2, &ty2);

        gfx_sprite_fast(tex_get(TEXID_CLOUDS), (v2_i32){50, 50}, (rec_i32){0, 0, 256, 256}, 0);

        draw_tiles(g, tx1, ty1, tx2, ty2, camx1, camy1);

        for (int n = 0; n < objset_len(&g->obj_active); n++) {
                obj_s  *o = objset_at(&g->obj_active, n);
                rec_i32 r = translate_rec_xy(obj_aabb(o), -camx1, -camy1);
                gfx_rec_fill(r, 1);
        }

        if (objhandle_is_valid(g->hero.hook)) {
                for (ropenode_s *r1 = g->hero.rope.head; r1 && r1->next; r1 = r1->next) {
                        ropenode_s *r2 = r1->next;
                        gfx_line_thick(r1->p.x - camx1, r1->p.y - camy1,
                                       r2->p.x - camx1, r2->p.y - camy1, 1, 1);
                }
        }

        // simple transition animation
        if (g->transitionphase) {
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

        gfx_sprite(tex_get(TEXID_ITEMS), (v2_i32){400 - 32, 4}, (rec_i32){0, 0, 32, 32}, 0);

        textbox_s *tb = &g->textbox;
        if (tb->active) {
                draw_textbox(tb);
        }

        fnt_s font = fnt_get(FNTID_DEFAULT);
        // gfx_text_ascii(&font, "Pickups: ", )
}