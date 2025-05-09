// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void puppet_on_animate(g_s *g, obj_s *o);
void puppet_companion_on_animate(obj_s *o, i32 animID, i32 anim_t);
void puppet_hero_on_animate(obj_s *o, i32 animID, i32 anim_t);

obj_s *puppet_create(g_s *g, i32 objID_puppet)
{
    obj_s    *o   = obj_create(g);
    puppet_s *c   = (puppet_s *)o->mem;
    o->ID         = objID_puppet;
    o->on_animate = puppet_on_animate;
    o->facing     = 1;

    if (0) {
        o->flags = OBJ_FLAG_ACTOR;
    }
    return o;
}

void puppet_on_animate(g_s *g, obj_s *o)
{
    puppet_s *c = (puppet_s *)o->mem;
    if (o->subtimer) {
        o->timer++;
        o->pos.x = c->movefunc(c->p_src.x, c->p_dst.x, o->timer, o->subtimer);
        o->pos.y = c->movefunc(c->p_src.y, c->p_dst.y, o->timer, o->subtimer);

        if (o->subtimer <= o->timer) {
            o->subtimer = 0;
            if (c->arrived_cb) {
                c->arrived_cb(g, o, c->arrived_ctx);
            }
        }
    }

    c->anim_t++;

    switch (o->ID) {
    case OBJID_PUPPET_COMPANION:
        puppet_companion_on_animate(o, c->animID, c->anim_t);
        break;
    case OBJID_PUPPET_HERO:
        puppet_hero_on_animate(o, c->animID, c->anim_t);
        break;
    }
}

// if animID not 0: change animation
// if facing not 0: change facing
void puppet_set_anim(obj_s *o, i32 animID, i32 facing)
{
    if (!o) return;

    puppet_s *c = (puppet_s *)o->mem;
    if (animID && animID != c->animID) {
        c->animID = animID;
        c->anim_t = 0;
    }
    if (facing) {
        o->facing = facing;
    }
}

// move companion to position over t ticks
// call function on arrival
void puppet_move(obj_s *o, v2_i32 p, i32 t)
{
    puppet_move_ext(o, p, t, 0, 0, 0, 0);
}

void puppet_move_ext(obj_s *o, v2_i32 p, i32 t, ease_i32 movefunc,
                     bool32 relative, void (*arrived_cb)(g_s *g, obj_s *o, void *ctx),
                     void  *ctx)
{
    if (!o) return;

    puppet_s *c    = (puppet_s *)o->mem;
    o->timer       = 0;
    o->subtimer    = t;
    c->p_src       = o->pos;
    c->p_dst       = relative ? v2_i32_add(p, o->pos) : p;
    c->arrived_cb  = arrived_cb;
    c->arrived_ctx = ctx;
    c->movefunc    = movefunc ? movefunc : ease_lin;
}
