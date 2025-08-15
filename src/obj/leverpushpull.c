// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    LEVER_PUSHPULL_X,
    LEVER_PUSHPULL_Y
};

typedef struct {
    i32 saveID;
    b32 was_moved;
    i32 dt_x_min;
    i32 dt_x_max;
    i32 dt_x_so_far;
    i32 dt_y_min;
    i32 dt_y_max;
    i32 dt_y_so_far;
    i32 move_back_ticks;
    i32 trigger_on_end;
    i32 trigger_on_start;
} leverpushpull_s;

void   leverpushpull_on_update(g_s *g, obj_s *o);
void   leverpushpull_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
void   leverpushpull_on_draw(g_s *g, obj_s *o, v2_i32 cam);
bool32 leverpushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
void   leverpushpull_on_hook(g_s *g, obj_s *o, i32 hooked);

void leverpushpull_load(g_s *g, map_obj_s *mo)
{
    obj_s           *o = obj_create(g);
    leverpushpull_s *l = (leverpushpull_s *)o->mem;
    o->ID              = OBJID_LEVERPUSHPULL;
    o->flags           = OBJ_FLAG_SOLID;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->on_draw         = leverpushpull_on_draw;
    o->on_update       = leverpushpull_on_update;
    o->on_hook         = leverpushpull_on_hook;
    o->pushable_weight = 2;
    o->render_priority = RENDER_PRIO_OWL + 1;

    l->move_back_ticks  = map_obj_i32(mo, "move_back_speed");
    l->trigger_on_start = map_obj_i32(mo, "trigger_on_start");
    l->trigger_on_end   = map_obj_i32(mo, "trigger_on_end");
    i32 dt_x            = map_obj_i32(mo, "dt_x");
    i32 dt_y            = map_obj_i32(mo, "dt_y");
    if (dt_x) {
        o->flags |= OBJ_FLAG_PUSHABLE_SOLID;
        o->substate            = LEVER_PUSHPULL_X;
        o->on_pushpull_blocked = leverpushpull_blocked;
        o->on_pushpull         = leverpushpull_on_pushpull;
        if (dt_x < 0) {
            l->dt_x_min = dt_x;
        }
        if (dt_x > 0) {
            l->dt_x_max = dt_x;
        }
    }
    if (dt_y) {
        o->substate = LEVER_PUSHPULL_Y;
        if (dt_y < 0) {
            l->dt_y_min = dt_y;
        }
        if (dt_y > 0) {
            l->dt_y_max = dt_y;
        }
    }
}

void leverpushpull_on_update(g_s *g, obj_s *o)
{
    leverpushpull_s *l = (leverpushpull_s *)o->mem;

    if (l->was_moved) {
        l->was_moved = 0;
        o->timer     = 0;
    } else if (l->dt_x_so_far || l->dt_y_so_far) {
        o->timer++;

        if (l->move_back_ticks <= o->timer) {
            o->timer = 0;
            i32 dx   = -2 * sgn_i32(l->dt_x_so_far);
            i32 dy   = -2 * sgn_i32(l->dt_y_so_far);
            obj_move(g, o, dx, dy);
            l->dt_x_so_far += dx;
            l->dt_y_so_far += dy;
            if ((dx && l->dt_x_so_far == 0) || (dy && l->dt_y_so_far == 0)) {
                game_on_trigger(g, l->trigger_on_start);
            }
        }
    }
}

void leverpushpull_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);

    i32 nx = o->w >> 4;
    i32 ny = o->h >> 4;

    render_tile_terrain_block(ctx, p, nx, ny, TILE_TYPE_BRIGHT_STONE);
}

void leverpushpull_on_pushpull(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
    leverpushpull_s *l = (leverpushpull_s *)o->mem;
    l->dt_x_so_far += dt_x;
    l->was_moved = 1;

    bool32 at_limit = l->dt_x_so_far &&
                      (l->dt_x_so_far == l->dt_x_max || l->dt_x_so_far == l->dt_x_min);
    if (dt_x && at_limit) { // just moved and now at limit
        game_on_trigger(g, l->trigger_on_end);
    }
}

bool32 leverpushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
    leverpushpull_s *l = (leverpushpull_s *)o->mem;
    i32              k = l->dt_x_so_far + dt_x;

    if (l->dt_x_min <= k && k <= l->dt_x_max) {
        return 0;
    }
    return 1;
}

void leverpushpull_on_hook(g_s *g, obj_s *o, i32 hooked)
{
}