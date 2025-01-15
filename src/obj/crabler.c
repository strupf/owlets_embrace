// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 p;
} crabler_leg_s;

typedef struct {
    v2_i32 p;
} crabler_arm_s;

typedef struct {
    i32           orientation;
    crabler_leg_s legs[6];
    crabler_arm_s arms[2];

} crabler_s;

enum {
    CRABLER_ST_IDLE,
};

void crabler_on_update(g_s *g, obj_s *o);
void crabler_on_animate(g_s *g, obj_s *o);
void crabler_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void crabler_load(g_s *g, map_obj_s *mo)
{
    obj_s     *o  = obj_create(g);
    crabler_s *c  = (crabler_s *)o->mem;
    o->ID         = OBJID_CRABLER;
    o->on_update  = crabler_on_update;
    o->on_draw    = crabler_on_draw;
    o->on_animate = crabler_on_animate;
}

void crabler_on_update(g_s *g, obj_s *o)
{
    crabler_s *c = (crabler_s *)o->mem;

    switch (o->state) {
    case CRABLER_ST_IDLE: {
        break;
    }
    }
}

void crabler_on_animate(g_s *g, obj_s *o)
{
    crabler_s *c = (crabler_s *)o->mem;

    switch (o->state) {
    case CRABLER_ST_IDLE: {
        break;
    }
    }
}

void crabler_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{

    gfx_ctx_s  ctx    = gfx_ctx_display();
    crabler_s *c      = (crabler_s *)o->mem;
    v2_i32     anchor = v2_add(obj_pos_center(o), cam);

    tex_s    tex       = asset_tex(TEXID_CRABLER);
    texrec_s tr_leg    = {tex, 0, 0, 0, 0};
    texrec_s tr_body   = {tex, 0, 0, 0, 0};
    texrec_s tr_hammer = {tex, 0, 0, 0, 0};

    i32 flip = o->facing == 1 ? 0 : SPR_FLIP_X;

    // rear legs
    for (i32 n = 0; n < 3; n++) {
    }

    // rear arm

    v2_i32 posarm1 = anchor;
    posarm1.x -= tr_hammer.w;
    gfx_spr(ctx, tr_hammer, posarm1, flip, 0);

    // body
    v2_i32 posbody = anchor;
    posbody.x -= tr_body.w;
    gfx_spr(ctx, tr_body, posbody, flip, 0);

    // front legs
    for (i32 n = 0; n < 3; n++) {
        texrec_s tr_leg_ = tr_leg;
        if (n == 2) {
            tr_leg_.x += tr_leg_.w;
        }
        v2_i32 posleg = anchor;
        posleg.x -= tr_leg.w;
        gfx_spr(ctx, tr_leg_, posleg, SPR_FLIP_X - flip, 0);
    }

    // front arm

    v2_i32 posarm2 = anchor;
    posarm2.x -= tr_hammer.w;
    gfx_spr(ctx, tr_hammer, posarm2, flip, 0);
    switch (o->state) {
    case CRABLER_ST_IDLE: {
        break;
    }
    }
}