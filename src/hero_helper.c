// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero.h"

bool32 hero_on_valid_ladder_or_climbwall(g_s *g, obj_s *o, i32 offx, i32 offy)
{
    i32 dt_x = 0;
    i32 t    = hero_ladder_or_climbwall_snapdata(g, o, offx, offy, &dt_x);
    return (t && dt_x == 0);
}

bool32 hero_has_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    return (h->upgrades & ((u32)1 << ID));
}

void hero_add_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    h->upgrades |= (u32)1 << ID;
    pltf_log("# ADD UPGRADE: %i\n", ID);
}

void hero_rem_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    h->upgrades &= ~((u32)1 << ID);
    pltf_log("# DEL UPGRADE: %i\n", ID);
}

obj_s *hero_interactable_available(g_s *g, obj_s *o)
{
    u32    d_sq = HERO_DISTSQ_INTERACT;
    obj_s *res  = 0;
    v2_i32 hc   = obj_pos_bottom_center(o);

    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_INTERACTABLE)) continue;

        it->interactable_hovered = 0;
        v2_i32 p                 = obj_pos_bottom_center(it);
        u32    d                 = v2_i32_distancesq(hc, p);
        if (d < d_sq) {
            d_sq = d;
            res  = it;
        }
    }
    if (res) {
        res->interactable_hovered = 1;
    }

    return res;
}

void hero_check_rope_intact(g_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    hero_s *h = (hero_s *)o->heap;
    rope_s *r = o->rope;
    if (!rope_is_intact(g, r)) {
        grapplinghook_destroy(g, &g->ghook);
    }
}

i32 hero_can_grab(g_s *g, obj_s *o, i32 dirx)
{
    if (!obj_grounded(g, o)) return 0;
    if (!dirx) return 0;

    i32 x  = o->pos.x + (0 < dirx ? o->w : -1);
    i32 y1 = o->pos.y;
    i32 y2 = o->pos.y + o->h - 1;

    for (i32 y = y1; y <= y2; y++) {
        if (map_blocked_pt(g, x, y)) return 0;
    }
    return dirx;
}

void hero_walljump(g_s *g, obj_s *o, i32 dir)
{
    hero_s *h             = (hero_s *)o->heap;
    h->climbing           = 0;
    h->impact_ticks       = 0;
    h->air_block_ticks_og = WALLJUMP_MOM_TICKS;
    h->air_block_ticks    = dir * WALLJUMP_MOM_TICKS;
    o->animation          = 0;
    o->facing             = dir;
    o->v_q8.x             = dir * 700;
    hero_start_jump(g, o, HERO_JUMP_WALL);
}

i32 hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy)
{
    if (!facing) return 0;
    rec_i32 r = {o->pos.x + dx, o->pos.y + dy, o->w, o->h};
    if (!!map_blocked(g, r)) return 0;
    if (obj_grounded(g, o)) return 0;

    i32 x  = dx + (0 < facing ? o->pos.x + o->w : o->pos.x - 1);
    i32 y1 = dy + o->pos.y + HERO_CLIMB_Y1_OFFS;
    i32 y2 = dy + o->pos.y + o->h - 1 - HERO_CLIMB_Y2_OFFS;

    for (i32 y = y1; y <= y2; y++) {
        i32 r = map_climbable_pt(g, x, y);
        if (r != MAP_CLIMBABLE_SUCCESS)
            return 0;
    }

    return 1;
}

i32 hero_breath_tick(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->breath_ticks;
}

i32 hero_breath_tick_max(g_s *g)
{
    return (hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 2500 : 100);
}

i32 hero_swim_frameID(i32 animation)
{
    return ((animation >> 3) % 6);
}

i32 hero_swim_frameID_idle(i32 animation)
{
    return ((animation >> 4) & 7);
}

void hero_leave_and_clear_inair(obj_s *o)
{
    hero_s *h             = (hero_s *)o->heap;
    h->spinattack         = 0;
    h->jumpticks          = 0;
    h->walljump_tick      = 0;
    h->air_block_ticks    = 0;
    h->air_block_ticks_og = 0;
}

void hero_stamina_update_ui(obj_s *o, i32 amount)
{
    hero_s *h = (hero_s *)o->heap;
    if (!h->stamina_added) return;

    i32 d = min_i32(h->stamina_added, amount);
    h->stamina_added -= d;
    h->stamina += d;
}

i32 hero_stamina_modify(obj_s *o, i32 dt)
{
    hero_s *h    = (hero_s *)o->heap;
    i32     stap = hero_stamina_left(o);

    if (0 < dt) { // add stamina
        i32 d = min_i32(dt, hero_stamina_max(o) - stap);
        h->stamina += d;
    } else if (dt < 0) { // remove stamina
        i32 d = -dt;
        if (h->stamina) {
            i32 x = min_i32(h->stamina, d);
            h->stamina -= x;
            d -= x;
        }
        h->stamina_added = max_i32(0, h->stamina_added - d);
    }
    i32 sta = hero_stamina_left(o);
    if (sta < stap) {
        h->jump_ui_may_hide = 0;
    }
    return sta;
}

void hero_stamina_add_ui(obj_s *o, i32 dt)
{
    hero_s *h  = (hero_s *)o->heap;
    i32     ft = hero_stamina_left(o);
    h->stamina_added += min_i32(dt, hero_stamina_max(o) - ft);
    h->stamina_ui_collected_tick = 10;
}

i32 hero_stamina_left(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return (h->stamina + h->stamina_added);
}

i32 hero_stamina_ui_full(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina;
}

i32 hero_stamina_ui_added(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina_added;
}

i32 hero_stamina_max(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina_upgrades * HERO_TICKS_PER_STAMINA_UPGRADE;
}

bool32 hero_present_and_alive(g_s *g, obj_s **o)
{
    obj_s *ot = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ot) return 0;
    if (o) {
        *o = ot;
    }
    return (ot->health);
}

i32 hero_register_jumpstomped(obj_s *ohero, obj_s *o, bool32 stomped)
{
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumpstomped; n++) {
        if (obj_from_obj_handle(h->jumpstomped[n].h) == o) {
            return 1;
        }
    }
    if (h->n_jumpstomped == HERO_NUM_JUMPED_ON) return 0;

    jumpstomped_s js = {obj_handle_from_obj(o), stomped};

    h->jumpstomped[h->n_jumpstomped++] = js;
    return 2;
}

void hero_ungrab(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    obj_s  *i = obj_from_obj_handle(h->obj_grabbed);

    if (i && i->on_ungrab) {
        i->on_ungrab(g, i);
    }
    h->grabbing = 0;
}

bool32 hero_ibuf_tap(hero_s *h, i32 b, i32 frames_ago)
{
    // oldest to newest
    i32 l = min_i32(frames_ago, HERO_LEN_INPUT_BUF);
    for (i32 n = 0, st = 0; n < l; n++) {
        i32 k = (h->n_ibuf - n) & (HERO_LEN_INPUT_BUF - 1);
        i32 i = h->ibuf[k] & b;

        switch (st) {
        case 0: // check if is unpressed
            if (!i) {
                st = 1;
            }
            break;
        case 1: // check if it was pressed
            if (i) {
                st = 2;
            }
            break;
        case 2: // check if it recently *became* pressed
            if (!i) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

bool32 hero_ibuf_pressed(hero_s *h, i32 b, i32 frames_ago)
{
    // oldest to newest
    i32 l = min_i32(frames_ago, HERO_LEN_INPUT_BUF);
    for (i32 n = 0, st = 0; n < l; n++) {
        i32 k = (h->n_ibuf - n) & (HERO_LEN_INPUT_BUF - 1);
        i32 i = h->ibuf[k] & b;

        switch (st) {
        case 0: // check if pressed
            if (i) {
                st = 1;
            }
            break;
        case 1: // check if it recently *became* pressed
            if (!i) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

void ui_itemswap_start(g_s *g)
{
    ui_itemswap_s *u = &g->ui_itemswap;
    u->progress      = 1;
    u->started       = 1;
    if (u->ticks_out) {
        u->ticks_in  = 1;
        u->ticks_out = 0;
    }
}

void ui_itemswap_set_progress(g_s *g, i32 p)
{
    ui_itemswap_s *u = &g->ui_itemswap;
    if (u->started) {
        u->progress = p;
        if (u->ticks_out) {
            u->ticks_in  = 1;
            u->ticks_out = 0;
        }
    }
}

void ui_itemswap_exit(g_s *g)
{
    ui_itemswap_s *u = &g->ui_itemswap;
    if (u->started) {
        u->started = 0;
        if (u->progress < UI_ITEMSWAP_TICKS_POP_UP) {
            u->ticks_out = 0;
            u->progress  = 0;
        } else if (!u->ticks_out) {
            u->ticks_out = 1;
        }
    }
}

void ui_itemswap_update(g_s *g)
{
    ui_itemswap_s *u = &g->ui_itemswap;
    if (u->progress) {
        u->ticks_in = u8_adds(u->ticks_in, 1);
    } else {
        u->ticks_in = 0;
    }

    if (u->ticks_out) {
        u->ticks_out++;

        if (UI_ITEMSWAP_TICKS_FADE <= u->ticks_out) {
            u->ticks_out = 0;
            u->progress  = 0;
            u->ticks_in  = 0;
            u->started   = 0;
        }
    }
}
