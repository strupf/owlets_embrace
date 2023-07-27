#include "os_internal.h"

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