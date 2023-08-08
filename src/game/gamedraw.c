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

static void tilecache_update(game_s *g, tilecache_s *tc, i32 x1, i32 y1, i32 x2, i32 y2, i32 camx1, i32 camy1)
{

        tc->x = ((x1 << 4) - camx1);
        tc->y = ((y1 << 4) - camy1);
        if (tc->x1 == x1 && tc->y1 == y1 & tc->x2 == x2 && tc->y2 == y2)
                return;
        tex_s t = tc->t;
        tc->x1  = x1;
        tc->y1  = y1;
        tc->x2  = x2;
        tc->y2  = y2;
        gfx_draw_to(tc->t);
        os_memclr4(t.px, t.w_byte * t.h * 2);
        const tex_s tileset = tex_get(TEXID_TILESET);
#if 1
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                rtile_s *rtiles = g->rtiles[x + y * g->tiles_x];
                rtile_s  rt     = rtiles[0]; // todo
                if (rt.flags == 0xFF) continue;
                v2_i32  pos     = {((x - x1) << 4), ((y - y1) << 4)};
                rec_i32 tilerec = {rt.tx << 4, rt.ty << 4, 16, 16};
                gfx_sprite_fast(tileset, pos, tilerec);
        }
#endif
        gfx_draw_to(tex_get(0));
}

static void draw_tiles(game_s *g, i32 x1, i32 y1, i32 x2, i32 y2, i32 camx1, i32 camy1)
{
        ASSERT(0 <= x1 && 0 <= y1 && x2 < g->tiles_x && y2 < g->tiles_y);

        const tex_s tileset = tex_get(TEXID_TILESET);
        foreach_tile_in_bounds(x1, y1, x2, y2, x, y)
        {
                rtile_s *rtiles = g->rtiles[x + y * g->tiles_x];
                rtile_s  rt     = rtiles[0]; // todo
                if (rt.flags == 0xFF) continue;
                v2_i32  pos     = {(x << 4) - camx1, (y << 4) - camy1};
                rec_i32 tilerec = {rt.tx << 4, rt.ty << 4, 16, 16};
                gfx_sprite_fast(tileset, pos, tilerec);
        }
}

void game_draw(game_s *g)
{
        i32     camx1 = g->cam.pos.x - g->cam.wh;
        i32     camy1 = g->cam.pos.y - g->cam.hh;
        rec_i32 camr  = {camx1, camy1, g->cam.w, g->cam.h};
        i32     tx1, ty1, tx2, ty2;
        game_tile_bounds_rec(g, camr, &tx1, &ty1, &tx2, &ty2);

        gfx_sprite_fast(tex_get(TEXID_CLOUDS), (v2_i32){50, 50}, (rec_i32){0, 0, 256, 256});

        float time1 = os_time();
#if 1
        draw_tiles(g, tx1, ty1, tx2, ty2, camx1, camy1);
#else
        tilecache_update(g, &g->tilecache, tx1, ty1, tx2, ty2, camx1, camy1);
        gfx_sprite_fast(g->tilecache.t,
                        (v2_i32){g->tilecache.x, g->tilecache.y},
                        (rec_i32){0, 0, TILECACHE_W, TILECACHE_H});
#endif
        float time2 = os_time();
        // PRINTF("%f\n", time2 - time1);
        os_debug_time(TIMING_DRAW_TILES, time2 - time1);

        obj_listc_s oalive = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < oalive.n; n++) {
                obj_s  *o = oalive.o[n];
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

        textbox_s *tb = &g->textbox;
        if (tb->active) {
                draw_textbox(tb);
        }
}