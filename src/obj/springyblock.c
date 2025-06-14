// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 moved;
} springyblock_s;

void springyblock_on_update(g_s *g, obj_s *o);
void springyblock_on_hook(g_s *g, obj_s *o, bool32 hooked);
void springyblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void springyblock_load(g_s *g, map_obj_s *mo)
{
    obj_s          *o = obj_create(g);
    springyblock_s *s = (springyblock_s *)o->mem;
    o->ID             = OBJID_SPRINYBLOCK;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->w              = mo->w;
    o->h              = mo->h;
    o->on_update      = springyblock_on_update;
    o->on_hook        = springyblock_on_hook;
    o->on_draw        = springyblock_on_draw;
    o->flags          = OBJ_FLAG_SOLID | OBJ_FLAG_HOOKABLE;
    o->ropeobj.m_q8   = 256;
}

void springyblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);
    p.x &= ~1;
    p.y &= ~1;
    render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4, TILE_TYPE_BRIGHT_STONE);
}

void springyblock_on_update(g_s *g, obj_s *o)
{
    springyblock_s *s = (springyblock_s *)o->mem;

    grapplinghook_s *gh = &g->ghook;
    rec_i32          rh = {gh->p.x, gh->p.y, 1, 1};

    i32 force = 0;

    if (gh->state == GRAPPLINGHOOK_HOOKED_SOLID && o->substate) {
        force = -grapplinghook_f_at_obj_proj(&g->ghook, o, (v2_i32){1, 0});
    }

#define SPRINGYBLOCK_ACC_Y 10

    // o->v_q8.y -= SPRINGYBLOCK_ACC_Y * s->moved;
    obj_vx_q8_mul(o, Q_VOBJ(0.97));
    o->v_q12.x += force >> 2;
    o->v_q12.x = clamp_sym_i32(o->v_q12.x, Q_VOBJ(4.0));

    o->subpos_q12.x += o->v_q12.x;
    i32 tm = o->subpos_q12.x >> 8;
    o->subpos_q12.x &= 255;

    tm = clamp_i32(s->moved + tm, 0, 256) - s->moved;

    s->moved += tm;
    obj_move(g, o, tm, 0);

    if (o->v_q12.x < 0 && s->moved == 0) {
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    }
    if (o->v_q12.x > 0 && s->moved == 256) {
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    }

    o->ropeobj.v_q8.x = o->v_q12.x;
    // o->ropeobj.a_q8.x = s->moved ? -SPRINGYBLOCK_ACC_Y * s->moved : 0;
}

void springyblock_on_hook(g_s *g, obj_s *o, bool32 hooked)
{
    o->substate = hooked;
}