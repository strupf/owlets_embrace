// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define BOMB_TICKS_TO_EXPLODE 200
#define BOMB_TICKS_ANIM       8

enum {
    BOMB_ST_IDLE,
    BOMB_ST_CARRIED,
};

void bomb_on_update(g_s *g, obj_s *o);
void bomb_on_animate(g_s *g, obj_s *o);
void bomb_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void bomb_set_carried(obj_s *o);
void bomb_on_carried_removed(g_s *g, obj_s *o);

obj_s *bomb_create(g_s *g)
{
    obj_s *o              = obj_create(g);
    o->ID                 = OBJID_BOMB;
    o->on_update          = bomb_on_update;
    o->on_animate         = bomb_on_animate;
    o->on_draw            = bomb_on_draw;
    o->w                  = OWL_W;
    o->h                  = OWL_W;
    o->flags              = OBJ_FLAG_ACTOR;
    o->on_carried_removed = bomb_on_carried_removed;
    o->moverflags =
        OBJ_MOVER_TERRAIN_COLLISIONS |
        OBJ_MOVER_ONE_WAY_PLAT;
    o->render_priority = RENDER_PRIO_OWL + 1;
    o->subtimer        = 0xFFFF; // be sure to trigger the first "bop" of the bomb animation as soon as possible
    return o;
}

void bomb_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    default: break;
    case BOMB_ST_IDLE: {
        if (o->bumpflags & OBJ_BUMP_X) {
        }
        if (o->bumpflags & OBJ_BUMP_Y_POS) {
            obj_vy_q8_mul(o, -Q_8(0.7));
        }
        if (o->bumpflags & OBJ_BUMP_Y_NEG) {
            o->v_q12.y = 0;
        }
        if (obj_grounded(g, o)) {
            obj_vx_q8_mul(o, +Q_8(0.7));
        }
        o->v_q12.y += Q_VOBJ(0.3);
        o->bumpflags = 0;

        obj_move_by_v_q12(g, o);
        break;
    }
    }

    o->timer++;
    o->subtimer++;

    i32 t = o->timer - BOMB_TICKS_TO_EXPLODE / 2;
    if (0 <= t) {
        if (o->animation) {
            o->animation++;
            if (BOMB_TICKS_ANIM <= o->animation) {
                o->animation = 0;
            }
        }

        i32 t2 = BOMB_TICKS_TO_EXPLODE - BOMB_TICKS_TO_EXPLODE / 2;
        if (ease_out_quad(25, 8, t, t2) <= o->subtimer) {
            o->subtimer  = 0;
            o->animation = 1;
        }
    }

    if (BOMB_TICKS_TO_EXPLODE <= o->timer) {
        v2_i32 p = obj_pos_center(o);
        animobj_create(g, p, ANIMOBJ_EXPLOSION_4);
        obj_delete(g, o);
    }
}

void bomb_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);

    i32 fy = 2;
    i32 fx = 7;
    if (o->animation) {
        fx += (2 < o->animation && o->animation < (BOMB_TICKS_ANIM - 2)) ? 3 : 1;
        fx += p.x & 1;
        fy += p.y & 1;
    }

    if (o->state == BOMB_ST_CARRIED) {
        if (g->cam.cowl.do_align_x)
            p.x &= ~1;
        if (g->cam.cowl.do_align_y) {
            p.y &= ~1;
        }
    }

    p.x += (o->w - 32) / 2;
    p.y += (o->h - 32) / 2;

    texrec_s tr = asset_texrec(TEXID_BOMBPLANT, fx * 32, fy * 32, 32, 32);
    gfx_spr(ctx, tr, p, 0, 0);
}

void bomb_on_animate(g_s *g, obj_s *o)
{
#if 0
    if (o->state == BOMB_ST_CARRIED) {
        obj_s *o_owl = owl_if_present_and_alive(g);
        if (o_owl) {
            o->pos.x = (o_owl->pos.x + o_owl->w / 2 - o->w / 2) + o_owl->facing * 6;
            o->pos.y = (o_owl->pos.y + 4);
            bomb_set_carried(o);
        } else {
            bomb_on_carried_removed(g, o);
        }
    }
#endif
}

void bomb_set_carried(obj_s *o)
{
    o->state   = BOMB_ST_CARRIED;
    o->v_q12.x = 0;
    o->v_q12.y = 0;
    o->flags &= ~OBJ_FLAG_ACTOR;
    o->moverflags = 0;
}

void bomb_on_carried_removed(g_s *g, obj_s *o)
{
    o->state = BOMB_ST_IDLE;
    o->flags |= OBJ_FLAG_ACTOR;
    o->moverflags |= OBJ_MOVER_TERRAIN_COLLISIONS |
                     OBJ_MOVER_ONE_WAY_PLAT;
}