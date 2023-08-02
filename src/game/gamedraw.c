// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "os/os.h"

static void draw_textbox(game_s *g)
{
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
                                rec_i32 tilerec = {rt.tx * 16,
                                                   rt.ty * 16,
                                                   16, 16};
                                gfx_sprite(tileset, pos, tilerec, rt.flags);
                        }
                }
        }
}

void game_draw(game_s *g)
{
        i32 camx1 = g->cam.pos.x - g->cam.wh;
        i32 camy1 = g->cam.pos.y - g->cam.hh;
        i32 camx2 = camx1 + g->cam.w;
        i32 camy2 = camy1 + g->cam.h;

        i32 tx1 = (uint)MAX(camx1, 0) / 16;
        i32 ty1 = (uint)MAX(camy1, 0) / 16;
        i32 tx2 = (uint)MIN(camx2, g->pixel_x - 1) / 16;
        i32 ty2 = (uint)MIN(camy2, g->pixel_y - 1) / 16;

        draw_tiles(g, tx1, ty1, tx2, ty2, camx1, camy1);

        for (int n = 0; n < objset_len(&g->obj_active); n++) {
                obj_s  *o = objset_at(&g->obj_active, n);
                rec_i32 r = translate_rec_xy(obj_aabb(o),
                                             -camx1,
                                             -camy1);
                gfx_rec_fill(r, 1);
        }

        if (g->textbox.active) {
                draw_textbox(g);
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
                        int x = lerp_i32(0, 410,
                                         g->transitionticks,
                                         TRANSITION_TICKS);
                        gfx_rec_fill((rec_i32){0, 0, x, 240}, 1);
                } break;
                case TRANSITION_FADE_OUT: {
                        int x = lerp_i32(410, 0,
                                         g->transitionticks,
                                         TRANSITION_TICKS);
                        gfx_rec_fill((rec_i32){0, 0, x, 240}, 1);
                } break;
                }
        }
        // gfx_sprite(tex_get(TEXID_TILESET), (v2_i32){0, 0}, (rec_i32){0, 0, 512, 128}, 0);
}