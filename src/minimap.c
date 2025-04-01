// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "minimap.h"
#include "game.h"

#define MINIMAP_TICKS_PIN      18
#define MINIMAP_NUM_PIN_ICONS  5
#define MINIMAP_TICKS_FADE_IN  8
#define MINIMAP_TICKS_FADE_OUT 8
#define MINIMAP_GRAV_DISTSQ    100

v2_i32 minimap_hero_pos(g_s *g);
bool32 minimap_try_grav_to_hovered_pin(minimap_s *m);
bool32 minimap_try_grav_to_hero(minimap_s *m);
bool32 minimap_hero_hovered(minimap_s *m, i32 *dx, i32 *dy);

void minimap_init(g_s *g)
{
    minimap_s *m = &g->minimap;
}

void minimap_open_internal(g_s *g, i32 state)
{
    minimap_s *m     = &g->minimap;
    v2_i32     ph    = minimap_hero_pos(g);
    m->herox         = (ph.x + 1) & ~1;
    m->heroy         = (ph.y + 1) & ~1;
    m->centerx       = m->herox;
    m->centery       = m->heroy;
    m->pin_hovered   = 0;
    m->tick          = 0;
    m->pin_deny_tick = 0;
    m->pin_ui_fade   = 0;
    m->state         = state;
}

void minimap_open(g_s *g)
{
    minimap_open_internal(g, MINIMAP_ST_FADE_IN);
}

void minimap_open_via_menu(g_s *g)
{
    minimap_open_internal(g, MINIMAP_ST_FADE_IN_MENU);
}

void minimap_update(g_s *g)
{
    minimap_s *m = &g->minimap;

    if (m->pin_deny_tick) {
        m->pin_deny_tick--;
    }

    switch (m->state) {
    case MINIMAP_ST_FADE_IN_MENU:
    case MINIMAP_ST_FADE_IN: {
        m->tick++;
        if (MINIMAP_TICKS_FADE_IN <= m->tick) {
            m->state = MINIMAP_ST_NAV;
            m->tick  = 0;
        }
        break;
    }
    case MINIMAP_ST_FADE_OUT: {
        m->tick++;
        if (MINIMAP_TICKS_FADE_OUT <= m->tick) {
            m->state = MINIMAP_ST_INACTIVE;
        }
        break;
    }
    case MINIMAP_ST_NAV: {
        i32 dpad_x = inp_x();
        i32 dpad_y = inp_y();

        if (inp_btn_jp(INP_B)) {
            m->state = MINIMAP_ST_FADE_OUT;
            m->tick  = 0;
        } else if (inp_btn_jp(INP_A)) {
            if (m->pin_hovered) {
                m->state = MINIMAP_ST_DELETE_PIN;
                m->tick  = 0;
            } else if (!minimap_try_grav_to_hero(m)) {
                if (m->n_pins < MAP_NUM_PINS) {
                    m->state        = MINIMAP_ST_SELECT_PIN;
                    m->pin_selected = 0;
                } else {
                    m->pin_deny_tick = 20;
                }
            }
        } else if (dpad_x | dpad_y) {
            m->centerx += dpad_x << 1;
            m->centery += dpad_y << 1;
            m->pin_hovered = 0;
        } else if (!minimap_try_grav_to_hovered_pin(m)) {
            for (i32 n = 0, ds = MINIMAP_GRAV_DISTSQ; n < m->n_pins; n++) {
                minimap_pin_s *p  = &m->pins[n];
                i32            dx = p->x - m->centerx;
                i32            dy = p->y - m->centery;
                i32            dp = dx * dx + dy * dy;
                if (dp < ds) {
                    ds             = dp;
                    m->pin_hovered = p;
                }
            }

            if (!m->pin_hovered) {
                minimap_try_grav_to_hero(m);
            }
        }
        break;
    }
    case MINIMAP_ST_SELECT_PIN: {
        if (inp_btn_jp(INP_B)) {
            m->state       = MINIMAP_ST_NAV;
            m->pin_ui_fade = 0;
        } else if (inp_btn_jp(INP_A)) {
            m->state         = MINIMAP_ST_NAV;
            minimap_pin_s *p = &m->pins[m->n_pins++];
            p->type          = m->pin_selected;
            p->x             = m->centerx;
            p->y             = m->centery;
            m->pin_hovered   = p;
        } else if (inp_btn_jp(INP_DL)) {
            m->pin_selected = clamp_i32((i32)m->pin_selected - 1,
                                        0, MINIMAP_NUM_PIN_ICONS - 1);
        } else if (inp_btn_jp(INP_DR)) {
            m->pin_selected = clamp_i32((i32)m->pin_selected + 1,
                                        0, MINIMAP_NUM_PIN_ICONS - 1);
        }
        break;
    }
    case MINIMAP_ST_DELETE_PIN: {
        if (inp_btn_jp(INP_B)) {
            m->state = MINIMAP_ST_NAV;
        } else if (inp_btn(INP_A)) {
            minimap_try_grav_to_hovered_pin(m);
            m->tick++;

            if (MINIMAP_TICKS_PIN <= m->tick) {
                m->state        = MINIMAP_ST_NAV;
                m->tick         = 0;
                *m->pin_hovered = m->pins[--m->n_pins];
                m->pin_hovered  = 0;
            }
        } else {
            m->state = MINIMAP_ST_NAV;
        }
        break;
    }
    }

    if (m->state == MINIMAP_ST_SELECT_PIN ||
        m->state == MINIMAP_ST_DELETE_PIN ||
        m->pin_deny_tick) {
        m->pin_ui_fade = min_i32(255, m->pin_ui_fade + 24);
    } else {
        m->pin_ui_fade = max_i32(0, m->pin_ui_fade - 4);
    }
}

void minimap_draw_at(tex_s tex, g_s *g, i32 ox, i32 oy, b32 menu)
{
    minimap_s *m            = &g->minimap;
    gfx_ctx_s  ctx          = gfx_ctx_default(tex);
    gfx_ctx_s  ctxr         = ctx;
    gfx_ctx_s  ctx_obscured = ctx;
    ctxr.pat                = gfx_pattern_50();
    ctx_obscured.pat        = gfx_pattern_shift(gfx_pattern_bayer_4x4(15),
                                                ox & 3, oy & 3);
    tex_clr(ctx.dst, GFX_COL_BLACK);

    // map images

    // images and hding rects
    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mr  = &g->map_rooms[n];
        v2_i32      pos = {mr->x + ox, mr->y + oy};

        gfx_spr(ctx, texrec_from_tex(mr->t), pos, 0, 0);

        for (i32 ny = 0; ny < mr->h; ny += 15) {
            for (i32 nx = 0; nx < mr->w; nx += 25) {
                i32 sx = nx + mr->x;
                i32 sy = ny + mr->y;
                i32 vi = minimap_screen_visited(g, sx, sy);
                if (vi & MINIMAP_SCREEN_VISITED) continue;

                rec_i32   r = {pos.x + nx, pos.y + ny, 25, 15};
                gfx_ctx_s ctxfill =
                    (vi & MINIMAP_SCREEN_VISITED_TMP) ? ctx_obscured : ctx;
                gfx_rec_fill(ctxfill, r, GFX_COL_BLACK);
            }
        }
    }

    // room borders
    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mr  = &g->map_rooms[n];
        v2_i32      pos = {mr->x + ox, mr->y + oy};

        for (i32 ny = 0; ny < mr->h; ny += 15) {
            i32 sy  = mr->y + ny;
            i32 sxl = mr->x + 0;
            i32 sxr = mr->x + mr->w - 1;

            if (minimap_screen_visited(g, sxl, sy) ||
                minimap_screen_visited(g, sxl - 1, sy)) {
                rec_i32 rr = {pos.x, pos.y + ny, 1, 15};
                gfx_rec_fill(ctxr, rr, PRIM_MODE_BLACK_WHITE);
            }
            if (minimap_screen_visited(g, sxr, sy) ||
                minimap_screen_visited(g, sxr + 1, sy)) {
                rec_i32 rr = {pos.x + mr->w, pos.y + ny, 1, 15};
                gfx_rec_fill(ctxr, rr, PRIM_MODE_BLACK_WHITE);
            }
        }
        for (i32 nx = 0; nx < mr->w; nx += 25) {
            i32 sx  = mr->x + nx;
            i32 syu = mr->y + 0;
            i32 syd = mr->y + mr->h - 1;

            if (minimap_screen_visited(g, sx, syu) ||
                minimap_screen_visited(g, sx, syu - 1)) {
                rec_i32 r = {pos.x + nx, pos.y, 25, 1};
                gfx_rec_fill(ctxr, r, PRIM_MODE_BLACK_WHITE);
            }
            if (minimap_screen_visited(g, sx, syd) ||
                minimap_screen_visited(g, sx, syd + 1)) {
                rec_i32 r = {pos.x + nx, pos.y + mr->h, 25, 1};
                gfx_rec_fill(ctxr, r, PRIM_MODE_BLACK_WHITE);
            }
        }
    }

    obj_s *ohero = obj_get_hero(g);

    // all map pins
    for (i32 n = 0; n < m->n_pins; n++) {
        minimap_pin_s *p      = &m->pins[n];
        texrec_s       tricon = asset_texrec(TEXID_BUTTONS,
                                             320, 32 + 32 * p->type, 32, 32);
        v2_i32         pos    = {p->x + ox - 16, p->y + oy - 16};
        gfx_spr(ctx, tricon, pos, 0, 0);
    }

    gfx_ctx_s ctxpin = ctx;
    ctxpin.pat       = gfx_pattern_interpolate(min_i32(m->pin_ui_fade, 48), 48);

    // draw pin selection to temporary texture first
    if (m->state == MINIMAP_ST_SELECT_PIN) {
        ALIGNAS(8) u32 px[(128 >> 5) * 32 * 2] = {0};

        tex_s tex_tmp     = {0};
        tex_tmp.w         = 128;
        tex_tmp.h         = 32;
        tex_tmp.wword     = (tex_tmp.w >> 5) << 1;
        tex_tmp.fmt       = 1;
        tex_tmp.px        = px;
        gfx_ctx_s ctx_tmp = gfx_ctx_default(tex_tmp);

#define PIN_SPACING 24

        // background of pin selection
        texrec_s tr1 = asset_texrec(TEXID_BUTTONS, 288, 0, 128, 32);
        gfx_spr(ctx_tmp, tr1, (v2_i32){(tex_tmp.w - tr1.w) / 2, 0}, 0, 0);

        v2_i32 pi = {(tex_tmp.w - MINIMAP_NUM_PIN_ICONS * PIN_SPACING) / 2, 0};

        // highlighted pin
        rec_i32 rfill = {pi.x + m->pin_selected * PIN_SPACING, 0,
                         PIN_SPACING, 32};

        switch (m->pin_selected) {
        case 0:
            rfill.w += 16;
            rfill.x -= 16;
            break;
        case MINIMAP_NUM_PIN_ICONS - 1:
            rfill.w += 16;
            break;
        }
        gfx_rec_fill(ctx_tmp, rfill, PRIM_MODE_WHITEN);

        // pins
        for (i32 n = 0; n < MINIMAP_NUM_PIN_ICONS; n++) {
            texrec_s tr2 = asset_texrec(TEXID_BUTTONS, 320, 32 + 32 * n, 32, 32);
            v2_i32   pos = {pi.x + n * PIN_SPACING - (32 - 24) / 2, pi.y};
            gfx_spr(ctx_tmp, tr2, pos, 0, 0);
        }

        // resulting image to display
        gfx_spr(ctxpin, texrec_from_tex(tex_tmp),
                (v2_i32){200 - tex_tmp.w / 2, 135}, 0, 0);
    }

    if (m->pin_ui_fade) {
        v2_i32 p1 = {200 - 25, 170};
        if (m->pin_deny_tick) {
            i32 offs = m->pin_deny_tick % 8;
            switch (offs) {
            case 0: offs = +0; break;
            case 1: offs = +1; break;
            case 2: offs = +2; break;
            case 3: offs = +1; break;
            case 4: offs = +0; break;
            case 5: offs = -1; break;
            case 6: offs = -2; break;
            case 7: offs = -1; break;
            }
            p1.x += offs;
        }
        texrec_s tr1 = asset_texrec(TEXID_BUTTONS, 128, 320, 9, 12);

        if (10 <= m->n_pins) {
            tr1.x = 128 + 9 * (m->n_pins / 10);
            gfx_spr(ctxpin, tr1, p1, 0, 0);
        }
        p1.x += 10;
        tr1.x = 128 + 9 * (m->n_pins % 10);
        gfx_spr(ctxpin, tr1, p1, 0, 0);
        p1.x += 10;
        tr1.x = 256;
        gfx_spr(ctxpin, tr1, p1, 0, 0);
        p1.x += 10;
        tr1.x = 128 + 9 * (MAP_NUM_PINS / 10);
        gfx_spr(ctxpin, tr1, p1, 0, 0);
        p1.x += 10;
        tr1.x = 128 + 9 * (MAP_NUM_PINS % 10);
        gfx_spr(ctxpin, tr1, p1, 0, 0);
    }

    // hero position marker
    if (ohero && (menu || ((g->tick_animation >> 4) & 3))) {
        texrec_s tricon  = asset_texrec(TEXID_BUTTONS, 288, 32, 32, 48);
        v2_i32   poshero = v2_i32_shr(obj_pos_center(ohero), 4);
        poshero.x += ox + g->map_room_cur->x - 16;
        poshero.y += oy + g->map_room_cur->y - 36;
        gfx_spr(ctx, tricon, poshero, 0, 0);
    }

    if (!menu) { // if fullscreen map draw cursor
        texrec_s tricon = asset_texrec(TEXID_BUTTONS, 448, 2 * 64, 64, 64);
        v2_i32   pos    = {200 - 32, 120 - 32};
        tricon.y += ((g->tick_animation >> 4) & 1) * 64;

        if (m->state == MINIMAP_ST_DELETE_PIN) {
            tricon.y        = 64 * 7;
            texrec_s trbar1 = asset_texrec(TEXID_BUTTONS, 352, 32, 38, 32);
            texrec_s trbar2 = asset_texrec(TEXID_BUTTONS, 352, 64, 38, 32);
            trbar2.w        = lerp_i32(0, trbar2.w, m->tick, MINIMAP_TICKS_PIN);
            v2_i32 pbar     = {200 - trbar1.w / 2, 135};
            gfx_spr(ctx, trbar1, pbar, 0, 0);
            gfx_spr(ctx, trbar2, pbar, 0, 0);
        } else if (m->pin_hovered || m->state == MINIMAP_ST_SELECT_PIN) {
            tricon.y += 64 * 2;

            if (m->state == MINIMAP_ST_SELECT_PIN) {
                texrec_s  tricon2 = asset_texrec(TEXID_BUTTONS,
                                                 320, 32 + m->pin_selected * 32,
                                                 32, 32);
                v2_i32    posicon = {pos.x + 16, pos.y + 16};
                gfx_ctx_s ctxicon = ctx;
                ctxicon.pat       = gfx_pattern_50();
                gfx_spr(ctxicon, tricon2, posicon, 0, 0);
            }
        } else if (minimap_hero_hovered(m, 0, 0)) {
            tricon.y = 64 * 4;
        }

        gfx_spr(ctx, tricon, pos, 0, 0);
    }
}

void minimap_draw(g_s *g)
{
    minimap_s *m   = &g->minimap;
    gfx_ctx_s  ctx = gfx_ctx_display();
    texrec_s   tr  = texrec_from_tex(asset_tex(TEXID_PAUSE_TEX));
    v2_i32     pos = {0};

    i32 ox = 200 - m->centerx;
    i32 oy = 120 - m->centery;

    switch (m->state) {
    case MINIMAP_ST_FADE_IN_MENU:
    case MINIMAP_ST_FADE_IN: {
        ctx.pat = gfx_pattern_interpolate(m->tick,
                                          MINIMAP_TICKS_FADE_IN);

        if (m->state == MINIMAP_ST_FADE_IN_MENU) {
            ox -= lerp_i32(100, 0, m->tick, MINIMAP_TICKS_FADE_IN);
        }
        break;
    }
    case MINIMAP_ST_FADE_OUT: {
        ctx.pat = gfx_pattern_interpolate(MINIMAP_TICKS_FADE_OUT - m->tick,
                                          MINIMAP_TICKS_FADE_OUT);
        break;
    }
    }

    minimap_draw_at(asset_tex(TEXID_PAUSE_TEX), g, ox, oy, 0);
    gfx_spr(ctx, tr, pos, 0, 0);
}

void minimap_draw_pause(g_s *g)
{
    minimap_s *m  = &g->minimap;
    v2_i32     ph = minimap_hero_pos(g);
    ph.x          = (ph.x + 1) & ~1;
    ph.y          = (ph.y + 1) & ~1;

    if (m->state) {
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

    u32 *v = m->visited;
    for (i32 n = 0; n < ARRLEN(m->visited); n++, v++) {
        *v |= (*v >> 1) & 0x55555555U; // write all permanently visited to temporary visited as well
        *v |= (*v << 1) & 0xAAAAAAAAU; // convert all temporary to permanently visited
    }
}

void minimap_try_visit_screen(g_s *g)
{
    sizeof(savefile_s);
    minimap_s *m  = &g->minimap;
    v2_i32     ph = minimap_hero_pos(g);
    i32        sx = ph.x / 25;
    i32        sy = ph.y / 15;
    assert(0 <= sx && sx < MINIMAP_SCREENS_X);
    assert(0 <= sy && sy < MINIMAP_SCREENS_Y);
    i32 k = (sx + sy * MINIMAP_SCREENS_X) << 1;
    m->visited[k >> 5] |= (u32)1 << (k & 31);
}

i32 minimap_screen_visited(g_s *g, i32 tx, i32 ty)
{
    minimap_s *m  = &g->minimap;
    i32        sx = tx / 25;
    i32        sy = ty / 15;
    assert(0 <= sx && sx < MINIMAP_SCREENS_X);
    assert(0 <= sy && sy < MINIMAP_SCREENS_Y);
    i32 k = (sx + sy * MINIMAP_SCREENS_X) << 1;
    i32 r = m->visited[k >> 5] >> (k & 31);
    return (r & 3);
}

v2_i32 minimap_hero_pos(g_s *g)
{
    v2_i32 p     = {0};
    obj_s *ohero = obj_get_hero(g);

    if (ohero) {
        v2_i32 ph = v2_i32_shr(obj_pos_center(ohero), 4);
        p.x       = g->map_room_cur->x + ph.x;
        p.y       = g->map_room_cur->y + ph.y;
    }
    return p;
}

void minimap_grav_dt(minimap_s *m, i32 dx, i32 dy)
{
    m->centerx += sgn_i32(dx) << 1;
    m->centery += sgn_i32(dy) << 1;
}

bool32 minimap_try_grav_to_hovered_pin(minimap_s *m)
{
    if (m->pin_hovered) {
        minimap_grav_dt(m,
                        m->pin_hovered->x - m->centerx,
                        m->pin_hovered->y - m->centery);
        return 1;
    }
    return 0;
}

bool32 minimap_hero_hovered(minimap_s *m, i32 *dx, i32 *dy)
{
    return 0;
    if (m->pin_hovered) return 0;

    i32 x = m->herox - m->centerx;
    i32 y = m->heroy - m->centery;
    if (x * x + y * y < MINIMAP_GRAV_DISTSQ) {
        if (dx) {
            *dx = x;
        }
        if (dy) {
            *dy = y;
        }
        return 1;
    }
    return 0;
}

bool32 minimap_try_grav_to_hero(minimap_s *m)
{
    return 0;
    i32 dx, dy;
    if (minimap_hero_hovered(m, &dx, &dy)) {
        minimap_grav_dt(m, dx, dy);
        return 1;
    }
    return 0;
}