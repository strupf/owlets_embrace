// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "item_select.h"
#include "game.h"

#define ITEM_SELECT_SNAP_ANG_Q12 (256 << 4)
#define ITEM_SELECT_SNAP_VEL_Q12 (400 << 4)

void item_select_turn_by(item_select_s *i, i32 dt_q16);

void item_select_update(item_select_s *i)
{
    if (inp_action(INP_A)) {
        i->docked = 0;
        return;
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
    if (dt_q16 == 0) return;

    i32 ac     = i->ang_q16;          // [0, 4096)
    i32 an     = i->ang_q16 + dt_q16; // can be < 0 and >= 4096
    i->ang_q16 = an & 0xFFFF;

    if (0 < dt_q16 && ((ac < 0x4000 && 0x4000 <= an) ||
                       (ac < 0xC000 && 0xC000 <= an))) {
        i->item = (i->item + 1) % i->n_items;
    }
    if (0 > dt_q16 && ((an < 0x4000 && 0x4000 <= ac) ||
                       (an < 0xC000 && 0xC000 <= ac))) {
        i->item = (i->item - 1 + i->n_items) % i->n_items;
    }
}

void item_select_draw(item_select_s *i)
{
    if (i->n_items <= 1) return;
    i32 q     = i->ang_q16 >> 14; // angle quadrant [0, 3]
    i32 itemp = ((i->item + ((q & 1) ? -1 : +1)) + i->n_items) % i->n_items;
    i32 itemc = i->item;

    i32       xx  = (100 * +sin_q16(i->ang_q16 << 2)) >> 16;
    i32       yy  = (100 * -cos_q16(i->ang_q16 << 2)) >> 16;
    gfx_ctx_s ctx = gfx_ctx_display();
    ctx.clip_x1   = 400 - 32;
    ctx.clip_y1   = 240 - 32;

    fnt_s font = asset_fnt(FNTID_LARGE);

    i32 offs = (16 * (((i->ang_q16 + 0x4000) % 0x8000) - 0x4000)) / 0x4000;

    char str1[2] = {0};
    char str2[2] = {0};
    str_from_u32(itemc, str1, sizeof(str1));
    str_from_u32(itemp, str2, sizeof(str2));
#if 0
    fnt_draw_ascii(ctx, font, (v2_i32){400 - 32, 240 - 32 + offs}, str1, GFX_COL_BLACK);
    fnt_draw_ascii(ctx, font, (v2_i32){400 - 32, 240 - 32 + offs + 32}, str2, GFX_COL_BLACK);
#endif
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