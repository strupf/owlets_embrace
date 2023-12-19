// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define SWITCH_ANIMATE_TICKS 4

enum {
    SWITCH_STATE_OFF,
    SWITCH_STATE_ON,
};

static void switch_set_sprite(obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];

    if (o->timer > 0) {
        spr->trec.r.x = 64;
    } else {
        spr->trec.r.x = 128 * o->state;
    }
}

obj_s *switch_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SWITCH;
    o->flags |= OBJ_FLAG_INTERACTABLE;
    o->flags |= OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
    o->trigger_off       = 1;
    o->trigger_on        = 0;
    o->state             = SWITCH_STATE_OFF;
    o->sprites[0].trec   = asset_texrec(TEXID_SWITCH, 0, 0, 64, 64);
    o->sprites[0].offs.x = -32;
    o->sprites[0].offs.y = -64;
    switch_set_sprite(o);
    return o;
}

void switch_on_animate(game_s *g, obj_s *o)
{
    if (o->timer > 0)
        o->timer--;
    switch_set_sprite(o);
}

void switch_on_interact(game_s *g, obj_s *o)
{
    if (o->switch_oneway) {
        o->flags &= ~OBJ_FLAG_INTERACTABLE;
    }

    int t = (o->state == SWITCH_STATE_ON ? o->trigger_off : o->trigger_on);
    sys_printf("trigger: %i\n", t);
    game_on_trigger(g, t);
    o->state = 1 - o->state;
    o->timer = SWITCH_ANIMATE_TICKS;
}