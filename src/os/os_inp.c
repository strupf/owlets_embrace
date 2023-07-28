/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "os_internal.h"

#ifdef TARGET_DESKTOP
void os_backend_inp_init()
{
}

void os_backend_inp_update()
{
        g_os.buttonsp = g_os.buttons;
        g_os.buttons  = 0;
        if (IsKeyDown(KEY_A)) {
                g_os.buttons |= INP_LEFT;
        }
        if (IsKeyDown(KEY_D)) {
                g_os.buttons |= INP_RIGHT;
        }
        if (IsKeyDown(KEY_W)) {
                g_os.buttons |= INP_UP;
        }
        if (IsKeyDown(KEY_S)) {
                g_os.buttons |= INP_DOWN;
        }
        if (IsKeyDown(KEY_COMMA)) {
                g_os.buttons |= INP_B;
        }
        if (IsKeyDown(KEY_PERIOD)) {
                g_os.buttons |= INP_A;
        }
        g_os.crankdockedp = g_os.crankdocked;
        g_os.crankdocked  = 1;
        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = 0;
        }
}

bool32 debug_inp_up() { return IsKeyDown(KEY_UP); }
bool32 debug_inp_down() { return IsKeyDown(KEY_DOWN); }
bool32 debug_inp_left() { return IsKeyDown(KEY_LEFT); }
bool32 debug_inp_right() { return IsKeyDown(KEY_RIGHT); }
bool32 debug_inp_w() { return IsKeyDown(KEY_W); }
bool32 debug_inp_a() { return IsKeyDown(KEY_A); }
bool32 debug_inp_s() { return IsKeyDown(KEY_S); }
bool32 debug_inp_d() { return IsKeyDown(KEY_D); }
bool32 debug_inp_enter() { return IsKeyDown(KEY_ENTER); }
bool32 debug_inp_space() { return IsKeyDown(KEY_SPACE); }

#endif
#ifdef TARGET_PD
static float (*PD_i_crank)(void);
static int (*PD_i_crankdocked)(void);
static void (*PD_i_buttonstate)(PDButtons *, PDButtons *, PDButtons *);

void os_backend_inp_init()
{
        PD_i_crank       = PD->system->getCrankAngle;
        PD_i_crankdocked = PD->system->isCrankDocked;
        PD_i_buttonstate = PD->system->getButtonState;
}

void os_backend_inp_update()
{
        g_os.buttonsp     = g_os.buttons;
        g_os.crankdockedp = g_os.crankdocked;
        PD_i_buttonstate(&g_os.buttons, NULL, NULL);
        g_os.crankdocked = PD_i_crankdocked();

        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = (int)(PD_i_crank() * 182.04444f) & 0xFFFF;
        }
}

bool32 debug_inp_up() { return os_inp_pressed(INP_UP); }
bool32 debug_inp_down() { return os_inp_pressed(INP_DOWN); }
bool32 debug_inp_left() { return os_inp_pressed(INP_LEFT); }
bool32 debug_inp_right() { return os_inp_pressed(INP_RIGHT); }
bool32 debug_inp_w() { return 0; }
bool32 debug_inp_a() { return 0; }
bool32 debug_inp_s() { return 0; }
bool32 debug_inp_d() { return 0; }
bool32 debug_inp_enter() { return 0; }
bool32 debug_inp_space() { return 0; }

#endif

bool32 os_inp_pressed(int b)
{
        return (g_os.buttons & b);
}

bool32 os_inp_pressedp(int b)
{
        return (g_os.buttonsp & b);
}

bool32 os_inp_just_released(int b)
{
        return (!(g_os.buttons & b) && (g_os.buttonsp & b));
}

bool32 os_inp_just_pressed(int b)
{
        return ((g_os.buttons & b) && !(g_os.buttonsp & b));
}

int os_inp_crank_change()
{
        return 0;
}

int os_inp_crank()
{
        return g_os.crank_q16;
}

int os_inp_crankp()
{
        return g_os.crankp_q16;
}

bool32 os_inp_crank_dockedp()
{
        return g_os.crankdockedp;
}

bool32 os_inp_crank_docked()
{
        return g_os.crankdocked;
}