// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *weapon_pickup_create(g_s *g);
void   weapon_pickup_on_update(g_s *g, obj_s *o);
void   weapon_pickup_on_animate(g_s *g, obj_s *o);
void   weapon_pickup_on_interact(g_s *g, obj_s *o);

obj_s *weapon_pickup_create(g_s *g)
{
    obj_s *o          = obj_create(g);
    o->ID             = OBJ_ID_WEAPON_PICKUP;
    o->w              = 16;
    o->h              = 16;
    o->flags          = OBJ_FLAG_MOVER | OBJ_FLAG_SPRITE | OBJ_FLAG_INTERACTABLE;
    o->grav_q8.y      = 60;
    o->drag_q8.y      = 256;
    o->on_animate     = weapon_pickup_on_animate;
    o->on_update      = weapon_pickup_on_update;
    o->on_interact    = weapon_pickup_on_interact;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_HERO, 0, 21 * 64, 64, 64);
    spr->offs.x       = -32;
    spr->offs.y       = -48;
    return o;
}

void weapon_pickup_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = weapon_pickup_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
}

void weapon_pickup_place(g_s *g, obj_s *ohero)
{
    hero_s *h       = &g->hero_mem;
    h->holds_weapon = 0;
    obj_s *o        = weapon_pickup_create(g);
    v2_i32 oherop   = obj_pos_bottom_center(ohero);
    o->pos.x        = oherop.x - 8;
    o->pos.y        = oherop.y - 16;
    snd_play(SNDID_WEAPON_UNEQUIP, 1.f, 1.f);
}

void weapon_pickup_on_pickup(g_s *g, obj_s *o, obj_s *ohero)
{
    hero_s *h = &g->hero_mem;
    hero_action_ungrapple(g, o);
    h->holds_weapon = 1;
    obj_delete(g, o);
    snd_play(SNDID_WEAPON_EQUIP, 0.7f, 0.8f);
}

void weapon_pickup_on_update(g_s *g, obj_s *o)
{
    if (o->interactable_hovered) {
        if (o->subtimer == 0) {
            if (50 <= o->timer) {
                o->subtimer = 1;
            } else {
                o->timer++;
            }
        } else {
            o->subtimer++;
            if (6 <= o->subtimer) {
                o->subtimer = 0;
                o->timer    = 0;
            }
        }
    } else {
        o->subtimer = 0;
        o->timer    = 49;
    }

    if (obj_grounded(g, o)) {
    }

    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;
}

void weapon_pickup_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];

    switch (o->interactable_hovered) {
    case 0:
        spr->trec.r.x = 0;
        break;
    case 1:
        if (o->subtimer == 0) {
            spr->trec.r.x = 0;
        } else {
            spr->trec.r.x = 64 * ease_lin(1, 3, o->subtimer, 5);
        }
        break;
    }
}

void weapon_pickup_on_interact(g_s *g, obj_s *o)
{
    weapon_pickup_on_pickup(g, o, obj_get_tagged(g, OBJ_TAG_HERO));
}