// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define SWITCH_ANIMATE_TICKS 4

enum {
    SWITCH_ST_OFF = 0,
    SWITCH_ST_ON  = 1,
};

typedef struct {
    i32    trigger_on_enable;
    i32    trigger_on_disable;
    bool32 once;
} obj_switch_s;

static void switch_set_sprite(obj_s *o)
{
    o->sprites[0].trec.r.x = o->timer ? 64 : 0;
    o->sprites[0].flip     = o->state ? SPR_FLIP_X : 0;
}

obj_s *switch_create(game_s *g)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_SWITCH;
    o->render_priority = RENDER_PRIO_HERO - 1;
    o->flags           = OBJ_FLAG_INTERACTABLE |
               OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
    o->sprites[0].trec   = asset_texrec(TEXID_SWITCH, 0, 0, 64, 64);
    o->sprites[0].offs.x = -32;
    o->sprites[0].offs.y = -64;
    switch_set_sprite(o);
    return o;
}

void switch_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = switch_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;

    obj_switch_s *os       = (obj_switch_s *)o->mem;
    os->trigger_on_enable  = map_obj_i32(mo, "Trigger_enable");
    os->trigger_on_disable = map_obj_i32(mo, "Trigger_disable");
    os->once               = map_obj_bool(mo, "Once");
}

void switch_on_animate(game_s *g, obj_s *o)
{
    if (0 < o->timer)
        o->timer--;
    switch_set_sprite(o);
}

void switch_on_interact(game_s *g, obj_s *o)
{
    if (!(o->flags & OBJ_FLAG_INTERACTABLE)) return;
    snd_play_ext(SNDID_SWITCH, .5f, 1.f);
    cam_screenshake(&g->cam, 8, 5);
    obj_switch_s *os = (obj_switch_s *)o->mem;
    if (os->once) {
        o->flags &= ~OBJ_FLAG_INTERACTABLE;
    }

    switch (o->state) {
    case SWITCH_ST_OFF:
        game_on_trigger(g, os->trigger_on_enable);
        break;
    case SWITCH_ST_ON:
        game_on_trigger(g, os->trigger_on_disable);
        break;
    }

    o->state = 1 - o->state;
    o->timer = SWITCH_ANIMATE_TICKS;
}