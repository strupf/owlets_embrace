// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "inp.h"

static struct {
    inp_state_s curr;
    inp_state_s prev;
} INP;

void inp_update()
{
    INP.prev           = INP.curr;
    //
    INP.curr.crank_q12 = (int)(sys_crank() * 4096.f) & 0xFFF;
    INP.curr.btn       = sys_inp();
    INP.curr.btn |= sys_crank_docked() ? INP_CRANK_DOCKED : 0;
}

void inp_on_resume()
{
    inp_update();
    INP.prev = INP.curr;
}

inp_s inp_state()
{
    inp_s i = {0};
    i.curr  = INP.curr;
    i.prev  = INP.prev;
    return i;
}

bool32 inps_pressed(inp_s i, int b)
{
    return (i.curr.btn & b);
}

bool32 inps_was_pressed(inp_s i, int b)
{
    return (i.prev.btn & b);
}

bool32 inps_just_pressed(inp_s i, int b)
{
    return ((i.curr.btn & b) && !(i.prev.btn & b));
}

bool32 inps_just_released(inp_s i, int b)
{
    return (!(i.curr.btn & b) && (i.prev.btn & b));
}

int inps_dpad_x(inp_s i)
{
    if (inps_pressed(i, INP_DPAD_L)) return -1;
    if (inps_pressed(i, INP_DPAD_R)) return +1;
    return 0;
}

int inps_dpad_y(inp_s i)
{
    if (inps_pressed(i, INP_DPAD_U)) return -1;
    if (inps_pressed(i, INP_DPAD_D)) return +1;
    return 0;
}

bool32 inp_pressed(int b)
{
    return (INP.curr.btn & b);
}

bool32 inp_was_pressed(int b)
{
    return (INP.prev.btn & b);
}

bool32 inp_just_pressed(int b)
{
    return inp_pressed(b) && !inp_was_pressed(b);
}

bool32 inp_just_released(int b)
{
    return !inp_pressed(b) && inp_was_pressed(b);
}

int inp_dpad_x()
{
    if (inp_pressed(INP_DPAD_L)) return -1;
    if (inp_pressed(INP_DPAD_R)) return +1;
    return 0;
}

int inp_dpad_y()
{
    if (inp_pressed(INP_DPAD_U)) return -1;
    if (inp_pressed(INP_DPAD_D)) return +1;
    return 0;
}

int inp_dpad_dir()
{
    int x = inp_dpad_x();
    int y = inp_dpad_y();
    int d = ((y + 1) << 2) | (x + 1);
    switch (d) {
    case 1: return INP_DPAD_DIR_N;
    case 9: return INP_DPAD_DIR_S;
    case 6: return INP_DPAD_DIR_E;
    case 4: return INP_DPAD_DIR_W;
    case 10: return INP_DPAD_DIR_SE;
    case 8: return INP_DPAD_DIR_SW;
    case 0: return INP_DPAD_DIR_NW;
    case 2: return INP_DPAD_DIR_NE;
    }
    return INP_DPAD_DIR_NONE;
}

int inp_crank_q12()
{
    return INP.curr.crank_q12;
}

int inp_prev_crank_q12()
{
    return INP.prev.crank_q12;
}

int inp_crank_dt_q12()
{
    return inp_crank_calc_dt_q12(INP.prev.crank_q12, INP.curr.crank_q12);
}

int inp_crank_calc_dt_q12(int ang_from, int ang_to)
{
    int dt = ang_to - ang_from;
    if (dt <= -2048) return (dt + 4096);
    if (dt >= +2048) return (dt - 4096);
    return dt;
}

int inp_crank_docked()
{
    return (INP.curr.btn & INP_CRANK_DOCKED);
}

int inp_crank_was_docked()
{
    return (INP.prev.btn & INP_CRANK_DOCKED);
}

int inp_crank_just_docked()
{
    return inp_crank_docked() && !inp_crank_was_docked();
}

int inp_crank_just_undocked()
{
    return !inp_crank_docked() && inp_crank_was_docked();
}

int inp_debug_space()
{
    return backend_debug_space();
}