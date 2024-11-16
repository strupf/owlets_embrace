// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

typedef struct {
    u32 hash;
} stompable_block_s;

void stompable_block_load(g_s *g, map_obj_s *mo)
{
    char saveID[64] = {0};
    map_obj_strs(mo, "SaveID", saveID);
    u32 hash = hash_str(saveID);
    if (saveID_has(g, hash)) return;

    obj_s             *o = obj_create(g);
    stompable_block_s *b = (stompable_block_s *)o->mem;
    b->hash              = hash;
    o->w                 = mo->w;
    o->h                 = 16;
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
    o->ID                = OBJ_ID_STOMPABLE_BLOCK;
    o->mass              = 1;
    o->flags             = OBJ_FLAG_RENDER_AABB;
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
    saveID_put(g, b->hash);
}