// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HOOKLEVER_SWITCH,
    HOOKLEVER_SPRING,
};

typedef struct {
    i32 saveID;
    i32 ticks_reset;
    i32 moved;
    i32 moved_max;
    i32 dir_x;
    i32 dir_y;
    b32 reset_blocked;
} hooklever_s;

#define HOOKLEVER_TICKS 15

i32 hooklever_pull_dir(obj_s *o)
{
    hooklever_s *hl = (hooklever_s *)o->mem;
    if (hl->dir_x == -1) return DIR_X_NEG;
    if (hl->dir_x == +1) return DIR_X_POS;
    if (hl->dir_y == -1) return DIR_Y_NEG;
    if (hl->dir_y == +1) return DIR_Y_POS;
    return 0;
}

void hooklever_pull(g_s *g, obj_s *o)
{
    hooklever_s *hl = (hooklever_s *)o->mem;
    if (hl->moved < hl->moved_max) {
        hl->moved++;
        obj_move(g, o, hl->dir_x, hl->dir_y);
    }

    o->subtimer = 0;
    switch (o->subID) {
    case HOOKLEVER_SWITCH: {
        if (hl->moved == hl->moved_max) {
            o->state = 1;
            game_on_trigger(g, o->trigger);
            saveID_put(g, hl->saveID);
        }
        break;
    }
    case HOOKLEVER_SPRING: {
        o->timer = (hl->moved * hl->ticks_reset) / hl->moved_max;
        break;
    }
    }
}

void hooklever_on_update(g_s *g, obj_s *o)
{
    hooklever_s *hl = (hooklever_s *)o->mem;
    if (o->state != 0) return;
    if (!(hl->dir_x | hl->dir_y)) return;

    obj_s *ohook = obj_get_tagged(g, OBJ_TAG_HOOK);
    obj_s *ohero = obj_get_hero(g);
    if (ohero && ohook) {
        rec_i32 rhook = obj_aabb(ohook);
        rhook.x--;
        rhook.y--;
        rhook.w += 2;
        rhook.h += 2;
        if (overlap_rec(rhook, obj_aabb(o)) &&
            80 <= hero_hook_pulling_force(g, ohero)) {
            hooklever_pull(g, o);
        }
    }

    if (hl->moved & !hl->reset_blocked) {
        o->subtimer++;
        if (10 <= o->subtimer) {
            switch (o->subID) {
            case HOOKLEVER_SWITCH: {
                hl->moved--;
                obj_move(g, o, -hl->dir_x, -hl->dir_y);
                break;
            }
            case HOOKLEVER_SPRING: {
                if (o->timer) {
                    o->timer--;
                    i32 dt = (hl->moved_max * o->timer) / hl->ticks_reset - hl->moved;
                    obj_move(g, o, hl->dir_x * dt, hl->dir_y * dt);
                    hl->moved += dt;
                }
                break;
            }
            }
        }
    }
}

void hooklever_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HOOKLEVER;
    o->flags = OBJ_FLAG_RENDER_AABB;

    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->w            = 32;
    o->h            = 32;
    o->mass         = 2;
    hooklever_s *hl = (hooklever_s *)o->mem;
    hl->moved_max   = map_obj_i32(mo, "Distance_Moved");
    o->trigger      = map_obj_i32(mo, "trigger");
    i32 ticks_reset = map_obj_i32(mo, "Ticks_Reset");
    if (ticks_reset) {
        o->subID        = HOOKLEVER_SPRING;
        hl->ticks_reset = ticks_reset;
    } else {
        o->subID = HOOKLEVER_SWITCH;
    }

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
    hl->saveID        = map_obj_i32(mo, "saveID");
    hl->dir_x         = map_obj_i32(mo, "DX");
    hl->dir_y         = map_obj_i32(mo, "DY");

    if (saveID_has(g, hl->saveID)) { // saveid has
        o->pos.x += hl->dir_x * hl->moved_max;
        o->pos.y += hl->dir_y * hl->moved_max;
        o->state = 1;
    }
}

ratio_i32 hooklever_spring_ratio(obj_s *o)
{
    ratio_i32 r = {0};
    if (o->subID == HOOKLEVER_SWITCH) return r;

    hooklever_s *hl = (hooklever_s *)o->mem;
    r.num           = o->timer;
    r.den           = hl->ticks_reset;
    return r;
}