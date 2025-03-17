// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "minimap.h"
#include "game.h"

v2_i32 minimap_hero_pos(g_s *g)
{
    v2_i32 p     = {0};
    obj_s *ohero = obj_get_hero(g);

    if (ohero) {
        v2_i32 poshero = v2_i32_shr(obj_pos_center(ohero), 4);
        p.x            = g->map_room_cur->x + poshero.x;
        p.y            = g->map_room_cur->y + poshero.y;
    }
    return p;
}

void minimap_init(g_s *g)
{
    minimap_s *m = &g->minimap;
}

void minimap_open(g_s *g)
{
    minimap_s *m   = &g->minimap;
    m->opened      = 1;
    m->pin_hovered = 0;
    v2_i32 ph      = minimap_hero_pos(g);
    m->centerx     = (ph.x + 1) & ~1;
    m->centery     = (ph.y + 1) & ~1;
    m->fade        = 0;
    m->fade_out    = 0;
}

void minimap_update(g_s *g)
{
    minimap_s *m = &g->minimap;

    if (m->fade_out) {
        m->fade_out++;
        if (m->fade_out == 8) {
            m->fade_out = 0;
            m->opened   = 0;
        }
        return;
    }

    if (inp_btn_jp(INP_B)) {
        // m->opened = 0;
        m->fade_out = 1;
        return;
    }

    m->fade++;
    i32 dpad_x = inp_x();
    i32 dpad_y = inp_y();

    if (inp_btn(INP_A)) {
        if (inp_btn_jp(INP_A)) {
            if (m->pin_hovered) {
                m->tick = 1;
            } else {
#if 0
                map_pin_s *p   = &m->pins[m->n_pins++];
                p->x           = m->centerx;
                p->y           = m->centery;
                m->pin_hovered = p;
#endif
            }
        } else if (m->tick) {
            m->tick++;

            if (m->tick == 20) {
                m->tick         = 0;
                *m->pin_hovered = m->pins[--m->n_pins];
                m->pin_hovered  = 0;
            }
        }
    } else {
        m->tick = 0;
    }

    if (!m->tick) {
        if (dpad_x | dpad_y) {
            m->centerx += dpad_x << 1;
            m->centery += dpad_y << 1;
            m->pin_hovered = 0;
        } else {
            i32 dxp = 0;
            i32 dyp = 0;

            for (i32 n = 0, d_closest = 50; n < m->n_pins; n++) {
                map_pin_s *p  = &m->pins[n];
                i32        dx = p->x - m->centerx;
                i32        dy = p->y - m->centery;
                i32        dp = dx * dx + dy * dy;
                if (dp < d_closest) {
                    d_closest      = dp;
                    m->pin_hovered = p;
                    dxp            = dx;
                    dyp            = dy;
                }
            }

            if (m->pin_hovered) {
                m->centerx += sgn_i32(dxp) << 1;
                m->centery += sgn_i32(dyp) << 1;
            }
        }
    }
}

void minimap_draw_at(tex_s tex, g_s *g, i32 ox, i32 oy, b32 menu)
{
    minimap_s *m    = &g->minimap;
    gfx_ctx_s  ctx  = gfx_ctx_default(tex);
    gfx_ctx_s  ctxr = ctx;
    ctxr.pat        = gfx_pattern_25();
    tex_clr(ctx.dst, GFX_COL_BLACK);

    for (i32 pass = 0; pass < 2; pass++) {
        for (i32 n = 0; n < g->n_map_rooms; n++) {
            map_room_s *mr  = &g->map_rooms[n];
            v2_i32      pos = {mr->x + ox,
                               mr->y + oy};

            switch (pass) {
            case 0: {
                gfx_spr(ctx, texrec_from_tex(mr->t), pos, 0, 0);
                break;
            }
            case 1: {
                rec_i32 rr = {pos.x, pos.y, mr->w, mr->h};
                rec_i32 r1 = {rr.x, rr.y - 1, rr.w, 2};
                rec_i32 r2 = {rr.x - 1, rr.y, 2, rr.h};
                rec_i32 r3 = {rr.x + rr.w - 1, rr.y, 2, rr.h};
                rec_i32 r4 = {rr.x, rr.y + rr.h - 1, rr.w, 2};

                gfx_rec_fill(ctxr, r1, GFX_COL_WHITE);
                gfx_rec_fill(ctxr, r2, GFX_COL_WHITE);
                gfx_rec_fill(ctxr, r3, GFX_COL_WHITE);
                gfx_rec_fill(ctxr, r4, GFX_COL_WHITE);
                break;
            }
            }
        }
    }

    gfx_ctx_s ctx_obscured = ctx;
    ctx_obscured.pat       = gfx_pattern_75();
    for (i32 y = 0; y < MINIMAP_SCREENS_Y; y++) {
        for (i32 x = 0; x < MINIMAP_SCREENS_X; x++) {
            if (minimap_screen_visited_tmp(g, x, y)) continue;

            rec_i32   rr   = {(x - 81 - 1) * 25 + ox,
                              (y - 136 - 1) * 15 + oy, 25, 15};
            gfx_ctx_s ctxr = minimap_screen_visited_tmp(g, x, y) ? ctx_obscured
                                                                 : ctx;
            gfx_rec_fill(ctx, rr, GFX_COL_BLACK);
        }
    }

    obj_s *ohero = obj_get_hero(g);

    if (ohero && (menu || ((g->tick_animation >> 4) & 3))) {
        texrec_s tricon  = asset_texrec(TEXID_BUTTONS, 288, 0, 32, 32);
        v2_i32   poshero = v2_i32_shr(obj_pos_center(ohero), 4);
        poshero.x += ox + g->map_room_cur->x - 16;
        poshero.y += oy + g->map_room_cur->y - 16;

        gfx_spr(ctx, tricon, poshero, 0, 0);
    }

    for (i32 n = 0; n < m->n_pins; n++) {
        map_pin_s *p      = &m->pins[n];
        texrec_s   tricon = asset_texrec(TEXID_BUTTONS, 288 + 32, 0, 32, 32);
        v2_i32     pos    = {p->x + ox - 16, p->y + oy - 16};
        gfx_spr(ctx, tricon, pos, 0, 0);
    }

    if (!menu) {
        texrec_s tricon = asset_texrec(TEXID_BUTTONS, 448, 2 * 64, 64, 64);
        v2_i32   pos    = {200 - 32, 120 - 32};
        tricon.y += ((g->tick_animation >> 4) & 1) * 64;

        if (m->pin_hovered) {
            if (m->tick) {
                tricon.y = 64 * 7;
            } else {
                tricon.y += 64 * 4;
            }
        }

        gfx_spr(ctx, tricon, pos, 0, 0);
    }
}

void minimap_draw(g_s *g)
{
    minimap_s *m  = &g->minimap;
    i32        ox = 200 - m->centerx;
    i32        oy = 120 - m->centery;
    i32        f2 = 10;
    i32        f  = min_i32(m->fade, f2);
    ox -= lerp_i32(100, 0, f, f2);
    minimap_draw_at(asset_tex(TEXID_PAUSE_TEX), g, ox, oy, 0);
    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = texrec_from_tex(asset_tex(TEXID_PAUSE_TEX));
    v2_i32    pos = {0};

    if (m->fade_out) {
        f2 = 8;
        f  = f2 - min_i32(m->fade_out, f2);
    }
    ctx.pat = gfx_pattern_interpolate(f, f2);
    gfx_spr(ctx, tr, pos, 0, 0);
}

void minimap_draw_pause(g_s *g)
{
    minimap_s *m  = &g->minimap;
    v2_i32     ph = minimap_hero_pos(g);
    ph.x          = (ph.x + 1) & ~1;
    ph.y          = (ph.y + 1) & ~1;

    if (m->opened) {
        ph.x = m->centerx;
        ph.y = m->centery;
    } else {
        ph.x += 100;
    }
    i32 ox = 200 - ph.x;
    i32 oy = 120 - ph.y;
    minimap_draw_at(asset_tex(TEXID_PAUSE_TEX), g, ox, oy, 1);
}

void minimap_confirm_visited(g_s *g)
{
    minimap_s *m = &g->minimap;

    for (i32 n = 0; n < ARRLEN(m->visited); n++) {
        m->visited[n] |= m->visited_tmp[n];
    }
}

void minimap_try_visit_screen(g_s *g)
{
    minimap_s *m  = &g->minimap;
    v2_i32     ph = minimap_hero_pos(g);
    i32        xi = ph.x / 25 + 81;
    i32        yi = ph.y / 15 + 136;
    i32        k  = xi + yi * MINIMAP_SCREENS_X;
    m->visited_tmp[k >> 5] |= (u32)1 << (k & 31);
    // minimap_confirm_visited(g);
}

b32 minimap_screen_visited_tmp(g_s *g, i32 sx, i32 sy)
{
    minimap_s *m  = &g->minimap;
    i32        xi = sx;
    i32        yi = sy;
    i32        k  = xi + yi * MINIMAP_SCREENS_X;
    return (m->visited_tmp[k >> 5] & ((u32)1 << (k & 31)));
}

b32 minimap_screen_visited(g_s *g, i32 sx, i32 sy)
{
    minimap_s *m  = &g->minimap;
    i32        xi = sx;
    i32        yi = sy;
    i32        k  = xi + yi * MINIMAP_SCREENS_X;
    return (m->visited[k >> 5] & ((u32)1 << (k & 31)));
}