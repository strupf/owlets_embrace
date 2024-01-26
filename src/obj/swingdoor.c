// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A swinging door which blocks the path when closed

#include "game.h"

enum {
    SWINGDOOR_OPEN   = 0,
    SWINGDOOR_CLOSED = 1,
};

#define SWINGDOOR_TICKS     8
#define SWINGDOOR_KEY_RANGE 16

typedef struct {
    int trigger_to_open;
    int trigger_to_close;
    int key_to_open;
} swingdoor_s;

obj_s *swingdoor_create(game_s *g)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_DOOR_SWING;
    o->flags           = OBJ_FLAG_SPRITE;
    o->render_priority = -1;
    o->w               = 16;
    o->h               = 32;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_MISCOBJ, 0, 112, 64, 32);
    spr->offs.x          = -16;
    return o;
}

static void swingdoor_set_blocks(game_s *g, obj_s *o)
{
    int tx     = o->pos.x >> 4;
    int ty     = o->pos.y >> 4;
    int ny     = o->h >> 4;
    int to_set = o->state == SWINGDOOR_CLOSED ? TILE_BLOCK : TILE_EMPTY;

    for (int k = 0; k < ny; k++) {
        g->tiles[tx + (ty + k) * g->tiles_x].collision = to_set;
    }

    if (o->state == SWINGDOOR_CLOSED) {
        game_on_solid_appear(g);
    }
}

void swingdoor_load(game_s *g, map_obj_s *mo)
{
    obj_s       *o  = swingdoor_create(g);
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
        swingdoor_set_blocks(g, o);
    }

    if (map_obj_bool(mo, "Interactable")) {
        o->flags |= OBJ_FLAG_INTERACTABLE;
    }
    od->trigger_to_open  = map_obj_i32(mo, "trigger_to_open");
    od->trigger_to_close = map_obj_i32(mo, "trigger_to_close");
    od->key_to_open      = map_obj_i32(mo, "KeyID");
}

void swingdoor_toggle(game_s *g, obj_s *o)
{
    snd_play_ext(SNDID_DOOR_SQUEEK, 1.f, rngr_f32(0.4f, 0.6f));
    o->timer = 0;
    o->state = 1 - o->state;
    swingdoor_set_blocks(g, o);
}

void swingdoor_on_update(game_s *g, obj_s *o)
{
    o->timer++;
    if (o->state == SWINGDOOR_OPEN) return;
    swingdoor_s *od    = (swingdoor_s *)o->mem;
    int          keyID = od->key_to_open;
    if (keyID == 0) return;
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;
    inventory_s *inv = &g->inventory;
    if (!inventory_count_of(inv, keyID)) return;

    rec_i32 rkey = obj_aabb(o);
    rkey.x -= SWINGDOOR_KEY_RANGE;
    rkey.w += SWINGDOOR_KEY_RANGE << 1;
    if (overlap_rec(rkey, obj_aabb(ohero))) {
        inventory_rem(inv, keyID, 1);
        swingdoor_toggle(g, o);
    }
}

void swingdoor_on_trigger(game_s *g, obj_s *o, int trigger)
{
    swingdoor_s *od = (swingdoor_s *)o->mem;

    if ((o->state == SWINGDOOR_OPEN && trigger == od->trigger_to_close) ||
        (o->state == SWINGDOOR_CLOSED && trigger == od->trigger_to_open)) {
        swingdoor_toggle(g, o);
    }
}

void swingdoor_on_interact(game_s *g, obj_s *o)
{
    if (o->timer <= SWINGDOOR_TICKS) return;
    swingdoor_toggle(g, o);
}

void swingdoor_on_animate(game_s *g, obj_s *o)
{
    if (o->timer == SWINGDOOR_TICKS) {
        f32 pitch = o->state == SWINGDOOR_OPEN ? 0.5f : 1.f;
        snd_play_ext(SNDID_DOOR_TOGGLE, 1.f, pitch);
    }
    int t  = min_i(o->timer, SWINGDOOR_TICKS);
    int fr = (t * 2) / SWINGDOOR_TICKS;

    if (o->state == SWINGDOOR_CLOSED) {
        fr = 2 - fr;
    }
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec.r.x        = 64 * fr;
}