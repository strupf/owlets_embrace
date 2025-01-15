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

#define DOOR_DETECT_PX  32 // size of trigger box in front of door
#define DOOR_MOVE_TICKS 20

typedef struct {
    obj_handle_s d[2];
    i32          type;
    i32          anim_ticks;
    i32          trigger_open;
    i32          trigger_close;
    i32          collectID;
    i32          n_collect;
    i32          keyID;
    i32          saveID;
} door_s;

static void door_set_state(g_s *g, obj_s *o, i32 state)
{
    o->state = state;
    if (o->state == DOOR_CLOSED) {
        tile_map_set_collision(g, obj_aabb(o), TILE_BLOCK, 0);
    } else {
        tile_map_set_collision(g, obj_aabb(o), TILE_EMPTY, 0);
    }
}

void door_toggle(g_s *g, obj_s *o);

void door_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");

    obj_s  *o        = obj_create(g);
    door_s *d        = (door_s *)o->mem;
    o->ID            = OBJID_DOOR;
    o->w             = mo->w;
    o->h             = mo->h;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->subID         = 0;
    d->anim_ticks    = 1;
    d->saveID        = saveID;
    d->keyID         = map_obj_i32(mo, "keyID");
    d->collectID     = map_obj_i32(mo, "collectID");
    d->n_collect     = map_obj_i32(mo, "n_collect");
    d->trigger_open  = map_obj_i32(mo, "trigger_open");
    d->trigger_close = map_obj_i32(mo, "trigger_close");
    door_set_state(g, o, DOOR_CLOSED);
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
}

void door_on_update(g_s *g, obj_s *o)
{
    door_s *d = (door_s *)o->mem;

    if (o->timer) {
        o->timer++;
        if (o->timer == DOOR_MOVE_TICKS) {
            o->timer = 0;
        }
    }

    obj_s *ohero         = 0;
    bool32 hero_detected = 0;
    if (hero_present_and_alive(g, &ohero)) {
        rec_i32 r_tog = {o->pos.x - DOOR_DETECT_PX,
                         o->pos.y,
                         o->w + 2 * DOOR_DETECT_PX,
                         o->h};
        hero_detected = overlap_rec(obj_aabb(ohero), r_tog);
    }

    switch (o->substate) {
    case DOOR_ACT_COLLECT: {

        break;
    }
    case DOOR_ACT_KEY: {
        if (hero_detected) {
            door_toggle(g, o);
            o->substate = 0;
        }
        break;
    }
    case DOOR_ACT_TRIGGER: {

        break;
    }
    }
}

void door_on_animate(g_s *g, obj_s *o)
{
    door_s *d = (door_s *)o->mem;
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
    gfx_ctx_s ctx      = gfx_ctx_display();
    texrec_s  tr       = asset_texrec(TEXID_MISCOBJ,
                                      45 * 16,
                                      5 * 16,
                                      64, 64);
    texrec_s  tr1      = asset_texrec(TEXID_MISCOBJ,
                                      49 * 16,
                                      8 * 16,
                                      64, 16);
    door_s   *d        = (door_s *)o->mem;
    i32       dy_total = o->h / 2 + 12;
    i32       dy       = 0;
    i32       dy0      = dy_total * (1 - o->state);
    i32       dy1      = dy_total * (o->state);

    if (o->timer) {
        dy = ease_out_back(dy0, dy1, o->timer, DOOR_MOVE_TICKS);
    } else {
        dy = dy1;
    }

    v2_i32 ct = v2_add(obj_pos_center(o), cam);
    v2_i32 pc = {ct.x - 32, ct.y - 32}; // pos center
    v2_i32 p1 = {pc.x, pc.y + 16 + dy};
    v2_i32 p2 = {pc.x, pc.y - 16 - dy};

    gfx_spr(ctx, tr, p1, 0, 0);
    gfx_spr(ctx, tr, p2, SPR_FLIP_XY, 0);

    v2_i32 p3 = {ct.x - 32, ct.y + o->h / 2 - 16};
    v2_i32 p4 = {ct.x - 32, ct.y - o->h / 2};
    gfx_spr(ctx, tr1, p3, 0, 0);
    gfx_spr(ctx, tr1, p4, SPR_FLIP_Y, 0);
}

void door_toggle(g_s *g, obj_s *o)
{
    o->timer = 1;
    door_set_state(g, o, 1 - o->state);
}