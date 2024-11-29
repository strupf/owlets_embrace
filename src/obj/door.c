// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    DOOR_CLOSED,
    DOOR_OPENED,
};

enum {
    DOOR_ACT_TRIGGER,
    DOOR_ACT_KEY,
    DOOR_ACT_COLLECT,
};

enum {
    DOOR_T_SLIDE_HOR,
    DOOR_T_SLIDE_VER,
};

#define DOOR_SLIDE_DT 100

typedef struct {
    obj_handle_s d[2];
    i32          type;
    i32          anim_ticks;
    i32          trigger_open;
    i32          trigger_close;
    i32          collectID;
    i32          n_collect;
    i32          keyID;
    u32          save_hash;
} door_s;

void door_toggle(g_s *g, obj_s *o);

void door_load(g_s *g, map_obj_s *mo)
{
    u32 save_hash;
    if (map_obj_saveID(mo, "saveID", &save_hash)) {
    }
    obj_s  *o        = obj_create(g);
    door_s *d        = (door_s *)o->mem;
    o->ID            = OBJ_ID_DOOR;
    o->w             = mo->w;
    o->h             = mo->h;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->subID         = 0;
    d->anim_ticks    = 1;
    o->timer         = 0x1000; // don't animate after loading
    d->save_hash     = save_hash;
    d->keyID         = map_obj_i32(mo, "keyID");
    d->collectID     = map_obj_i32(mo, "collectID");
    d->n_collect     = map_obj_i32(mo, "n_collect");
    d->trigger_open  = map_obj_i32(mo, "trigger_open");
    d->trigger_close = map_obj_i32(mo, "trigger_close");
    if (d->keyID) o->substate = DOOR_ACT_KEY;
    if (d->collectID) o->substate = DOOR_ACT_COLLECT;
    if (d->trigger_open ||
        d->trigger_close) o->substate = DOOR_ACT_TRIGGER;

    switch (d->type) {
    case DOOR_T_SLIDE_VER:
    case DOOR_T_SLIDE_HOR: {
        obj_sprite_s *s1 = &o->sprites[0];
        obj_sprite_s *s2 = &o->sprites[1];
        break;
    }
    }

    if (o->state == DOOR_CLOSED) {
        o->flags |= OBJ_FLAG_RENDER_AABB;
    } else {
        o->flags &= ~OBJ_FLAG_RENDER_AABB;
    }
}

void door_on_update(g_s *g, obj_s *o)
{
    o->timer++;

    door_s *d     = (door_s *)o->mem;
    v2_i32  oc    = obj_pos_center(o);
    obj_s  *ohero = 0;
    hero_present_and_alive(g, &ohero);
    u32 hd = U32_MAX;
    if (ohero) {
        v2_i32 hc = obj_pos_center(ohero);
        hd        = v2_distancesq(oc, hc);
    }

    switch (o->substate) {
    case DOOR_ACT_COLLECT: {

        break;
    }
    case DOOR_ACT_KEY: {

        break;
    }
    case DOOR_ACT_TRIGGER: {

        break;
    }
    }
}

void door_on_animate(g_s *g, obj_s *o)
{
    door_s *d  = (door_s *)o->mem;
    i32     t1 = min_i32(o->timer, d->anim_ticks);
    i32     t2 = d->anim_ticks;

    switch (d->type) {
    default:
    case DOOR_T_SLIDE_VER:
    case DOOR_T_SLIDE_HOR: {
        o->n_sprites     = 2;
        obj_sprite_s *s0 = &o->sprites[0];
        obj_sprite_s *s1 = &o->sprites[1];

        i32 p0      = DOOR_SLIDE_DT * (1 - o->state);
        i32 p1      = DOOR_SLIDE_DT * o->state;
        i32 p_slide = lerp_i32(p0, p1, t1, t2);
        s0->offs.y  = -p_slide;
        s1->offs.y  = +p_slide;
        break;
    }
    }
}

void door_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    if (o->substate != DOOR_ACT_TRIGGER) return;
    door_s *d = (door_s *)o->mem;

    bool32 toggle = 0;
    switch (o->state) {
    case DOOR_OPENED: {
        if (trigger == d->trigger_close) toggle = 1;
        break;
    }
    case DOOR_CLOSED: {
        if (trigger == d->trigger_open) toggle = 1;
        break;
    }
    }

    if (toggle) {
        door_toggle(g, o);
    }
}

void door_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    door_s *d = (door_s *)o->mem;
}

void door_toggle(g_s *g, obj_s *o)
{
    o->state = 1 - o->state;
    o->timer = 0;

    if (o->state == DOOR_CLOSED) {
        o->flags |= OBJ_FLAG_RENDER_AABB;
    } else {
        o->flags &= ~OBJ_FLAG_RENDER_AABB;
    }
}