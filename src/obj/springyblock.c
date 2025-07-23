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
    // o->ropeobj.m_q8   = 256;
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
        force = -grapplinghook_f_at_obj_proj(&g->ghook, o, (v2_i32){0, 1});
    }

#define SPRINGYBLOCK_ACC_Y 10

    o->v_q12.y -= 10 * s->moved;
    obj_vy_q8_mul(o, Q_8(0.97));
    o->v_q12.y += force >> 1;
    o->v_q12.y = clamp_sym_i32(o->v_q12.y, Q_VOBJ(4.0));

    o->subpos_q12.y += o->v_q12.y;
    i32 tm = o->subpos_q12.y >> 12;
    o->subpos_q12.y &= 0xFFF;

    tm = clamp_i32(s->moved + tm, 0, 64) - s->moved;

    s->moved += tm;
    obj_move(g, o, 0, tm);

    if (o->v_q12.y < 0 && s->moved == 0) {
        o->v_q12.y      = 0;
        o->subpos_q12.y = 0;
    }
    if (o->v_q12.y > 0 && s->moved == 64) {
        o->v_q12.y      = 0;
        o->subpos_q12.y = 0;
    }

    // o->ropeobj.v_q8.y = o->v_q12.y >> 4;
    //  o->ropeobj.a_q8.x = s->moved ? -SPRINGYBLOCK_ACC_Y * s->moved : 0;
}

void springyblock_on_hook(g_s *g, obj_s *o, bool32 hooked)
{
    o->substate = hooked;
}