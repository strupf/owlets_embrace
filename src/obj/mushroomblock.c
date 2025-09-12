// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define MUSHROOMBLOCK_TR_ENABLE    0
#define MUSHROOMBLOCK_TR_DISABLE   1
#define MUSHROOMBLOCK_TICKS_APPEAR 3

typedef struct {
    i32 trigger[2];
} mushroomblock_s;

void        mushroomblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void        mushroomblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void        mushroomblock_on_animate(g_s *g, obj_s *o);
static void mushroomblock_set_state(g_s *g, obj_s *o, i32 state);

void mushroomblock_load(g_s *g, map_obj_s *mo)
{
    obj_s           *o  = obj_create(g);
    mushroomblock_s *ot = (mushroomblock_s *)o->mem;
    o->UUID             = mo->UUID;
    o->ID               = OBJID_MUSHROOMBLOCK;
    o->render_priority  = RENDER_PRIO_OWL + 1;
    o->state            = 0;
    o->pos.x            = mo->x;
    o->pos.y            = mo->y;
    o->w                = mo->w;
    o->h                = mo->h;
    o->on_draw          = mushroomblock_on_draw;
    o->on_trigger       = mushroomblock_on_trigger;
    o->on_animate       = mushroomblock_on_animate;

    ot->trigger[MUSHROOMBLOCK_TR_ENABLE]  = map_obj_i32(mo, "Trigger_enable");
    ot->trigger[MUSHROOMBLOCK_TR_DISABLE] = map_obj_i32(mo, "Trigger_disable");
    mushroomblock_set_state(g, o, map_obj_bool(mo, "Enabled"));
}

void mushroomblock_on_animate(g_s *g, obj_s *o)
{
    o->animation++;
}

void mushroomblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr  = asset_texrec(TEXID_TOGGLE, 0, 0, 16, 16);
    v2_i32    pos = v2_i32_add(o->pos, cam);
    i32       nx  = o->w >> 4;
    i32       ny  = o->h >> 4;

    i32 tiletype = TILE_TYPE_MUSHROOMS;

    if (o->animation < MUSHROOMBLOCK_TICKS_APPEAR) {
        tiletype = TILE_TYPE_MUSHROOMS_HALF;
    } else if (o->state == 0) {
        tiletype = TILE_TYPE_MUSHROOMS_GONE;
    }
    render_tile_terrain_block(ctx, pos, nx, ny, tiletype);
}

static void mushroomblock_set_state(g_s *g, obj_s *o, i32 state)
{
    if (o->state == state) return;

    o->state     = state;
    o->animation = 0;

    switch (state) {
    case 0:
        o->flags &= ~OBJ_FLAG_SOLID;
        o->flags &= ~OBJ_FLAG_CLIMBABLE;
        o->flags &= ~OBJ_FLAG_HOOKABLE;
        o->render_priority = RENDER_PRIO_BEHIND_TERRAIN_LAYER - 1;
        break;
    default:
        o->flags |= OBJ_FLAG_SOLID;
        o->flags |= OBJ_FLAG_CLIMBABLE;
        o->flags |= OBJ_FLAG_HOOKABLE;
        game_on_solid_appear_ext(g, o);
        o->render_priority = RENDER_PRIO_OWL + 1;
        break;
    }
    g->objrender_dirty = 1;
}

void mushroomblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    mushroomblock_s *ot = (mushroomblock_s *)o->mem;

    if (trigger == ot->trigger[o->state]) {
        mushroomblock_set_state(g, o, 1 - o->state);
    }
}
