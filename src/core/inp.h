// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INP_H
#define INP_H

#include "pltf/pltf.h"

enum {
    INP_DPAD_DIR_NONE,
    INP_DPAD_DIR_N,
    INP_DPAD_DIR_S,
    INP_DPAD_DIR_E,
    INP_DPAD_DIR_W,
    INP_DPAD_DIR_NE,
    INP_DPAD_DIR_NW,
    INP_DPAD_DIR_SE,
    INP_DPAD_DIR_SW,
};

#define PLTF_SDL_EMULATE_SIM 1

enum {
#ifdef PLTF_PD
    INP_CRANK_DOCK = 1 << 15,
    INP_A          = PLTF_PD_BTN_A,
    INP_B          = PLTF_PD_BTN_B,
    INP_DL         = PLTF_PD_BTN_DL,
    INP_DR         = PLTF_PD_BTN_DR,
    INP_DU         = PLTF_PD_BTN_DU,
    INP_DD         = PLTF_PD_BTN_DD,
    //
    INP_CONFIRM    = INP_A,
    INP_CANCEL     = INP_B,
    INP_JUMP       = INP_A,
    INP_ITEM       = INP_B
#elif PLTF_SDL_EMULATE_SIM
    INP_CRANK_DOCK = 1 << 15,
    INP_A          = 1 << 0,
    INP_B          = 1 << 1,
    INP_DL         = 1 << 2,
    INP_DR         = 1 << 3,
    INP_DU         = 1 << 4,
    INP_DD         = 1 << 5,
    //
    INP_CONFIRM    = INP_A,
    INP_CANCEL     = INP_B,
    INP_JUMP       = INP_A,
    INP_ITEM       = INP_B
#else
#error INP NOT IMPLEMENTED
    INP_DL        = 1 << 0,
    INP_DR        = 1 << 1,
    INP_DU        = 1 << 2,
    INP_DD        = 1 << 3,
    INP_CONFIRM   = 1 << 4,
    INP_CANCEL    = 1 << 5,
    INP_JUMP      = 1 << 6,
    INP_ITEM_1    = 1 << 7,
    INP_ITEM_2    = 1 << 8,
    INP_ITEM_NEXT = 1 << 9,
    INP_ITEM_PREV = 1 << 10,
    INP_INVENTORY = 1 << 11,
    INP_PAUSE     = 1 << 12
#endif
};

typedef struct {
    u16 actions;
    u16 crank_q16;
} inp_new_s;

i32    inp_x();    // [-1,+1]
i32    inp_y();    // [-1,+1]
i32    inp_xp();   // [-1,+1]
i32    inp_yp();   // [-1,+1]
v2_i32 inp_dir();  // [-1,+1]
v2_i32 inp_dirp(); // [-1,+1]
bool32 inp_action(i32 b);
bool32 inp_actionp(i32 b);
bool32 inp_action_jp(i32 b);
bool32 inp_action_jr(i32 b);
i32    inp_crank_q16();      // curr crank angle in Q16, turns
i32    inp_crankp_q16();     // prev crank angle in Q16, turns
i32    inp_crank_qx(i32 q);  // curr crank angle in QXX, turns
i32    inp_crankp_qx(i32 q); // prev crank angle in QXX, turns
i32    inp_crank_dt_q16();
i32    inp_crank_calc_dt_q16(i32 ang_from, i32 ang_to);
i32    inp_crank_calc_dt_qx(i32 q, i32 ang_from, i32 ang_to);

void inp_update();
void inp_on_resume();

#endif