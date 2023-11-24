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

typedef struct {
    struct {
        u16 x, y, w, h;
    } frame;
    struct {
        u16 x, y, w, h;
    } spritesourcesize;
    struct {
        u16 w, h;
    } sourcesize;
    u16 duration;
    u8  trimmed;
    u8  rotated;
} ase_frame_s;

typedef struct {
    char         tag[16];
    ase_frame_s *frames;
    int          n_frames;
} ase_tagged_anim_s;

typedef struct {
    ase_frame_s       *frames;
    ase_tagged_anim_s *tagged_anim;
    int                n_tagged_anim;
    int                n_frames;
} ase_anim_s;

enum {
    ASE_SUCCESS,
    ASE_ERR_FILE,
    ASE_ERR_JSON,
    ASE_ERR_ALLOC,
};

int      ase_anim_parse(ase_anim_s *a, const char *file, void *(*allocf)(usize s));
bool32   ase_anim_get_tag(ase_anim_s a, const char *tag, ase_tagged_anim_s *t);
void     spriteanim_update(spriteanim_s *a);
texrec_s spriteanim_frame(spriteanim_s *a);

#endif