// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 y_top;
    i32 y_og;
} steam_platform_s;

#define STEAM_PLAT_TICKS_UP   20
#define STEAM_PLAT_TICKS_HOLD 70
#define STEAM_PLAT_TICKS_DOWN 20

enum {
    STEAM_PLATFORM_IDLE,
    STEAM_PLATFORM_UP,
    STEAM_PLATFORM_HOLD,
    STEAM_PLATFORM_DOWN,
};

void steam_platform_on_update(g_s *g, obj_s *o);
void steam_platform_on_trigger(g_s *g, obj_s *o, i32 trigger);
void steam_platform_on_animate(g_s *g, obj_s *o);
void steam_platform_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void steam_platform_on_burst(g_s *g, obj_s *o);

void steam_platform_load(g_s *g, map_obj_s *mo)
{
    obj_s            *o  = obj_create(g);
    steam_platform_s *sp = (steam_platform_s *)o->mem;
    o->ID                = OBJ_ID_STEAM_PLATFORM;
    o->flags             = OBJ_FLAG_RENDER_AABB;
    o->w                 = mo->w;
    o->h                 = mo->h;
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
    o->mass              = 1;
    o->on_update         = steam_platform_on_update;
    o->on_animate        = steam_platform_on_animate;
    o->on_draw           = steam_platform_on_draw;
    o->on_trigger        = steam_platform_on_trigger;
    sp->y_og             = o->pos.y;
    sp->y_top            = sp->y_og - 120;
}

void steam_platform_on_update(g_s *g, obj_s *o)
{
    steam_platform_s *sp = (steam_platform_s *)o->mem;

    switch (o->state) {
    case STEAM_PLATFORM_IDLE:
        o->timer++;
        if (100 <= o->timer)
            steam_platform_on_burst(g, o);
        break;
    case STEAM_PLATFORM_UP: {
        o->timer++;
        i32 tp      = ease_out_quad(sp->y_og, sp->y_top,
                                    o->timer, STEAM_PLAT_TICKS_UP);
        o->tomove.y = tp - o->pos.y;
        if (STEAM_PLAT_TICKS_UP <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case STEAM_PLATFORM_HOLD: {
        o->timer++;
        i32 tp      = sp->y_top + ((o->timer >> 2) & 1) * 2;
        o->tomove.y = tp - o->pos.y;
        if (STEAM_PLAT_TICKS_HOLD <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case STEAM_PLATFORM_DOWN: {
        o->timer++;
        i32 tp      = ease_in_quad(sp->y_top, sp->y_og,
                                   o->timer, STEAM_PLAT_TICKS_DOWN);
        o->tomove.y = tp - o->pos.y;
        if (STEAM_PLAT_TICKS_DOWN <= o->timer) {
            o->state = STEAM_PLATFORM_IDLE;
            o->timer = 0;
        }
        break;
    }
    }
}

void steam_platform_on_burst(g_s *g, obj_s *o)
{
    o->state = STEAM_PLATFORM_UP;
    o->timer = 0;
}

void steam_platform_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
}

void steam_platform_on_animate(g_s *g, obj_s *o)
{
}

void steam_platform_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
}