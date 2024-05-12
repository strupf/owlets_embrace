// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// This is supposed to be a basic 3D renderer at some point

#ifndef GFX_3D_H
#define GFX_3D_H

#include "gfx.h"

typedef struct {
    i32 x, y, z;
} g3d_v3_s;

typedef struct {
    i32 x, y, z, w;
} g3d_v4_s;

typedef struct {
    i32 m[9];
} g3d_m3_s;

typedef struct {
    i32 m[16];
} g3d_m4_s;

typedef struct {
    g3d_v3_s p[3];
} g3d_tri_s;

typedef struct {
    g3d_v3_s pos;
    g3d_v3_s up;
    g3d_v3_s look_at;
} g3d_cam_s;

typedef struct {
    g3d_v3_s p;
    i32      col;
} g3d_vtx_s;

typedef struct {
    i32        n;
    i32        c; // cap
    g3d_vtx_s *v;
} g3d_vtx_buf_s;

typedef struct {
    i32  n;
    i32  c; // cap
    u16 *v;
} g3d_idx_buf_s;

typedef struct {
    tex_s         dst;
    g3d_cam_s     cam;
    g3d_vtx_buf_s vbuf;
    g3d_idx_buf_s ibuf;
} g3d_s;

g3d_m4_s g3d_proj();
g3d_m4_s g3d_orth();
g3d_m3_s g3d_mul_m3(g3d_m3_s a, g3d_m3_s b);
g3d_v3_s g3d_mul_m3_v3(g3d_m3_s m, g3d_v3_s v);
g3d_v4_s g3d_mul_m4_v3(g3d_m4_s m, g3d_v3_s v);
g3d_v4_s g3d_mul_m4_v4(g3d_m4_s m, g3d_v4_s v);
void     g3d_render(g3d_s g);

#endif