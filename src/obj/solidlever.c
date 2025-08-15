// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    SOLIDLEVER_ST_IDLE,
    SOLIDLEVER_ST_GRABBED,
    SOLIDLEVER_ST_JUST_UNGRABBED,
};

typedef struct {
    v2_i32 pos_og;
    i32    dir;
    i32    maxmove;
    i32    moved;
} solidlever_s;

bool32 solidlever_on_move(g_s *g, obj_s *o, i32 dx, i32 dy);
void   solidlever_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
void   solidlever_on_grab(g_s *g, obj_s *o);
void   solidlever_on_ungrab(g_s *g, obj_s *o);
void   solidlever_on_update(g_s *g, obj_s *o);
void   solidlever_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void solidlever_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o = obj_create(g);
    solidlever_s *s = (solidlever_s *)o->mem;
    o->ID           = OBJID_SOLIDLEVER;
    o->on_update    = solidlever_on_update;
    // o->on_grab      = solidlever_on_grab;
    o->on_pushpull  = solidlever_on_pushpull;
    o->on_draw      = solidlever_on_draw;
    o->w            = 32;
    o->h            = 32;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->flags =
        OBJ_FLAG_SOLID |
        OBJ_FLAG_GRAB;
    s->pos_og.x = mo->x;
    s->pos_og.y = mo->y;
    s->dir      = DIR_X_NEG;
    s->maxmove  = 32;
    s->moved    = 0;
}

void solidlever_on_update(g_s *g, obj_s *o)
{
    solidlever_s *s = (solidlever_s *)o->mem;
    o->timer++;

    switch (o->state) {
    case SOLIDLEVER_ST_IDLE: {
        if (v2_i32_eq(s->pos_og, o->pos)) break;

        v2_i32 dt = v2_i32_sub(s->pos_og, o->pos);
        if ((o->timer & 1) == 0) {
            obj_move(g, o, sgn_i32(dt.x), sgn_i32(dt.y));
            s->moved--;
        }
        break;
    }
    case SOLIDLEVER_ST_GRABBED: {

        break;
    }
    case SOLIDLEVER_ST_JUST_UNGRABBED: {
        if (50 <= o->timer) {
            o->timer = 0;
            o->state = SOLIDLEVER_ST_IDLE;
        }
        break;
    }
    }
}

bool32 solidlever_on_move(g_s *g, obj_s *o, i32 dx, i32 dy)
{
    solidlever_s *s = (solidlever_s *)o->mem;

    v2_i32 movedir = dir_v2(s->dir);
    if (sgn_i32(movedir.x) != sgn_i32(dx) ||
        sgn_i32(movedir.y) != sgn_i32(dy)) return 0;

    i32 dtpullmax = max_i32(0, s->maxmove - s->moved);
    i32 xa        = abs_i32(dx);
    i32 ya        = abs_i32(dy);
    i32 xm        = min_i32(dtpullmax, xa);
    i32 ym        = min_i32(dtpullmax, ya);
    obj_move(g, o, movedir.x * xm, movedir.y * ym);
    i32 tomove = max_i32(xm, ym);
    s->moved += tomove;
    return tomove;
}

void solidlever_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
}

void solidlever_on_grab(g_s *g, obj_s *o)
{
    solidlever_s *s = (solidlever_s *)o->mem;
    o->state        = SOLIDLEVER_ST_GRABBED;
}

void solidlever_on_ungrab(g_s *g, obj_s *o)
{
    solidlever_s *s = (solidlever_s *)o->mem;
    o->state        = SOLIDLEVER_ST_JUST_UNGRABBED;
    o->timer        = 0;
}

void solidlever_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx       = gfx_ctx_display();
    texrec_s  tr_handle = asset_texrec(TEXID_SOLIDLEVER, 0, 0, 16, 32);

    v2_i32 p = v2_i32_add(o->pos, cam);
    p.y -= 0;
    p.x -= 16;
    gfx_spr(ctx, tr_handle, p, 0, 0);
    rec_i32 aabb = translate_rec(obj_aabb(o), cam.x, cam.y);

    ctx.pat = gfx_pattern_50();
    gfx_rec_fill(ctx, aabb, PRIM_MODE_BLACK_WHITE);
}