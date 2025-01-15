// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_PD_H
#define PLTF_PD_H

#undef TARGET_EXTENSION
#define TARGET_EXTENSION 1

#include "PD/pd_api.h"
#include "pltf_types.h"

extern PlaydateAPI *PD;
extern void (*PD_system_logToConsole)(const char *fmt, ...);
extern void *(*PD_system_realloc)(void *ptr, size_t size);
#if PLTF_ENABLE_LOG
#define pltf_log PD_system_logToConsole
#else
#define pltf_log(...)
#endif

#define pltf_mem_alloc(S) PD_system_realloc(0, S)
#define pltf_mem_free(P)  PD_system_realloc(P, 0)

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
void  *pltf_pd_menu_add_opt(const char *title, const char **opt, i32 n_opt,
                            void (*func)(void *ctx, i32 opt), void *ctx);
void  *pltf_pd_menu_add(const char *title,
                        void (*func)(void *ctx, i32 opt), void *ctx);
i32    pltf_pd_menu_opt_val(void *itemp);
void   pltf_pd_menu_opt_val_set(void *itemp, i32 v);
void   pltf_pd_menu_rem(void *itemp);

#endif