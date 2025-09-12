// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    VINEBLOCKADE_NULL,
    VINEBLOCKADE_HOR,
    VINEBLOCKADE_VER
};

enum {
    VINEBLOCKADE_ST_INACTIVE,
    VINEBLOCKADE_ST_APPEAR,
    VINEBLOCKADE_ST_ACTIVE,
    VINEBLOCKADE_ST_DISAPPEAR
};

typedef struct {
    i32 trigger_to_delete;
    i32 trigger_to_spawn;
} vineblockade_s;

void vineblockade_on_trigger(g_s *g, obj_s *o, i32 trigger);
void vineblockade_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void vineblockade_on_animate(g_s *g, obj_s *o);

void vineblockade_load(g_s *g, map_obj_s *mo)
{
    i32 s0 = map_obj_i32(mo, "only_if_saveID");
    i32 s1 = map_obj_i32(mo, "only_if_not_saveID");
    if (s0 && !save_event_exists(g, s0)) return;
    if (s1 && save_event_exists(g, s1)) return;

    obj_s          *o = obj_create(g);
    vineblockade_s *v = (vineblockade_s *)o->mem;

    o->UUID            = mo->UUID;
    o->ID              = OBJID_VINEBLOCKADE;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->render_priority = RENDER_PRIO_OWL + 1;
    if (mo->hash == hash_str("vineblockade_hor")) {
        o->subID = VINEBLOCKADE_HOR;
    }
    if (mo->hash == hash_str("vineblockade_ver")) {
        o->subID = VINEBLOCKADE_VER;
    }

    v->trigger_to_delete = map_obj_i32(mo, "delete_on_trigger");
    v->trigger_to_spawn  = map_obj_i32(mo, "spawn_on_trigger");
    o->on_trigger        = vineblockade_on_trigger;
    o->on_animate        = vineblockade_on_animate;

    if (map_obj_bool(mo, "active")) {
        o->on_draw = vineblockade_on_draw;
        o->state   = VINEBLOCKADE_ST_ACTIVE;
        o->flags |= OBJ_FLAG_SOLID | OBJ_FLAG_HURT_ON_TOUCH;
        game_on_solid_appear(g);
    }
}

void vineblockade_on_animate(g_s *g, obj_s *o)
{
    o->timer++;

    switch (o->state) {
    case VINEBLOCKADE_ST_APPEAR: {
        if (12 <= o->timer) {
            o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
            o->state   = VINEBLOCKADE_ST_ACTIVE;
            o->on_draw = vineblockade_on_draw;
            o->timer   = 0;
        }
        break;
    }
    case VINEBLOCKADE_ST_DISAPPEAR: {
        o->timer++;
        if (12 <= o->timer) {
            o->state   = VINEBLOCKADE_ST_INACTIVE;
            o->on_draw = 0;
            o->timer   = 0;
        }
        break;
    }
    }
}

void vineblockade_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    pos = v2_i32_add(o->pos, cam);
    i32       f   = 64 * frame_from_ticks_pingpong(o->timer >> 4, 3);

    switch (o->subID) {
    case VINEBLOCKADE_HOR: {
        i32      n_t   = o->w >> 4;
        texrec_s tr    = asset_texrec(TEXID_VINES, 64, f, 16, 64);
        v2_i32   trpos = {pos.x, pos.y - 16};

        for (i32 n = 0; n < n_t; n++) {
            tr.x = 64 + (n & 3) * 16;
            gfx_spr(ctx, tr, trpos, 0, 0);
            trpos.x += 16;
        }
        break;
    }
    case VINEBLOCKADE_VER: {
        i32      n_t   = o->h >> 4;
        texrec_s tr    = asset_texrec(TEXID_VINES, 0, f, 64, 16);
        v2_i32   trpos = {pos.x - 16, pos.y};

        for (i32 n = 0; n < n_t; n++) {
            tr.y = f + (n & 3) * 16;
            gfx_spr(ctx, tr, trpos, 0, 0);
            trpos.y += 16;
        }
        break;
    }
    }
}

void vineblockade_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    vineblockade_s *v    = (vineblockade_s *)o->mem;
    bool32          poof = 0;

    if (trigger == v->trigger_to_delete &&
        (o->state == VINEBLOCKADE_ST_ACTIVE || o->state == VINEBLOCKADE_ST_APPEAR)) {
        poof     = 1;
        o->timer = 0;
        o->state = VINEBLOCKADE_ST_DISAPPEAR;
        o->flags &= ~(OBJ_FLAG_SOLID | OBJ_FLAG_HURT_ON_TOUCH);
    }

    if (trigger == v->trigger_to_spawn &&
        (o->state == VINEBLOCKADE_ST_INACTIVE || o->state == VINEBLOCKADE_ST_DISAPPEAR)) {
        poof     = 1;
        o->timer = 0;
        o->state = VINEBLOCKADE_ST_APPEAR;
        o->flags |= OBJ_FLAG_SOLID;
        game_on_solid_appear(g);
    }

    if (poof) {
        switch (o->subID) {
        case VINEBLOCKADE_HOR: {
            i32    n_t   = o->w >> 2;
            v2_i32 trpos = {o->pos.x + 16, o->pos.y + 16};

            for (i32 n = 0; n < o->w; n += 32) {
                animobj_create(g, trpos, ANIMOBJ_EXPLOSION_2);
                trpos.x += 32;
            }
            break;
        }
        case VINEBLOCKADE_VER: {
            i32    n_t   = o->h >> 2;
            v2_i32 trpos = {o->pos.x + 16, o->pos.y + 16};

            for (i32 n = 0; n < o->h; n += 32) {
                animobj_create(g, trpos, ANIMOBJ_EXPLOSION_2);
                trpos.y += 32;
            }
            break;
        }
        }
    }
}