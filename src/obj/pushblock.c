// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void   pushblock_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
void   pushblock_on_update(g_s *g, obj_s *o);
void   pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
bool32 pushblock_pushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);

typedef struct {
    i32    saveID;
    v2_i32 saveID_pt;
} pushblock_s;

void pushblock_load(g_s *g, map_obj_s *mo)
{
    obj_s       *o  = obj_create(g);
    pushblock_s *pb = (pushblock_s *)o->mem;
    o->editorUID    = mo->UID;
    o->ID           = OBJID_PUSHBLOCK;

    i32    saveID   = map_obj_i32(mo, "saveID");
    v2_i16 p_save   = map_obj_pt(mo, "saveID_pt_touch");
    pb->saveID      = saveID;
    pb->saveID_pt.x = p_save.x << 4;
    pb->saveID_pt.y = p_save.y << 4;

    if (saveID_has(g, saveID)) {
        v2_i16 p = map_obj_pt(mo, "pos_if_saveID");
        o->pos.x = (i32)p.x << 4;
        o->pos.y = (i32)p.y << 4;
    } else {
        o->pos.x = mo->x;
        o->pos.y = mo->y;
    }

    i32 w                  = map_obj_i32(mo, "weight");
    o->substate            = 1 < w ? w : 1;
    o->on_update           = pushblock_on_update;
    o->on_draw             = pushblock_on_draw;
    o->on_pushpull         = pushblock_on_pushpull;
    o->on_pushpull_blocked = pushblock_pushpull_blocked;
    o->flags               = OBJ_FLAG_PUSHABLE_SOLID | OBJ_FLAG_GRAB;
    o->moverflags          = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->render_priority     = RENDER_PRIO_OWL + 1;
    o->w                   = mo->w;
    o->h                   = mo->h;
}

void pushblock_on_update(g_s *g, obj_s *o)
{
    pushblock_s *pb = (pushblock_s *)o->mem;
    rec_i32      r  = obj_aabb(o);
    if (overlap_rec_pnt(r, pb->saveID_pt)) {
        saveID_put(g, pb->saveID);
    }

    if (map_blocked(g, obj_rec_bottom(o))) {
        o->timer = 0; // reset fall delay timer
    } else {
        // not grounded -> don't push, only fall
        o->timer++; // fall delay timer
        i32 t  = o->timer - 15;
        i32 dy = 0 <= t ? min_i32(t >> 1, 8) & ~1 : 0;

        for (i32 m = abs_i32(dy); m; m--) {
            if (!map_blocked(g, obj_rec_bottom(o))) {
                obj_step_solid(g, o, 0, +1);
            }
        }
    }
}

void pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32 p = o->pos;
    p        = v2_i32_add(p, cam);
    render_tile_terrain_block(gfx_ctx_display(), p, o->w / 16, o->h / 16, TILE_TYPE_BRIGHT_STONE);
}

void pushblock_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
}

bool32 pushblock_pushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
    // only be able to be pushed if not blocked and currently grounded
    if (obj_pushpull_blocked_default(g, o, dt_x, dt_y)) return 1;

    rec_i32 rb1      = {o->pos.x, o->pos.y + o->h, o->w, 1};
    rec_i32 rb2      = {o->pos.x + dt_x, o->pos.y + o->h + dt_y, o->w, 1};
    bool32  grounded = map_blocked_excl(g, rb1, o);
    return (!grounded);
}