// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BOMBPLANT_ST_BOMB,
    BOMBPLANT_ST_NO_BOMB,
    BOMBPLANT_ST_RESPAWNING,
    BOMBPLANT_ST_WIGGLE,
};

enum {
    BOMBPLANT_FLOOR,
    BOMBPLANT_CEILING,
    BOMBPLANT_LEFT,
    BOMBPLANT_RIGHT
};

typedef struct {
    i32 trigger_to_delete;
} bombplant_s;

#define BOMBPLANT_TICKS_WIGGLE     20
#define BOMBPLANT_TICKS_WAITING    150
#define BOMBPLANT_TICKS_RESPAWNING 14

void   bombplant_on_update(g_s *g, obj_s *o);
void   bombplant_on_animate(g_s *g, obj_s *o);
void   bombplant_on_hit(g_s *g, obj_s *o);
void   bombplant_on_trigger(g_s *g, obj_s *o, i32 trigger);
bool32 bombplant_pushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);

void bombplant_load(g_s *g, map_obj_s *mo)
{
    i32 sID = map_obj_i32(mo, "only_if_not_saveID");
    if (sID && save_event_exists(g, sID)) return;

    obj_s       *o         = obj_create(g);
    bombplant_s *b         = (bombplant_s *)o->mem;
    o->UUID                = mo->UUID;
    o->ID                  = OBJID_BOMBPLANT;
    o->on_update           = bombplant_on_update;
    o->on_animate          = bombplant_on_animate;
    o->on_trigger          = bombplant_on_trigger;
    b->trigger_to_delete   = map_obj_i32(mo, "trigger_to_delete");
    o->flags               = OBJ_FLAG_PUSHABLE_SOLID;
    o->on_pushpull_blocked = bombplant_pushpull_blocked;
    // o->render_priority     = RENDER_PRIO_OWL + 1;

    switch (map_obj_i32(mo, "dir")) {
    case 0: {
        o->subID = BOMBPLANT_FLOOR;
        o->w     = 16;
        o->h     = 16;
        obj_place_to_map_obj(o, mo, 0, +1);
        break;
    }
    case 1: {
        o->subID = BOMBPLANT_CEILING;
        o->w     = 16;
        o->h     = 16;
        obj_place_to_map_obj(o, mo, 0, -1);
        break;
    }
    case 2: {
        o->subID = BOMBPLANT_LEFT;
        o->w     = 16;
        o->h     = 16;
        obj_place_to_map_obj(o, mo, -1, 0);
        break;
    }
    case 3: {
        o->subID = BOMBPLANT_RIGHT;
        o->w     = 16;
        o->h     = 16;
        obj_place_to_map_obj(o, mo, +1, 0);
        break;
    }
    }
}

void bombplant_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case BOMBPLANT_ST_BOMB: {
        break;
    }
    case BOMBPLANT_ST_WIGGLE: {
        o->timer++;
        if (BOMBPLANT_TICKS_WIGGLE <= o->timer) {
            o->timer = 0;
            o->state = BOMBPLANT_ST_NO_BOMB;
        }
        break;
    }
    case BOMBPLANT_ST_NO_BOMB: {
        o->timer++;
        if (BOMBPLANT_TICKS_WAITING <= o->timer) {
            o->timer = 0;
            o->state = BOMBPLANT_ST_RESPAWNING;
        }
        break;
    }
    case BOMBPLANT_ST_RESPAWNING: {
        o->timer++;
        if (BOMBPLANT_TICKS_RESPAWNING <= o->timer) {
            o->timer = 0;
            o->state = BOMBPLANT_ST_BOMB;
            o->flags |= OBJ_FLAG_GRABBABLE_SOLID;
            game_on_solid_appear_ext(g, o);
        }
        break;
    }
    }
}

void bombplant_on_animate(g_s *g, obj_s *o)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    i32           w   = 64;
    i32           h   = 64;
    i32           fx  = 0;
    i32           fy  = 0;

    switch (o->state) {
    case BOMBPLANT_ST_BOMB: {
        fx = 5;
        fy = 0;
        break;
    }
    case BOMBPLANT_ST_WIGGLE: {
        fy = 1;
        fx = 1 + lerp_i32(0, 2, o->timer, BOMBPLANT_TICKS_WIGGLE);
        if (fx == 3) {
            fx = 1;
        }
        break;
    }
    case BOMBPLANT_ST_NO_BOMB: {
        fx = 0;
        fy = 0;
        break;
    }
    case BOMBPLANT_ST_RESPAWNING: {
        fy = 0;
        fx = lerp_i32(1, 4, o->timer, BOMBPLANT_TICKS_RESPAWNING);
        break;
    }
    }

    switch (o->subID) {
    case BOMBPLANT_FLOOR: {
        spr->offs.x = (o->w - w) / 2;
        spr->offs.y = (o->h - h);
        break;
    }
    case BOMBPLANT_CEILING: {
        fy += 2;
        spr->offs.x = (o->w - w) / 2;
        spr->offs.y = 0;
        break;
    }
    case BOMBPLANT_LEFT: {
        SWAP(i32, fx, fy);
        fy += 4;
        spr->offs.x = 0;
        spr->offs.y = (o->h - h) / 2;
        break;
    }
    case BOMBPLANT_RIGHT: {
        SWAP(i32, fx, fy);
        fy += 4;
        fx += 3;
        spr->offs.x = (o->w - w);
        spr->offs.y = (o->h - h) / 2;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_BOMBPLANT, fx * w, fy * h, w, h);
}

void bombplant_on_pickup(g_s *g, obj_s *o)
{
    o->state = BOMBPLANT_ST_WIGGLE;
    o->timer = 0;
    o->flags &= ~OBJ_FLAG_GRABBABLE_SOLID;
}

void bombplant_on_hit(g_s *g, obj_s *o)
{
    bombplant_on_pickup(g, o);
    v2_i32 p    = obj_pos_center(o);
    obj_s *ob   = bomb_create(g);
    ob->pos.x   = p.x - ob->w / 2;
    ob->pos.y   = p.y - ob->h / 2;
    ob->v_q12.y = -Q_VOBJ(3.0);
    ob->v_q12.x = rngr_sym_i32(Q_VOBJ(3.0));
}

void bombplant_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    bombplant_s *b = (bombplant_s *)o->mem;
    if (trigger == b->trigger_to_delete) {
        obj_delete(g, o);
    }
}

bool32 bombplant_pushpull_blocked(g_s *g, obj_s *o, i32 dt_x, i32 dt_y)
{
    return 1;
}