// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

#ifdef TARGET_DESKTOP
void os_backend_inp_init()
{
}

void os_backend_inp_update()
{
        g_os.buttonsp = g_os.buttons;
        g_os.buttons  = 0;

        if (IsKeyDown(KEY_A)) g_os.buttons |= INP_LEFT;
        if (IsKeyDown(KEY_D)) g_os.buttons |= INP_RIGHT;
        if (IsKeyDown(KEY_W)) g_os.buttons |= INP_UP;
        if (IsKeyDown(KEY_S)) g_os.buttons |= INP_DOWN;
        if (IsKeyDown(KEY_COMMA)) g_os.buttons |= INP_B;
        if (IsKeyDown(KEY_PERIOD)) g_os.buttons |= INP_A;

        /* only for testing purposes because emulating Playdate buttons
         * on a keyboard is weird */
        if (IsGamepadAvailable(0)) {
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -.1f)
                        g_os.buttons |= INP_LEFT;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > +.1f)
                        g_os.buttons |= INP_RIGHT;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) < -.1f)
                        g_os.buttons |= INP_UP;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) > +.1f)
                        g_os.buttons |= INP_DOWN;
                if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                        g_os.buttons |= INP_B;
                if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                        g_os.buttons |= INP_A;
        }

        g_os.crankdockedp = g_os.crankdocked;
        g_os.crankdocked  = 0;
        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                if (GetMouseWheelMove() != 0.f) {
                        g_os.crank_q16 += (int)(5000.f * GetMouseWheelMove());
                } else {
                        if (IsKeyDown(KEY_E)) g_os.crank_q16 += 2048;
                        if (IsKeyDown(KEY_Q)) g_os.crank_q16 -= 2048;
                }

                g_os.crank_q16 &= 0xFFFF;
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

/* NW  N  NE         Y -1
 *   \ | /           |
 * W__\|/___E   -1 --+--> X +1
 *    /|\            |
 *   / | \           V +1
 * SW  S  SE
 */
int os_inp_dpad_direction()
{
        int m = ((os_inp_dpad_x() + 1) << 2) | (os_inp_dpad_y() + 1);
        switch (m) {
        case 0x0: return INP_DPAD_NW;   // 0000 (-1 | -1)
        case 0x1: return INP_DPAD_W;    // 0001 (-1 |  0)
        case 0x2: return INP_DPAD_SW;   // 0010 (-1 | +1)
        case 0x4: return INP_DPAD_N;    // 0100 ( 0 | -1)
        case 0x5: return INP_DPAD_NONE; // 0101 ( 0 |  0)
        case 0x6: return INP_DPAD_S;    // 0110 ( 0 | +1)
        case 0x8: return INP_DPAD_NE;   // 1000 (+1 | -1)
        case 0x9: return INP_DPAD_E;    // 1001 (+1 |  0)
        case 0xA: return INP_DPAD_SE;   // 1010 (+1 | +1)
        }
        ASSERT(0);
        return 0;
}

int os_inp_dpad_x()
{
        if (os_inp_pressed(INP_LEFT))
                return -1;
        if (os_inp_pressed(INP_RIGHT))
                return +1;
        return 0;
}

int os_inp_dpad_y()
{
        if (os_inp_pressed(INP_UP))
                return -1;
        if (os_inp_pressed(INP_DOWN))
                return +1;
        return 0;
}

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
        i32 dt = g_os.crank_q16 - g_os.crankp_q16;
        if (dt <= -32768) return (dt + 65536);
        if (dt >= +32768) return (dt - 65536);
        return dt;
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