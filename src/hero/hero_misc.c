// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero_hook.h"

void hero_handle_input(game_s *g, obj_s *o)
{
    hero_s *h      = &g->hero_mem;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();
    i32     state  = hero_determine_state(g, o, h);

    h->action_jumpp = h->action_jump;
    h->action_jump  = inp_action(INP_A);

    if (inp_action_jp(INP_A)) {
        h->jump_btn_buffer = 6;
    }

    if (state == HERO_STATE_LADDER) {
        h->attack_tick = 0;
        h->b_hold_tick = 0;
        hero_action_ungrapple(g, o);
        return;
    }
    if (state == HERO_STATE_SWIMMING) {
        h->attack_tick = 0;
        hero_action_ungrapple(g, o);
        return;
    }

    if (h->b_hold_tick) {
        if (!inp_action(INP_B)) {
            if (inp_x() | inp_y()) {
                hero_action_throw_grapple(g, o);
            }
            h->b_hold_tick = 0;
        } else if (h->b_hold_tick < U8_MAX) {
            h->b_hold_tick++;
        }
    } else if (inp_action_jp(INP_B)) {
        if (o->rope) {
            hero_action_ungrapple(g, o);
        } else {
            h->b_hold_tick = 1;
        }
    }
}

void hero_process_hurting_things(game_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    if (o->health == 0 || h->invincibility_ticks) return;

    v2_i32  hcenter        = obj_pos_center(o);
    rec_i32 heroaabb       = obj_aabb(o);
    i32     hero_dmg       = 0;
    v2_i32  hero_knockback = {0};

    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_HURT_ON_TOUCH)) continue;
        if (!overlap_rec(heroaabb, obj_aabb(it))) continue;

        v2_i32 ocenter   = obj_pos_center(it);
        v2_i32 dt        = v2_sub(hcenter, ocenter);
        hero_knockback.x = 1000 * sgn_i32(dt.x);
        hero_knockback.y = 1000 * sgn_i32(dt.y);
        hero_dmg         = max_i32(hero_dmg, 1);

        switch (it->ID) {
        case OBJ_ID_PROJECTILE:
            projectile_on_collision(g, it);
            break;
        }
    }

    i32 bx1 = max_i32(heroaabb.x >> 4, 0);
    i32 by1 = max_i32(heroaabb.y >> 4, 0);
    i32 bx2 = min_i32((heroaabb.x + heroaabb.w - 1) >> 4, g->tiles_x - 1);
    i32 by2 = min_i32((heroaabb.y + heroaabb.h - 1) >> 4, g->tiles_y - 1);

    for (i32 y = by1; y <= by2; y++) {
        for (i32 x = bx1; x <= bx2; x++) {
            if (g->tiles[x + y * g->tiles_x].collision != TILE_SPIKES)
                continue;
            hero_dmg = max_i32(hero_dmg, 1);
        }
    }

    if (hero_dmg) {
        hero_hurt(g, o, hero_dmg);
        snd_play(SNDID_SWOOSH, 0.5f, 0.5f);
        o->v_q8 = v2_i16_from_i32(hero_knockback, 0);
        o->bumpflags &= ~OBJ_BUMPED_Y; // have to clr y bump
        g->events_frame |= EVENT_HERO_DAMAGE;
    }
}

void hero_post_update(game_s *g, obj_s *o)
{
    if (!o) return;
    assert(0 < o->health);

    v2_i32  hcenter      = obj_pos_center(o);
    hero_s *hero         = &g->hero_mem;
    rec_i32 heroaabb     = obj_aabb(o);
    bool32  herogrounded = obj_grounded(g, o);

    // hero touching other objects
    for (obj_each(g, it)) {
        switch (o->ID) {
        case OBJ_ID_SHROOMY: {
            if (herogrounded) break;
            rec_i32 rs = obj_aabb(it);
            rec_i32 ri;
            if (!intersect_rec(heroaabb, rs, &ri)) break;
            if (0 < o->v_q8.y &&
                (heroaabb.y + heroaabb.h) < (rs.y + rs.h) &&
                o->pos_prev.y < o->pos.y) {
                o->v_q8.y = -2000;
                o->tomove.y -= ri.h;
                shroomy_bounced_on(it);
            }
            break;
        }
        }
    }

    // possibly enter new substates
    hero_check_rope_intact(g, o);
    if (o->rope) {
        obj_s *ohook = obj_from_obj_handle(hero->hook);
        if (ohook) {

            if (ohook->ID == OBJ_ID_HOOK && ohook->state == 0) {

                ohook->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, ohook), 0);
            } else {
                o->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, o), 0);
            }
            if (!rope_intact(g, o->rope)) {
                hero_action_ungrapple(g, ohook);
                return;
            }
        }
    }

    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_HOVER_TEXT)) continue;
        v2_i32 ocenter = obj_pos_center(it);
        u32    dist    = v2_distancesq(hcenter, ocenter);
#if 0
        if (dist < 3000 && it->hover_text_tick < OBJ_HOVER_TEXT_TICKS) {
            it->hover_text_tick++;
        } else if (it->hover_text_tick) {
            it->hover_text_tick--;
        }
#endif
    }

    staminarestorer_try_collect_any(g, o);
    bool32 collected_upgrade = 0;

    for (obj_each(g, it)) {
        if (it->ID != OBJ_ID_HEROUPGRADE) continue;
        if (!overlap_rec(heroaabb, obj_aabb(it))) continue;
        heroupgrade_on_collect(g, it);
        obj_delete(g, it);
        objs_cull_to_delete(g);
        collected_upgrade = 1;
        break;
    }

    if (!collected_upgrade) {
        bool32 t = maptransition_try_hero_slide(g);
        if (t == 0 && inp_action_jp(INP_DU)) { // nothing happended
            obj_s *interactable = obj_from_obj_handle(hero->interactable);
            if (interactable && interactable->on_interact) {
                interactable->on_interact(g, interactable);
                hero->interactable = obj_handle_from_obj(NULL);
            }
        }
    }
}

obj_s *hero_pickup_available(game_s *g, obj_s *o)
{
    u32          d_sq = HERO_DISTSQ_PICKUP;
    obj_s       *res  = NULL;
    const v2_i32 hc   = obj_pos_bottom_center(o);

    for (obj_each(g, it)) {
        if (it->ID != OBJ_ID_HERO_PICKUP) continue;

        v2_i32 p = obj_pos_bottom_center(it);
        u32    d = v2_distancesq(hc, p);
        if (d < d_sq) {
            d_sq = d;
            res  = it;
        }
    }
    return res;
}

obj_s *hero_interactable_available(game_s *g, obj_s *o)
{
    u32          d_sq = HERO_DISTSQ_INTERACT;
    obj_s       *res  = NULL;
    const v2_i32 hc   = obj_pos_bottom_center(o);

    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_INTERACTABLE)) continue;

        v2_i32 p = obj_pos_bottom_center(it);
        u32    d = v2_distancesq(hc, p);
        if (d < d_sq) {
            d_sq = d;
            res  = it;
        }
    }
    return res;
}

void hero_start_item_pickup(game_s *g, obj_s *o)
{
}

void hero_drop_item(game_s *g, obj_s *o)
{
}