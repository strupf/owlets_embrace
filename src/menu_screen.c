// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render/render.h"

#define MAP_PIN_DELETE_TICKS 25

v2_i32 mapview_hero_world_q8(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    v2_i32 vres  = {0};
    if (!ohero) return vres;

    v2_i32 hc    = obj_pos_center(ohero);
    i32    rposx = (hc.x / SYS_DISPLAY_W) * SYS_DISPLAY_W + SYS_DISPLAY_W / 2;
    i32    rposy = (hc.y / SYS_DISPLAY_H) * SYS_DISPLAY_H + SYS_DISPLAY_H / 2;
    vres.x       = ((g->map_worldroom->x + rposx) << 8) / SYS_DISPLAY_W;
    vres.y       = ((g->map_worldroom->y + rposy) << 8) / SYS_DISPLAY_H;
    return vres;
}

void menu_screen_update(game_s *g, menu_screen_s *m)
{
    if (inp_just_pressed(INP_B)) {
        g->substate = 0;
        return;
    }

    if (m->map.fade_tick) {
        m->map.fade_tick--;

        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (ohero) {
            v2_i32 targetpos = mapview_hero_world_q8(g);
            m->map.pos       = v2_lerp(m->map.pos, targetpos, 1, 4);
        }
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
            if (inp_pressed(INP_A) && m->map.pin_delete_tick && m->map.pin) {
                m->map.pin_delete_tick++;
                if (MAP_PIN_DELETE_TICKS <= m->map.pin_delete_tick) {
                    *m->map.pin            = g->save.map_pins[--g->save.n_map_pins];
                    m->map.pin_delete_tick = 0;
                }
                break;
            }

            i32 scl_q8             = m->map.scl_q12 >> 4;
            u32 closest_dist       = 50; // snap distance squared
            m->map.pin             = NULL;
            m->map.pin_delete_tick = 0;
            for (int n = 0; n < g->save.n_map_pins; n++) {
                map_pin_s *p      = &g->save.map_pins[n];
                v2_i32     pinpos = mapview_screen_from_world_q8(p->pos, m->map.pos, 400, 240, scl_q8);
                u32        dist   = v2_distancesq(ctr_screen, pinpos);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    m->map.pin   = p;
                }
            }

            m->map.scl_q12 += inp_crank_dt_q12() << 5;
            m->map.scl_q12  = max_i(m->map.scl_q12, 12 << 8);
            int    dpadx    = inp_dpad_x();
            int    dpady    = inp_dpad_y();
            bool32 pin_snap = (closest_dist < 5);

            if (m->map.pin && (dpadx | dpady) == 0) {
                if (pin_snap) {
                    m->map.pos = m->map.pin->pos;
                    m->map.cursoranimtick++;
                } else {
                    m->map.pos = v2_lerp(m->map.pos, m->map.pin->pos, 1, 4);
                }
            } else {
                m->map.cursoranimtick = 0;
                m->map.pos.x += dpadx << 4;
                m->map.pos.y += dpady << 4;
            }

            if (inp_just_pressed(INP_A)) {

                if (m->map.pin && pin_snap) {
                    m->map.pin_delete_tick = 1; // start holding A
                } else if (g->save.n_map_pins < NUM_MAP_PINS) {
                    m->map.mode           = MAP_MODE_PIN_SELECT;
                    m->map.pin_type       = 0;
                    m->map.cursoranimtick = 0;
                }
            }
            break;
        }
        case MAP_MODE_PIN_SELECT: {
            m->map.pin_delete_tick = 0;
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
        render_map(g, ctx, 0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H, scl_q8, m->map.pos);
        v2_i32 herop_world  = mapview_hero_world_q8(g);
        v2_i32 herop_screen = mapview_screen_from_world_q8(herop_world, m->map.pos,
                                                           SYS_DISPLAY_W, SYS_DISPLAY_H, scl_q8);

        gfx_ctx_s ctx_cir = ctx;
        ctx_cir.pat       = gfx_pattern_interpolate(sin_q16(time_now() << 13) + 65526, 65536 * 2);
        herop_screen.x -= 16;
        herop_screen.y -= 16;
        texrec_s theropin = asset_texrec(TEXID_UI, 320, 160, 32, 32);
        gfx_spr(ctx_cir, theropin, herop_screen, 0, 0);

        v2_i32 cursorpos = {SYS_DISPLAY_W / 2 - 16, SYS_DISPLAY_H / 2 - 16};
        if (m->map.mode == MAP_MODE_PIN_SELECT) {
            texrec_s tpinselected = asset_texrec(TEXID_UI, 256 + m->map.pin_type * 32, 192, 32, 32);
            gfx_spr(ctx, tpinselected, cursorpos, 0, 0);

            i32     pinoffsetx = SYS_DISPLAY_W / 2 - (NUM_MAP_PIN_TYPES * 32) / 2;
            i32     pinoffsety = 200;
            rec_i32 rselected  = {pinoffsetx + m->map.pin_type * 32, pinoffsety, 32, 32};
            gfx_rec_fill(ctx, rselected, PRIM_MODE_WHITE);

            for (int n = 0; n < NUM_MAP_PIN_TYPES; n++) {
                texrec_s tpin   = asset_texrec(TEXID_UI, 256 + n * 32, 192, 32, 32);
                v2_i32   pospin = {pinoffsetx + n * 32, pinoffsety};
                gfx_spr(ctx, tpin, pospin, 0, 0);
            }
        }

        if (m->map.pin_delete_tick) {
            rec_i32 rbar_1 = {240, 120, 16, 32};
            i32     bh     = ((rbar_1.h - 4) * m->map.pin_delete_tick) / MAP_PIN_DELETE_TICKS;
            rec_i32 rbar_2 = {rbar_1.x + 2, rbar_1.y + 2, rbar_1.w - 4, bh};
            gfx_rec_fill(ctx, rbar_1, PRIM_MODE_BLACK);
            gfx_rec_fill(ctx, rbar_2, PRIM_MODE_WHITE);
        }

        texrec_s tr_cursor  = {tui, {0, 0, 16, 16}};
        i32      cursoranim = 2 + ((sin_q16(m->map.cursoranimtick << 13) * 2) >> 16);

        for (int y = -1; y <= 1; y += 2) {
            for (int x = -1; x <= 1; x += 2) {
                v2_i32 pcur = cursorpos;
                pcur.x += (0 < x) * 16 + x * cursoranim;
                pcur.y += (0 < y) * 16 + y * cursoranim;
                tr_cursor.r.x = 224 + (0 < x) * 16;
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
    g->substate      = SUBSTATE_MENUSCREEN;
    m->tab           = MENU_SCREEN_TAB_MAP;
    m->map.scl_q12   = MAPVIEW_SCL_DEFAULT << 12;
    m->map.fade_tick = 25;
    if (obj_get_tagged(g, OBJ_TAG_HERO)) {
        m->map.pos = mapview_hero_world_q8(g);
        m->map.pos.x += (100 << 8) / MAPVIEW_SCL_DEFAULT; // offset to match menu screen
    }
}