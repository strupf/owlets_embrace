// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define WORM_SEG_DST (14 << 8)

enum {
    WALLWORM_NORMAL,
    WALLWORM_PULLED,
};

typedef struct wallworm_hole_s wallworm_hole_s;
struct wallworm_hole_s {
    wallworm_hole_s *next;
    v2_i16           p;
};

typedef struct {
    v2_i32 p_q8;
} wallworm_seg_s;

typedef struct {
    v2_i32         dir;
    i32            n_seg;
    wallworm_seg_s segs[32];
} wallworm_s;

static_assert(sizeof(wallworm_s) <= 512, "M");

void wallworm_on_update(g_s *g, obj_s *o);
void wallworm_on_animate(g_s *g, obj_s *o);
void wallworm_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void wallworm_constrain_to(obj_s *o, wallworm_seg_s *seg);

void wallworm_load(g_s *g, map_obj_s *mo)
{
    sizeof(wallworm_s);
    obj_s      *o      = obj_create(g);
    wallworm_s *w      = (wallworm_s *)o->mem;
    o->ID              = OBJID_WALLWORM_PARENT;
    o->on_update       = wallworm_on_update;
    o->on_animate      = wallworm_on_animate;
    o->on_draw         = wallworm_on_draw;
    o->render_priority = 10;

    w->n_seg = 15;
    for (i32 n = 0; n < w->n_seg; n++) {
        wallworm_seg_s *seg = &w->segs[n];
        seg->p_q8.x         = mo->x << 8;
        seg->p_q8.y         = mo->y << 8;
    }

    i32     n_pt = 0;
    v2_i16 *pt   = (v2_i16 *)map_obj_arr(mo, "P", &n_pt);
}

void wallworm_on_update(g_s *g, obj_s *o)
{
    wallworm_s *w = (wallworm_s *)o->mem;

    switch (o->state) {
    case WALLWORM_NORMAL: {
        wallworm_seg_s *head = &w->segs[0];
        w->dir.x += rngr_i32(-32, +32);
        w->dir.y += rngr_i32(-32, +32);
        w->dir.x = 0;
        w->dir.y = 0;
#ifdef PLTF_SDL
        if (pltf_sdl_key(SDL_SCANCODE_DOWN)) {
            w->dir.y = +1;
        }
        if (pltf_sdl_key(SDL_SCANCODE_UP)) {
            w->dir.y = -1;
        }
        if (pltf_sdl_key(SDL_SCANCODE_RIGHT)) {
            w->dir.x = +1;
        }
        if (pltf_sdl_key(SDL_SCANCODE_LEFT)) {
            w->dir.x = -1;
        }
        w->dir.x *= 512;
        w->dir.y *= 512;
#endif

        head->p_q8.x += w->dir.x;
        head->p_q8.y += w->dir.y;
        wallworm_constrain_to(o, head);
        break;
    }
    }
}

void wallworm_on_animate(g_s *g, obj_s *o)
{
    wallworm_s *w = (wallworm_s *)o->mem;
}

void wallworm_draw_segs(g_s *g, obj_s *o, v2_i32 cam, i32 srcx)
{
    gfx_ctx_s   ctx = gfx_ctx_display();
    texrec_s    tr  = asset_texrec(TEXID_WALLWORM, 0, 0, 32, 32);
    wallworm_s *w   = (wallworm_s *)o->mem;
    for (i32 n = w->n_seg - 1; 0 <= n; n--) {
        v2_i32 p1 = w->segs[n].p_q8;

        v2_i32 dir = {0};
        if (0 < n) {
            v2_i32 p2  = w->segs[n - 1].p_q8;
            v2_i32 dt2 = v2_sub(p2, p1);
            dir        = v2_add(dir, dt2);
        }
        if (n < w->n_seg - 1) {
            v2_i32 p2  = w->segs[n + 1].p_q8;
            v2_i32 dt2 = v2_sub(p1, p2);
            dir        = v2_add(dir, dt2);
        }

        f32 ang   = atan2f((f32)dir.y, (f32)dir.x);
        i32 frame = ((i32)((ang * 8.f) / PI_FLOAT + .5f) + 16 + 12) % 16;
        i32 spr   = 14 - n;

        if (n == 0) {
            tr.y = 32 * frame;
            tr.x = srcx + 32 * 14;
        } else {
            tr.y = 32 * frame;
            // tr.r.x = srcx + 32 * (7 + ((gameplay_time(g) >> 2) % 3));
            tr.x = srcx + 32 * 6;
        }

        v2_i32 p = v2_add(v2_shr(p1, 8), cam);
        p.x -= 16;
        p.y -= 16;
        gfx_spr(ctx, tr, p, 0, 0);
    }
}

void wallworm_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    wallworm_draw_segs(g, o, cam, 512);
    wallworm_draw_segs(g, o, cam, 0);
}

void wallworm_constrain_segs(wallworm_seg_s *seg0, wallworm_seg_s *seg1)
{
    v2_i32 dt = v2_sub(seg1->p_q8, seg0->p_q8);
    i32    ls = v2_lensq(dt);
    if (POW2(WORM_SEG_DST) < ls) {
        dt         = v2_setlen(dt, WORM_SEG_DST);
        seg1->p_q8 = v2_add(seg0->p_q8, dt);
    }
}

void wallworm_constrain_to(obj_s *o, wallworm_seg_s *seg)
{
    wallworm_s *w = (wallworm_s *)o->mem;
    i32         i = (i32)(seg - &w->segs[0]);

    for (i32 n = i - 1; 0 <= n; n--) {
        wallworm_constrain_segs(&w->segs[n + 1], &w->segs[n]);
    }

    for (i32 n = i + 1; n < w->n_seg; n++) {
        wallworm_constrain_segs(&w->segs[n - 1], &w->segs[n]);
    }
}