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

void hero_coins_change(g_s *g, i32 n)
{
    if (n == 0) return;
    hero_s *h  = &g->hero;
    i32     ct = h->coins + g->coins_added + n;
    if (ct < 0) return;

    if (g->coins_added == 0 || g->coins_added_ticks) {
        g->coins_added_ticks = 100;
    }
    g->coins_added += n;
}

i32 hero_coins(g_s *g)
{
    hero_s *h = &g->hero;
    i32     c = h->coins + g->coins_added;
    assert(0 <= c);
    return c;
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
    i32 y1 = dy + o->pos.y + 2;
    i32 y2 = dy + o->pos.y + o->h - 1 - 4;

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
    h->jumpticks          = 0;
    h->jump_btn_buffer    = 0;
    h->walljump_tech_tick = 0;
    h->walljump_tick      = 0;
    h->air_block_ticks    = 0;
    h->air_block_ticks_og = 0;
}

void hero_stamina_update_ui(g_s *g, obj_s *o, i32 amount)
{
    hero_s *h = (hero_s *)o->heap;
    if (!h->stamina_added) return;

    i32 d = min_i32(h->stamina_added, amount);
    h->stamina_added -= d;
    h->stamina += d;
}

i32 hero_stamina_modify(g_s *g, obj_s *o, i32 dt)
{
    hero_s *h    = (hero_s *)o->heap;
    i32     stap = hero_stamina_left(g, o);

    if (0 < dt) { // add stamina
        i32 d = min_i32(dt, hero_stamina_max(g, o) - stap);
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
    i32 sta = hero_stamina_left(g, o);
    if (sta < stap) {
        h->jump_ui_may_hide = 0;
    }
    return sta;
}

void hero_stamina_add_ui(g_s *g, obj_s *o, i32 dt)
{
    hero_s *h  = (hero_s *)o->heap;
    i32     ft = hero_stamina_left(g, o);
    h->stamina_added += min_i32(dt, hero_stamina_max(g, o) - ft);
    h->stamina_ui_collected_tick = 10;
}

i32 hero_stamina_left(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return (h->stamina + h->stamina_added);
}

i32 hero_stamina_ui_full(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina;
}

i32 hero_stamina_ui_added(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina_added;
}

i32 hero_stamina_max(g_s *g, obj_s *o)
{
    return g->hero.stamina_upgrades * HERO_TICKS_PER_STAMINA_UPGRADE;
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

i32 hero_register_jumped_or_stomped_on(obj_s *ohero, obj_s *o)
{
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumped_or_stomped_on; n++) {
        if (obj_from_obj_handle(h->jumped_or_stomped_on[n]) == o) {
            return 1;
        }
    }
    if (h->n_jumped_or_stomped_on == HERO_NUM_JUMPED_ON) return 0;
    h->jumped_or_stomped_on[h->n_jumped_or_stomped_on++] =
        obj_handle_from_obj(o);
    return 2;
}

i32 hero_register_jumped_on(obj_s *ohero, obj_s *o)
{
    hero_register_jumped_or_stomped_on(ohero, o);
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumped_on; n++) {
        if (obj_from_obj_handle(h->jumped_on[n]) == o) {
            return 1;
        }
    }
    if (h->n_jumped_on == HERO_NUM_JUMPED_ON) return 0;
    h->jumped_on[h->n_jumped_on++] = obj_handle_from_obj(o);
    return 2;
}

i32 hero_register_stomped_on(obj_s *ohero, obj_s *o)
{
    hero_register_jumped_or_stomped_on(ohero, o);
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_stomped_on; n++) {
        if (obj_from_obj_handle(h->stomped_on[n]) == o)
            return 1;
    }
    if (h->n_stomped_on == HERO_NUM_JUMPED_ON) return 0;
    h->stomped_on[h->n_stomped_on++] = obj_handle_from_obj(o);
    return 2;
}