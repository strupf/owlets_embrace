// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    SWITCH_ST_OFF = 0,
    SWITCH_ST_ON  = 1,
};

typedef struct {
    i32    trigger_on_enable;
    i32    trigger_on_disable;
    bool32 once;
} obj_switch_s;

void switch_on_interact(g_s *g, obj_s *o)
{
    if (!(o->flags & OBJ_FLAG_INTERACTABLE)) return;
    snd_play(SNDID_SWITCH, .5f, 1.f);
    cam_screenshake(&g->cam, 8, 5);
    obj_switch_s *os = (obj_switch_s *)o->mem;

    if (os->once) {
        o->flags &= ~(OBJ_FLAG_INTERACTABLE |
                      OBJ_FLAG_OWL_JUMPABLE |
                      OBJ_FLAG_OWL_STOMPABLE);
    }

    switch (o->state) {
    case SWITCH_ST_OFF:
        game_on_trigger(g, os->trigger_on_enable);
        break;
    case SWITCH_ST_ON:
        game_on_trigger(g, os->trigger_on_disable);
        break;
    }

    o->state             = 1 - o->state;
    o->sprites[0].trec.x = o->state * 64;
}

void switch_load(g_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJID_SWITCH;
    o->render_priority = RENDER_PRIO_HERO - 1;
    o->flags =
        OBJ_FLAG_INTERACTABLE |
        OBJ_FLAG_OWL_JUMPABLE |
        OBJ_FLAG_OWL_STOMPABLE;
    // o->on_interact       = switch_on_interact;
    o->n_sprites         = 1;
    o->sprites[0].trec   = asset_texrec(TEXID_SWITCH, 0, 0, 64, 64);
    o->sprites[0].offs.x = -25;
    o->sprites[0].offs.y = -48;
    o->w                 = 16;
    o->h                 = 16;
    o->pos.x             = mo->x - 8;
    o->pos.y             = mo->y - 32 + o->h;

    obj_switch_s *os       = (obj_switch_s *)o->mem;
    os->trigger_on_enable  = map_obj_i32(mo, "Trigger_enable");
    os->trigger_on_disable = map_obj_i32(mo, "Trigger_disable");
    os->once               = map_obj_bool(mo, "Once");
    pltf_log("once? %i\n", os->once);
}
