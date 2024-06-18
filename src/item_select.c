// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "item_select.h"
#include "game.h"

#define ITEM_SELECT_SNAP_ANG_Q12 (256 << 4)
#define ITEM_SELECT_SNAP_VEL_Q12 (400 << 4)

void item_select_turn_by(item_select_s *i, i32 dt_q16);

void item_select_init(item_select_s *i)
{
    inp_crank_click_init(&i->crank_click, 2, 0);
    i->n_items = 2;
#ifdef PLTF_PD
    inp_crank_click_turn_by(&i->crank_click, inp_crank_q16());
    i->ang_q16 = i->crank_click.ang;
#endif
}

void item_select_update(item_select_s *i)
{
    if (i->tick_item_scroll) {
        i->tick_item_scroll -= sgn_i32(i->tick_item_scroll);
    }

    if (i->n_items <= 1) return;
    if (i->docked) {
        item_select_turn_by(i, inp_crank_dt_q16());
        return;
    }

    i32 a        = inp_crank_q16();
    i32 dock_a   = (i->ang_q16);
    i32 dock_b   = (i->ang_q16 + 0x8000) & 0xFFFF;
    i32 dt_a     = inp_crank_calc_dt_q16(dock_a, a);
    i32 dt_b     = inp_crank_calc_dt_q16(dock_b, a);
    i32 dt_a_abs = abs_i32(dt_a);
    i32 dt_b_abs = abs_i32(dt_b);

    if (dt_a_abs < ITEM_SELECT_SNAP_ANG_Q12 ||
        dt_b_abs < ITEM_SELECT_SNAP_ANG_Q12) {
        i32 dt_min = min_i32(dt_a_abs, dt_b_abs);
        i32 dir    = sgn_i32(dt_a_abs < dt_b_abs ? dt_a : dt_b);
        i32 dt     = dir * ((ITEM_SELECT_SNAP_VEL_Q12 * (ITEM_SELECT_SNAP_ANG_Q12 - dt_min)) /
                        ITEM_SELECT_SNAP_ANG_Q12);

        if (dt_min < dt || dt < -dt_min) {
            dt        = clamp_i32(dt, -dt_min, +dt_min);
            i->docked = 1;
            pltf_log("docked!\n");
        }
        item_select_turn_by(i, dt);
    }
}

void item_select_turn_by(item_select_s *i, i32 dt_q16)
{
    i32 di     = inp_crank_click_turn_by(&i->crank_click, dt_q16);
    i->item    = (((i32)i->item + di) + i->n_items) % i->n_items;
    i->ang_q16 = i->crank_click.ang;

    if (di) {
        pltf_log("ITEM: %i\n", i->item);
        i->tick_item_scroll = ITEM_SELECT_SCROLL_TICK * sgn_i32(di);
    }
}

void item_select_draw(item_select_s *i)
{
    if (i->n_items <= 1) return;
}

i32 item_select_clutch_dist_x_q16(item_select_s *i)
{
    if (i->docked) return 0;

    i32 a        = inp_crank_q16();
    i32 dock_a   = (i->ang_q16);
    i32 dock_b   = (i->ang_q16 + 0x8000) & 0xFFFF;
    i32 dt_a     = inp_crank_calc_dt_q16(dock_a, a);
    i32 dt_b     = inp_crank_calc_dt_q16(dock_b, a);
    i32 dt_a_abs = abs_i32(dt_a);
    i32 dt_b_abs = abs_i32(dt_b);
    i32 dt_min   = min3_i32(dt_a_abs, dt_b_abs, ITEM_SELECT_SNAP_ANG_Q12);
    return (((1 << 16) * (ITEM_SELECT_SNAP_ANG_Q12 - dt_min)) / ITEM_SELECT_SNAP_ANG_Q12);
}