// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render/render.h"

bool32 menu_screen_active(menu_screen_s *m)
{
    return m->active;
}

void menu_screen_update(game_s *g, menu_screen_s *m)
{
    if (inp_just_pressed(INP_B)) {
        m->active = 0;
        return;
    }

#if 0
    m->tab_ticks++;

    m->tab_selected = 0; // debug
    if (m->tab_selected) {
        const i32 t = m->tab;
        if (inp_just_pressed(INP_DPAD_L)) {
            m->tab--;
        }
        if (inp_just_pressed(INP_DPAD_R)) {
            m->tab++;
        }
        m->tab = (m->tab + NUM_MENU_SCREEN_TABS) % NUM_MENU_SCREEN_TABS;
        if (m->tab != t) {
            m->tab_ticks = 0;
        }
    }
#endif

    v2_i32 ctr_screen = {SYS_DISPLAY_W / 2, SYS_DISPLAY_H / 2};

    switch (m->tab) {
    case MENU_SCREEN_TAB_MAP: {
        switch (m->map.mode) {
        case MAP_MODE_SCROLL: {
            i32        scl_q8        = m->map.scl_q12 >> 4;
            u32        closest__dist = 40; // snap distance squared
            map_pin_s *closest_pin   = NULL;
            for (int n = 0; n < g->save.n_map_pins; n++) {
                map_pin_s *p      = &g->save.map_pins[n];
                v2_i32     pinpos = mapview_screen_from_world_q8(p->pos, m->map.pos, 400, 240, scl_q8);
                u32        dist   = v2_distancesq(ctr_screen, pinpos);
                if (dist < closest__dist) {
                    closest__dist = dist;
                    closest_pin   = p;
                }
            }

            m->map.scl_q12 += inp_crank_dt_q12() << 5;
            m->map.scl_q12 = max_i(m->map.scl_q12, 12 << 8);
            int dpadx      = inp_dpad_x();
            int dpady      = inp_dpad_y();
            if (closest_pin && (dpadx | dpady) == 0) {
                m->map.pos = closest_pin->pos;
                m->map.cursoranimtick++;
            } else {
                m->map.cursoranimtick = 0;
                m->map.pos.x += dpadx << 4;
                m->map.pos.y += dpady << 4;
            }

            if (inp_just_pressed(INP_A)) {
                if (closest_pin) {
                    *closest_pin = g->save.map_pins[--g->save.n_map_pins];
                } else {
                    m->map.mode           = MAP_MODE_PIN_SELECT;
                    m->map.pin_type       = 0;
                    m->map.cursoranimtick = 0;
                }
            }
            break;
        }
        case MAP_MODE_PIN_SELECT: {
            m->map.cursoranimtick++;
            if (inp_just_pressed(INP_B)) {
                m->map.mode           = MAP_MODE_SCROLL;
                m->map.cursoranimtick = 0;
                break;
            }
            if (inp_just_pressed(INP_A)) {
                map_pin_s *pin = &g->save.map_pins[g->save.n_map_pins++];

                pin->pos    = mapview_world_q8_from_screen(ctr_screen, m->map.pos,
                                                           400, 240, m->map.scl_q12 >> 4);
                pin->type   = m->map.pin_type;
                m->map.mode = MAP_MODE_SCROLL;
                break;
            }

            if (inp_just_pressed(INP_DPAD_L)) {
                m->map.pin_type--;
                m->map.pin_type = max_i(m->map.pin_type, 0);
            }
            if (inp_just_pressed(INP_DPAD_R)) {
                m->map.pin_type++;
                m->map.pin_type = min_i(m->map.pin_type, NUM_MAP_PIN_TYPES - 1);
            }
            break;
        }
        }

        break;
    }
    case MENU_SCREEN_TAB_INVENTORY: {
        break;
    }
    }
}

void menu_screen_draw(game_s *g, menu_screen_s *m)
{
    const gfx_ctx_s ctx = gfx_ctx_display();
    tex_s           tui = asset_tex(TEXID_UI);
    tex_clr(ctx.dst, GFX_COL_BLACK);

    switch (m->tab) {
    case MENU_SCREEN_TAB_MAP: {
        i32 scl_q8 = m->map.scl_q12 >> 4;
        render_map(g, ctx, SYS_DISPLAY_W, SYS_DISPLAY_H, scl_q8, m->map.pos);
        v2_i32 herop_world = {
            ((g->map_worldroom->x + g->map_worldroom->w / 2) << 8) / SYS_DISPLAY_W,
            ((g->map_worldroom->y + g->map_worldroom->h / 2) << 8) / SYS_DISPLAY_H};
        v2_i32 herop_screen = mapview_screen_from_world_q8(herop_world, m->map.pos,
                                                           SYS_DISPLAY_W, SYS_DISPLAY_H, scl_q8);

        gfx_ctx_s ctx_cir = ctx;
        ctx_cir.pat       = gfx_pattern_interpolate(sin_q16(time_now() << 12) + 65526, 65536 * 2);
        gfx_cir_fill(ctx_cir, herop_screen, scl_q8 >> 9, PRIM_MODE_WHITE);

        v2_i32 cursorpos = {SYS_DISPLAY_W / 2 - 16, SYS_DISPLAY_H / 2 - 16};
        if (m->map.mode == MAP_MODE_PIN_SELECT) {
            texrec_s tpinselected = asset_texrec(TEXID_UI, 256 + m->map.pin_type * 32, 192, 32, 32);
            gfx_spr(ctx, tpinselected, cursorpos, 0, 0);

            rec_i32 rselected = {200 + m->map.pin_type * 32, 200, 32, 32};
            gfx_rec_fill(ctx, rselected, PRIM_MODE_WHITE);

            for (int n = 0; n < NUM_MAP_PIN_TYPES; n++) {
                texrec_s tpin   = asset_texrec(TEXID_UI, 256 + n * 32, 192, 32, 32);
                v2_i32   pospin = {200 + n * 32, 200};
                gfx_spr(ctx, tpin, pospin, 0, 0);
            }
        }

        texrec_s tr_cursor  = {tui, {0, 0, 16, 16}};
        i32      cursoranim = 2 + ((sin_q16(m->map.cursoranimtick << 13) * 2) >> 16);

        for (int y = -1; y <= 1; y += 2) {
            for (int x = -1; x <= 1; x += 2) {
                v2_i32 pcur = cursorpos;
                pcur.x += (0 < x) * 16 + x * cursoranim;
                pcur.y += (0 < y) * 16 + y * cursoranim;
                tr_cursor.r.x = 320 + (0 < x) * 16;
                tr_cursor.r.y = 192 + (0 < y) * 16;

                gfx_spr(ctx, tr_cursor, pcur, 0, 0);
            }
        }
    }
    case MENU_SCREEN_TAB_INVENTORY: {
        break;
    }
    }
}

void menu_screen_open(game_s *g, menu_screen_s *m)
{
    m->active      = 1;
    m->tab         = MENU_SCREEN_TAB_MAP;
    m->map.scl_q12 = 32 << 12;
    m->map.pos.x   = ((g->map_worldroom->x + g->map_worldroom->w / 2) << 8) / SYS_DISPLAY_W;
    m->map.pos.y   = ((g->map_worldroom->y + g->map_worldroom->h / 2) << 8) / SYS_DISPLAY_H;
    m->map.pos.x += (100 << 8) / (m->map.scl_q12 >> 12);
}