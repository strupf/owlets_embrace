#ifndef OS_H
#define OS_H

#include "os_audio.h"
#include "os_fileio.h"
#include "os_graphics.h"
#include "os_math.h"
#include "os_mem.h"
#include "os_types.h"

#if defined(TARGET_PD)
enum {
        INP_LEFT  = kButtonLeft,
        INP_RIGHT = kButtonRight,
        INP_UP    = kButtonUp,
        INP_DOWN  = kButtonDown,
        INP_A     = kButtonA,
        INP_B     = kButtonB,
};
#elif defined(TARGET_DESKTOP)
enum {
        INP_LEFT  = 0x01,
        INP_RIGHT = 0x02,
        INP_UP    = 0x04,
        INP_DOWN  = 0x08,
        INP_A     = 0x10,
        INP_B     = 0x20,
};
#endif

bool32 os_inp_pressed(int b);
bool32 os_inp_pressedp(int b);
bool32 os_inp_just_released(int b);
bool32 os_inp_just_pressed(int b);
int    os_inp_crank_change();
int    os_inp_crank();
int    os_inp_crankp();
bool32 os_inp_crank_dockedp();
bool32 os_inp_crank_docked();

#endif