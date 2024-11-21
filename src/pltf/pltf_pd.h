// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_PD_H
#define PLTF_PD_H

#undef TARGET_EXTENSION
#define TARGET_EXTENSION 1

#include "PD/pd_api.h"
#include "pltf_types.h"

#if 0
extern void (*PD_system_logToConsole)(const char *fmt, ...);
extern void *(*PD_system_realloc)(void *ptr, size_t s);
extern int (*PD_system_formatString)(char **outstr, const char *fmt, ...);

#define pltf_log(...)                                 \
    {                                                 \
        char *strret;                                 \
        PD_system_formatString(&strret, __VA_ARGS__); \
        PD_system_logToConsole(__VA_ARGS__);          \
        PD_system_realloc(strret, 0);                 \
    }
#elif 1
#define pltf_log PD_system_logToConsole
#else
#define pltf_log(...)
#endif

extern PlaydateAPI *PD;

enum {
    PLTF_PD_BTN_DL = kButtonLeft,
    PLTF_PD_BTN_DR = kButtonRight,
    PLTF_PD_BTN_DU = kButtonUp,
    PLTF_PD_BTN_DD = kButtonDown,
    PLTF_PD_BTN_B  = kButtonB,
    PLTF_PD_BTN_A  = kButtonA
};

bool32 pltf_pd_reduce_flicker();
void   pltf_pd_update_rows(i32 from_incl, i32 to_incl);
f32    pltf_pd_crank_deg();
f32    pltf_pd_crank();
bool32 pltf_pd_crank_docked();
u32    pltf_pd_btn();

#endif