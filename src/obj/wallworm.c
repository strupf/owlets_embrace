// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 tick;
    u8  from;
    u8  to;
} wallworm_seg_s;

typedef struct wallworm_hole_s wallworm_hole_s;
struct wallworm_hole_s {
    wallworm_hole_s *next;
    v2_i16           p;
};

typedef struct {
    u8               n_pt;
    wallworm_hole_s *hole_head;
    wallworm_hole_s  holes[16];
} wallworm_s;

static_assert(sizeof(wallworm_s) <= 512, "M");

void wallworm_on_update(game_s *g, obj_s *o);
void wallworm_on_animate(game_s *g, obj_s *o);
void wallworm_on_draw(game_s *g, obj_s *o, v2_i32 cam);

void wallworm_load(game_s *g, map_obj_s *mo)
{
    obj_s      *o      = obj_create(g);
    wallworm_s *w      = (wallworm_s *)o->mem;
    o->ID              = OBJ_ID_WALLWORM_PARENT;
    o->on_update       = wallworm_on_update;
    o->on_animate      = wallworm_on_animate;
    o->on_draw         = wallworm_on_draw;
    o->render_priority = 10;

    i32     n_pt = 0;
    v2_i16 *pt   = (v2_i16 *)map_obj_arr(mo, "P", &n_pt);

    w->n_pt          = n_pt + 1;
    w->hole_head     = NULL;
    w->holes[0].p.x  = mo->x >> 4;
    w->holes[0].p.y  = mo->y >> 4;
    w->holes[0].next = NULL;
    for (i32 n = 0; n < n_pt; n++) {
        wallworm_hole_s *hole = &w->holes[n + 1];
        hole->p               = pt[n];
        hole->next            = NULL;
    }

    if ((w->n_pt & 1) != 0) {
        pltf_log("wallworm hole number uneven!\n");
    }
    for (i32 n = 1; n < w->n_pt; n += 2) {
        i32 dx = (i32)w->holes[n].p.x - (i32)w->holes[n - 1].p.x;
        i32 dy = (i32)w->holes[n].p.y - (i32)w->holes[n - 1].p.y;
        if (!(dx == 0 || dy == 0)) pltf_log("wallworm holes not aligned!\n");
        if (!(abs_i32(dx) == 2 || abs_i32(dy) == 2)) pltf_log("wallworm holes not aligned!\n");
    }
}

void wallworm_on_update(game_s *g, obj_s *o)
{
    wallworm_s *w = (wallworm_s *)o->mem;
}

void wallworm_on_animate(game_s *g, obj_s *o)
{
    wallworm_s *w = (wallworm_s *)o->mem;
}

void wallworm_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s   ctx = gfx_ctx_display();
    wallworm_s *w   = (wallworm_s *)o->mem;
    for (i32 n = 0; n < w->n_pt; n++) {
        wallworm_hole_s hole  = w->holes[n];
        v2_i32          phole = {hole.p.x, hole.p.y};
        v2_i32          p     = v2_add(v2_shl(phole, 4), cam);
        gfx_rec_fill(ctx, (rec_i32){p.x, p.y, 16, 16}, GFX_COL_BLACK);
    }
}