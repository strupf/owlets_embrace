// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CRUMBLE_STATE_IDLE,
    CRUMBLE_STATE_BREAKING,
    CRUMBLE_STATE_RESPAWNING,
};

#define CRUMBLE_TICKS_BREAK   75
#define CRUMBLE_TICKS_RESPAWN 100

static void crumbleblock_start_breaking(obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE) {
        o->state = CRUMBLE_STATE_BREAKING;
        o->timer = CRUMBLE_TICKS_BREAK;
    }
}

void crumbleblock_on_hooked(obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE && o->substate == TILE_BLOCK) {
        crumbleblock_start_breaking(o);
    }
}

void crumbleblock_on_update(game_s *g, obj_s *o)
{
    switch (o->state) {
    case CRUMBLE_STATE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;
        rec_i32 r = {o->pos.x, o->pos.y, o->w, 1};
        if (0 <= ohero->vel_q8.y && overlap_rec(obj_rec_bottom(ohero), r)) {
            crumbleblock_start_breaking(o);
        }
        break;
    }
    case CRUMBLE_STATE_BREAKING: {
        o->timer--;
        if (0 < o->timer) break;
        tile_map_set_collision(g, obj_aabb(o), 0, 0);
        o->flags &= ~OBJ_FLAG_SPRITE;
        o->state = CRUMBLE_STATE_RESPAWNING;
        o->timer = CRUMBLE_TICKS_RESPAWN;
        break;
    }
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (0 < o->timer) break;
        o->state = CRUMBLE_STATE_IDLE;
        o->flags |= OBJ_FLAG_SPRITE;
        tile_map_set_collision(g, obj_aabb(o), o->substate, o->substate == TILE_BLOCK ? TILE_TYPE_DIRT : 0);
        game_on_solid_appear(g);
        break;
    }
    }
}

void crumbleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    if (o->state == CRUMBLE_STATE_RESPAWNING) return;

    gfx_ctx_s ctx = gfx_ctx_display();
    i32       rx  = (o->substate == TILE_ONE_WAY ? 64 : 0);
    texrec_s  tr  = asset_texrec(TEXID_CRUMBLE, rx, 0, 16, 48);
    i32       n   = o->w >> 4;
    v2_i32    p   = v2_add(o->pos, cam);
    p.y -= 16;

    i32 ry = o->state == CRUMBLE_STATE_BREAKING ? 2 : 0;
    if (n == 1) {
        tr.r.y = rngr_i32(0, ry) * 48;
        gfx_spr(ctx, tr, p, 0, 0);
    } else {
        tr.r.x = 16 + rx;
        tr.r.y = rngr_i32(0, ry) * 48;
        gfx_spr(ctx, tr, p, 0, 0);
        p.x += 16;
        for (i32 i = 1; i < n - 1; i++) {
            tr.r.x = 32 + rx;
            tr.r.y = rngr_i32(0, ry) * 48;
            gfx_spr(ctx, tr, p, 0, 0);
            p.x += 16;
        }
        tr.r.x = 48 + rx;
        tr.r.y = rngr_i32(0, ry) * 48;
        gfx_spr(ctx, tr, p, 0, 0);
    }
}

void crumbleblock_load(game_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_CRUMBLEBLOCK;
    o->on_update       = crumbleblock_on_update;
    o->on_draw         = crumbleblock_on_draw;
    o->render_priority = 0;
    o->w               = 16;
    o->state           = CRUMBLE_STATE_IDLE;
    o->substate        = TILE_BLOCK;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    i32 n              = o->w >> 4;
    o->substate        = map_obj_bool(mo, "Platform") ? TILE_ONE_WAY : TILE_BLOCK;

    tile_map_set_collision(g, obj_aabb(o), o->substate, o->substate == TILE_BLOCK ? TILE_TYPE_DIRT : 0);
}
