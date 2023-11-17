// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "inp.h"

typedef struct {
    int btn;
    int crank_q16;
    int crank_docked;
} inp_state_s;

static struct {
    inp_state_s curr;
    inp_state_s prev;
} INP;

void inp_update()
{
    INP.prev              = INP.curr;
    INP.curr.btn          = sys_inp();
    INP.curr.crank_q16    = (int)(sys_crank() * 65536.f);
    INP.curr.crank_docked = sys_crank_docked();
}

bool32 inp_pressed(int b)
{
    return (INP.curr.btn & b);
}

bool32 inp_pressed_any(int b)
{
    return (INP.curr.btn & b) != 0;
}

bool32 inp_pressed_all(int b)
{
    return (INP.curr.btn & b) == b;
}

bool32 inp_was_pressed(int b)
{
    return (INP.prev.btn & b);
}

bool32 inp_was_pressed_any(int b)
{
    return (INP.prev.btn & b) != 0;
}

bool32 inp_was_pressed_all(int b)
{
    return (INP.prev.btn & b) == b;
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

int inp_crank_q16()
{
    return INP.curr.crank_q16;
}

int inp_prev_crank_q16()
{
    return INP.prev.crank_q16;
}

int inp_crank_dt_q16()
{
    i32 dt = INP.curr.crank_q16 - INP.prev.crank_q16;
    if (dt <= -32768) return (dt + 65536);
    if (dt >= +32768) return (dt - 65536);
    return dt;
}

int inp_crank_docked()
{
    return INP.curr.crank_docked;
}

int inp_crank_was_docked()
{
    return INP.prev.crank_docked;
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