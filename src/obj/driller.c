// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define DRILLER_SCAN_LEN 128 // length of scan box
#define DRILLER_SCAN_W   32  // width of scan box
#define DRILLER_V_PX     8   // pixels per tick
#define DRILLER_W        32  // hitbox dimensions
#define DRILLER_H        16

enum {
    DRILLER_LURK, // is in spawn
    DRILLER_PREPARE_DASH,
    DRILLER_DASH,
    DRILLER_HIDE
};

typedef struct {
    u8     dir;
    u8     spawn_from;
    u8     spawn_to;
    u8     n_spawns;
    obj_s *spawns[8];
} driller_s;

static_assert(sizeof(driller_s) <= OBJ_MEM_BYTES, "Driller size");

obj_s *driller_create(g_s *g, obj_s *ds);

void drillers_setup(g_s *g)
{
    for (obj_each(g, i)) {
        if (i->ID != OBJID_DRILLERSPAWN) continue;
        if (!i->action) continue; // is not master

        obj_s     *o             = driller_create(g, i);
        driller_s *d             = (driller_s *)o->mem;
        d->spawns[d->n_spawns++] = i; // self reference

        for (obj_each(g, k)) {
            if (k->ID != OBJID_DRILLERSPAWN) continue;
            if (k->action || k->substate != i->substate) continue;

            d->spawns[d->n_spawns++] = k;
        }
    }
}

void drillerspawn_load(g_s *g, map_obj_s *mo)
{
    obj_s *o    = obj_create(g);
    o->UUID     = mo->UUID;
    o->ID       = OBJID_DRILLERSPAWN;
    o->enemy    = enemy_default();
    o->w        = 32;
    o->h        = 32;
    o->pos.x    = mo->x;
    o->pos.y    = mo->y;
    o->substate = map_obj_i32(mo, "refID");

    if (map_obj_bool(mo, "spawn")) {
        o->action = 1;
    }
    o->render_priority = RENDER_PRIO_OWL + 1;
    o->n_sprites       = 1;
    obj_sprite_s *spr  = &o->sprites[0];
    spr->trec          = asset_texrec(TEXID_DRILLER, 0, 0, 48, 48);
    spr->offs.x        = -8;
    spr->offs.y        = -8;

    if (0) {
    } else if (mo->hash == hash_str("drillerspawn_u")) {
        o->subID = DIR_Y_NEG;
        spr->offs.y -= 8;
        spr->trec.x = 1 * 48;
    } else if (mo->hash == hash_str("drillerspawn_d")) {
        o->subID = DIR_Y_POS;
        spr->offs.y += 8;
        spr->trec.x = 3 * 48;
    } else if (mo->hash == hash_str("drillerspawn_l")) {
        o->subID = DIR_X_NEG;
        spr->offs.x -= 8;
        spr->trec.x = 0 * 48;
    } else if (mo->hash == hash_str("drillerspawn_r")) {
        o->subID = DIR_X_POS;
        spr->offs.x += 8;
        spr->trec.x = 2 * 48;
    }
}

void driller_on_update(g_s *g, obj_s *o);
void driller_on_animate(g_s *g, obj_s *o);
void driller_set_flags_dash(obj_s *o);
void driller_set_flags_hide(obj_s *o);

void driller_set_to_spawn(obj_s *odriller, obj_s *ospawn)
{
    driller_s *d    = (driller_s *)odriller->mem;
    v2_i32     v1   = obj_pos_center(ospawn);
    odriller->pos.x = v1.x - odriller->w / 2;
    odriller->pos.y = v1.y - odriller->h / 2;
    d->dir          = (u8)ospawn->subID;
    v2_i32 dir      = dir_v2(d->dir);
    odriller->pos.x += dir.x * 12;
    odriller->pos.y += dir.y * 12;
}

void driller_set_flags_dash(obj_s *o)
{
    o->flags = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY;
}

void driller_set_flags_hide(obj_s *o)
{
    o->flags = 0;
}

obj_s *driller_create(g_s *g, obj_s *ds)
{
    obj_s     *o = obj_create(g);
    driller_s *d = (driller_s *)o->mem;
    o->ID        = OBJID_DRILLER;
    driller_set_flags_hide(o);
    o->enemy      = enemy_default();
    o->substate   = +1;
    o->on_animate = driller_on_animate;
    o->on_update  = driller_on_update;
    d->dir        = (u8)ds->subID;

    switch (d->dir) {
    case DIR_Y_NEG:
    case DIR_Y_POS:
        o->w = DRILLER_H;
        o->h = DRILLER_W;
        break;
    case DIR_X_POS:
    case DIR_X_NEG:
        o->w = DRILLER_W;
        o->h = DRILLER_H;
        break;
    }

    driller_set_to_spawn(o, ds);
    return o;
}

void driller_on_update(g_s *g, obj_s *o)
{
    driller_s *d      = (driller_s *)o->mem;
    obj_s     *ospawn = d->spawns[d->spawn_from];

    switch (o->state) {
    case DRILLER_LURK: {
        obj_s *ohero = obj_get_owl(g);
        if (!ohero) break;

        rec_i32 rscan = obj_aabb(ospawn);

        switch (ospawn->subID) {
        case DIR_Y_NEG:
            rscan.x += (ospawn->w - DRILLER_SCAN_W) >> 1;
            rscan.w = DRILLER_SCAN_W;
            rscan.y -= DRILLER_SCAN_LEN;
            rscan.h += DRILLER_SCAN_LEN;
            break;
        case DIR_Y_POS:
            rscan.x += (ospawn->w - DRILLER_SCAN_W) >> 1;
            rscan.w = DRILLER_SCAN_W;
            rscan.h += DRILLER_SCAN_LEN;
            break;
        case DIR_X_POS:
            rscan.y += (ospawn->h - DRILLER_SCAN_W) >> 1;
            rscan.h = DRILLER_SCAN_W;
            rscan.w += DRILLER_SCAN_LEN;
            break;
        case DIR_X_NEG:
            rscan.y += (ospawn->h - DRILLER_SCAN_W) >> 1;
            rscan.h = DRILLER_SCAN_W;
            rscan.x -= DRILLER_SCAN_LEN;
            rscan.w += DRILLER_SCAN_LEN;
            break;
        }

        if (!overlap_rec(obj_aabb(ohero), rscan)) break;

        i32 n_spawns_possible  = 0;
        u8  spawns_possible[8] = {0};

        for (i32 n = 0; n < d->n_spawns; n++) {
            if (n == d->spawn_from) continue;

            if (d->spawns[n]->subID == dir_nswe_opposite(ospawn->subID)) {
                spawns_possible[n_spawns_possible++] = n;
                break;
            }
        }

        d->spawn_to = spawns_possible[rngr_i32(0, n_spawns_possible - 1)];
        o->state    = DRILLER_PREPARE_DASH;
        o->timer    = 0;
        break;
    }
    case DRILLER_PREPARE_DASH: {
        o->timer++;
        if (6 <= o->timer) {
            o->state = DRILLER_DASH;
            o->timer = 0;
            driller_set_flags_dash(o);
        }
        break;
    }
    case DRILLER_DASH: {
        v2_i32 v0      = obj_pos_center(ospawn);
        v2_i32 v1      = obj_pos_center(d->spawns[d->spawn_to]);
        v2_i32 pp      = obj_pos_center(o);
        v2_i32 k1      = v2_i32_sub(v1, v0);
        v2_i32 kk      = v2_i32_sub(pp, v0);
        bool32 arrived = 0;

        switch (ospawn->subID) {
        case DIR_X_NEG: {
            if (pp.x < v1.x) {
                arrived = 1;
                break;
            }

            i32 i0    = -kk.x;
            i32 i1    = -k1.x;
            i32 y_trg = lerp_i32(v0.y, v1.y, clamp_i32(i0, 0, i1), i1);
            obj_move(g, o, -DRILLER_V_PX, y_trg - pp.y);
            break;
        }
        case DIR_X_POS: {
            if (pp.x > v1.x) {
                arrived = 1;
                break;
            }

            i32 i0    = +kk.x;
            i32 i1    = +k1.x;
            i32 y_trg = lerp_i32(v0.y, v1.y, clamp_i32(i0, 0, i1), i1);
            obj_move(g, o, +DRILLER_V_PX, y_trg - pp.y);
            break;
        }
        case DIR_Y_NEG: {
            if (pp.y < v1.y) {
                arrived = 1;
                break;
            }

            i32 i0    = -kk.y;
            i32 i1    = -k1.y;
            i32 x_trg = lerp_i32(v0.x, v1.x, clamp_i32(i0, 0, i1), i1);
            obj_move(g, o, x_trg - pp.x, -DRILLER_V_PX);
            break;
        }
        case DIR_Y_POS: {
            if (pp.y > v1.y) {
                arrived = 1;
                break;
            }

            i32 i0    = +kk.y;
            i32 i1    = +k1.y;
            i32 x_trg = lerp_i32(v0.x, v1.x, clamp_i32(i0, 0, i1), i1);
            obj_move(g, o, x_trg - pp.x, +DRILLER_V_PX);
            break;
        }
        }

        if (arrived) {
            driller_set_flags_hide(o);
            o->state      = DRILLER_HIDE;
            o->timer      = 0;
            d->spawn_from = d->spawn_to;
            driller_set_to_spawn(o, d->spawns[d->spawn_to]);
        }
        break;
    }
    case DRILLER_HIDE:
        o->timer++;
        if (25 <= o->timer) {
            o->state = DRILLER_LURK;
            o->timer = 0;
        }
        break;
    }
}

void driller_on_animate(g_s *g, obj_s *o)
{
    if (o->state == DRILLER_HIDE) {
        o->n_sprites = 0;
        return;
    }

    driller_s *d = (driller_s *)o->mem;
    o->animation++;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->offs.x       = (o->w - 64) / 2;
    spr->offs.y       = (o->h - 64) / 2;

    i32 fy = 0;
    i32 fx = 0;

    switch (d->dir) {
    case DIR_Y_NEG: fy = 2; break;
    case DIR_Y_POS: fy = 3; break;
    case DIR_X_POS: fy = 1; break;
    case DIR_X_NEG: fy = 0; break;
    }

    switch (o->state) {
    case DRILLER_LURK:
        if ((o->animation >> 3) & 1) {
            v2_i32 dir = dir_v2(d->dir);
            spr->offs.x += dir.x * 2;
            spr->offs.y += dir.y * 2;
        }
        break;
    case DRILLER_PREPARE_DASH: {
        v2_i32 dir = dir_v2(d->dir);
        spr->offs.x -= dir.x * o->timer * 2;
        spr->offs.y -= dir.y * o->timer * 2;
        break;
    }
    case DRILLER_DASH: {
        i32 f = (o->animation / 3) & 3;
        fx    = 2 - (f < 3 ? f : 1);
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_DRILLER, fx * 64, 48 + fy * 64, 64, 64);
}