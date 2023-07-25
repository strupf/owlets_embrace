#ifndef OS_H
#define OS_H

#include "os_fileio.h"
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

int    os_inp_crank_change();
int    os_inp_crank();
int    os_inp_crankp();
bool32 os_inp_crank_dockedp();
bool32 os_inp_crank_docked();
//
void   scratchmem_push();
void   scratchmem_pop();
void   scratchmem_clr();
void  *scratchmem_alloc(size_t size);
void  *scratchmem_alloc_remaining();
void  *scratchmem_alloc_remainings(size_t *size);
void  *scratchmem_alloc_zero(size_t size);
void  *scratchmem_alloc_zero_remaining();
void  *scratchmem_alloc_zero_remainings(size_t *size);

#endif