// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A swinging door which blocks the path when closed

#include "game.h"

enum {
    SWINGDOOR_OPEN   = 0,
    SWINGDOOR_CLOSED = 1,
};

#define SWINGDOOR_TICKS          8
#define SWINGDOOR_KEY_RANGE      16
#define SWINGDOOR_KEY_OPEN_TICKS 50

typedef struct {
    i32    trigger_to_open;
    i32    trigger_to_close;
    i32    key_to_open;
    i32    key_opener_ticks;
    v2_i32 key_open_origin;
} swingdoor_s;

void swingdoor_toggle(g_s *g, obj_s *o)
{
    snd_play(SNDID_DOOR_SQUEEK, 1.f, rngr_f32(0.4f, 0.6f));
    o->timer   = 0;
    o->state   = 1 - o->state;
    int to_set = o->state == SWINGDOOR_CLOSED ? TILE_BLOCK : TILE_EMPTY;
    tile_map_set_collision(g, obj_aabb(o), to_set, 0);
}

void swingdoor_on_update(g_s *g, obj_s *o)
{
    o->timer++;
    swingdoor_s *od = (swingdoor_s *)o->mem;
    if (o->state == SWINGDOOR_OPEN) return;
    if (od->key_to_open == 0) return;

    if (od->key_opener_ticks == 0) {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) return;

        if (!hero_inv_count_of(g, od->key_to_open)) return;

        rec_i32 rkey = {o->pos.x - SWINGDOOR_KEY_RANGE,
                        o->pos.y,
                        o->w + (SWINGDOOR_KEY_RANGE << 1),
                        o->h};
        if (overlap_rec(rkey, obj_aabb(ohero))) {
            hero_inv_rem(g, od->key_to_open, 1);
            od->key_open_origin = obj_pos_center(ohero);
            od->key_open_origin.y -= 64;
            od->key_opener_ticks = 1;
            snd_play(SNDID_DOOR_KEY_SPAWNED, 1.f, 1.f);
        }
    } else {
        od->key_opener_ticks++;
        if (SWINGDOOR_KEY_OPEN_TICKS <= od->key_opener_ticks) {
            od->key_opener_ticks = 0;
            swingdoor_toggle(g, o);
            snd_play(SNDID_DOOR_UNLOCKED, 1.f, 1.f);
        }
    }
}

void swingdoor_on_animate(g_s *g, obj_s *o)
{
    if (o->timer == SWINGDOOR_TICKS) {
        f32 pitch = o->state == SWINGDOOR_OPEN ? 0.5f : 1.f;
        snd_play(SNDID_DOOR_TOGGLE, 1.f, pitch);
    }
    int t  = min_i32(o->timer, SWINGDOOR_TICKS);
    int fr = (t * 2) / SWINGDOOR_TICKS;

    if (o->state == SWINGDOOR_CLOSED) {
        fr = 2 - fr;
    }

    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec.r.x     = 64 * fr;

    swingdoor_s *od = (swingdoor_s *)o->mem;
    if (od->key_opener_ticks == 0) return;
    o->n_sprites       = 2;
    obj_sprite_s *sprk = &o->sprites[1];
    sprk->trec         = asset_texrec(TEXID_MISCOBJ, 224, 64, 16, 32);

    int    i0   = od->key_opener_ticks;
    int    i1   = SWINGDOOR_KEY_OPEN_TICKS;
    v2_i32 keyd = {ease_in_out_quad(od->key_open_origin.x, o->pos.x, i0, i1),
                   ease_in_out_quad(od->key_open_origin.y, o->pos.y, i0, i1)};
    sprk->offs  = v2_i16_from_i32(v2_sub(keyd, o->pos));
}

void swingdoor_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    swingdoor_s *od = (swingdoor_s *)o->mem;

    if ((o->state == SWINGDOOR_OPEN && trigger == od->trigger_to_close) ||
        (o->state == SWINGDOOR_CLOSED && trigger == od->trigger_to_open)) {
        swingdoor_toggle(g, o);
    }
}

void swingdoor_on_interact(g_s *g, obj_s *o)
{
    if (o->timer <= SWINGDOOR_TICKS) return;
    swingdoor_toggle(g, o);
}

void swingdoor_load(g_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_DOOR_SWING;
    o->flags           = OBJ_FLAG_SPRITE;
    o->on_update       = swingdoor_on_update;
    o->on_animate      = swingdoor_on_animate;
    o->on_trigger      = swingdoor_on_trigger;
    o->on_interact     = swingdoor_on_interact;
    o->render_priority = 1;
    o->w               = 16;
    o->h               = 32;

    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_MISCOBJ, 0, 112, 64, 32);
    spr->offs.x       = -16;

    swingdoor_s *od = (swingdoor_s *)o->mem;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->w            = mo->w;
    o->h            = mo->h;
    o->timer        = SWINGDOOR_TICKS + 1;

    if (map_obj_bool(mo, "Opened")) {
        o->state = SWINGDOOR_OPEN;
    } else {
        o->state = SWINGDOOR_CLOSED;
        tile_map_set_collision(g, obj_aabb(o), TILE_BLOCK, 0);
    }

    if (map_obj_bool(mo, "Interactable")) {
        o->flags |= OBJ_FLAG_INTERACTABLE;
    }
    od->trigger_to_open  = map_obj_i32(mo, "trigger_to_open");
    od->trigger_to_close = map_obj_i32(mo, "trigger_to_close");
    od->key_to_open      = map_obj_i32(mo, "KeyID");
}
