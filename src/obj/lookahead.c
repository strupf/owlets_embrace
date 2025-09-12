// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    cs_camera_pan_config_s pan_config;
} lookahead_s;

static_assert(sizeof(lookahead_s) <= OBJ_MEM_BYTES, "size");

void lookahead_on_update(g_s *g, obj_s *o);
void lookahead_on_animate(g_s *g, obj_s *o);
void lookahead_on_interact(g_s *g, obj_s *o);

void lookahead_load(g_s *g, map_obj_s *mo)
{
    i32     n_pt = 0;
    v2_i16 *pt   = map_obj_arr(mo, "PT", &n_pt);
    if (n_pt == 0) return;

    obj_s                  *o  = obj_create(g);
    lookahead_s            *la = (lookahead_s *)o->mem;
    cs_camera_pan_config_s *pc = &la->pan_config;
    o->ID                      = OBJID_LOOKAHEAD;
    o->pos.x                   = mo->x;
    o->pos.y                   = mo->y;
    o->w                       = mo->w;
    o->h                       = mo->h;
    o->flags                   = OBJ_FLAG_INTERACTABLE;
    o->n_sprites               = 1;
    o->on_animate              = lookahead_on_animate;
    o->on_interact             = lookahead_on_interact;
    o->facing                  = map_obj_bool(mo, "flip_sprite") ? -1 : +1;
    pc->circ                   = map_obj_bool(mo, "circ");
    pc->pt[pc->n_pt++]         = obj_pos_center(o);
    pc->controllable           = 1;

    for (i32 n = 0; n < n_pt; n++) {
        pc->pt[pc->n_pt++] = v2_i32_shl(v2_i32_from_i16(pt[n]), 4);
    }
    if (!pc->circ) {
        for (i32 n = n_pt - 2; 0 <= n; n--) {
            pc->pt[pc->n_pt++] = v2_i32_shl(v2_i32_from_i16(pt[n]), 4);
        }
    }

    pc->pt[pc->n_pt++] = obj_pos_center(o);
}

void lookahead_on_update(g_s *g, obj_s *o)
{
}

void lookahead_on_animate(g_s *g, obj_s *o)
{
    lookahead_s  *la  = (lookahead_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];
    i32           f   = 0;

    o->animation++;

    spr->offs.x = (o->w - 64) / 2;
    spr->offs.y = (o->h - 48);
    if (o->facing == -1) {
        spr->flip = SPR_FLIP_X;
        spr->offs.x++;
    }

    f         = ani_frame_loop(ANIID_LOOKAHEAD, o->animation);
    spr->trec = asset_texrec(TEXID_LOOKAHEAD, 0, f * 48, 64, 48);
}

void lookahead_on_interact(g_s *g, obj_s *o)
{
    lookahead_s *la = (lookahead_s *)o->mem;
    if (!o->state) {
        o->on_update = lookahead_on_update;
        o->timer     = 0;
        cs_camera_pan_enter(g, &la->pan_config);
    }
}