// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SPRITEANIM_H
#define SPRITEANIM_H

#include "gamedef.h"

enum {
    SPRITEANIM_MODE_NONE,
    SPRITEANIM_MODE_LOOP,
    SPRITEANIM_MODE_LOOP_REV,
    SPRITEANIM_MODE_LOOP_PINGPONG,
    SPRITEANIM_MODE_ONCE,
    SPRITEANIM_MODE_ONCE_REV,
};

typedef struct {
    int x;
    int y;
    int ticks;
} spriteanimframe_s;

typedef struct {
    tex_s              tex;
    int                w;
    int                h;
    int                n_frames;
    spriteanimframe_s *frames;
} spriteanimdata_s;

typedef struct spriteanim_s spriteanim_s;
struct spriteanim_s {
    spriteanimdata_s data;
    int              mode;
    int              tick;
    int              frame;
    void            *cb_arg;
    void (*cb_finished)(spriteanim_s *a, void *cb_arg);
    void (*cb_framechanged)(spriteanim_s *a, void *cb_arg);
};

void     spriteanim_update(spriteanim_s *a);
texrec_s spriteanim_frame(spriteanim_s *a);

#endif