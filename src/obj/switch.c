// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define SWITCH_ANIMATE_TICKS 4

enum {
    SWITCH_ST_OFF = 0,
    SWITCH_ST_ON  = 1,
};

static void switch_set_sprite(obj_s *o)
{
    o->sprites[0].trec.r.x = (o->timer > 0 ? 64 : 128 * o->state);
}

obj_s *switch_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SWITCH;
    o->flags |= OBJ_FLAG_INTERACTABLE;
    o->flags |= OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
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
    snd_play_ext(SNDID_SWITCH, .5f, 1.f);
    cam_screenshake(&g->cam, 8, 5);
    if (o->switch_oneway) {
        o->flags &= ~OBJ_FLAG_INTERACTABLE;
    }

    switch (o->state) {
    case SWITCH_ST_OFF: game_on_trigger(g, o->trigger_on_1); break;
    case SWITCH_ST_ON: game_on_trigger(g, o->trigger_on_0); break;
    }

    o->state = 1 - o->state;
    o->timer = SWITCH_ANIMATE_TICKS;
}