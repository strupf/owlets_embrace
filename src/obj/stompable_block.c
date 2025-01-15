// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

typedef struct {
    i32 saveID;
} stompable_block_s;

void stompable_block_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (saveID_has(g, saveID)) return;

    obj_s             *o = obj_create(g);
    stompable_block_s *b = (stompable_block_s *)o->mem;
    b->saveID            = saveID;
    o->w                 = mo->w;
    o->h                 = 16;
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
    o->ID                = OBJID_STOMPABLE_BLOCK;
    o->flags             = OBJ_FLAG_SOLID | OBJ_FLAG_RENDER_AABB;
}

void stompable_block_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
}

void stompable_block_on_update(g_s *g, obj_s *o)
{
}

void stompable_block_on_animate(g_s *g, obj_s *o)
{
}

void stompable_block_on_destroy(g_s *g, obj_s *o)
{
    stompable_block_s *b = (stompable_block_s *)o->mem;
    saveID_put(g, b->saveID);
}