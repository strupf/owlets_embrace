// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero_hook.h"

void hero_handle_input(g_s *g, obj_s *o)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();
    i32     state  = hero_determine_state(g, o, h);

    h->action_jumpp = h->action_jump;
    h->action_jump  = inp_action(INP_A);

    if (inp_action_jp(INP_A)) {
        h->jump_btn_buffer = 6;
    }

    if (state == HERO_STATE_SWIMMING || state == HERO_STATE_LADDER) {
        h->attack_tick  = 0;
        h->b_hold_tick  = 0;
        h->holds_weapon = 0;
        hero_action_ungrapple(g, o);
        return;
    }

    if (h->holds_weapon) { // weapon equipped
        if (h->b_hold_tick) {
            if (!inp_action(INP_B)) {
                if (h->b_hold_tick < 7) {
                    h->attack_ID = 1;
                }
                hero_action_attack(g, o);
                h->b_hold_tick = 0;
            } else if (h->b_hold_tick < U8_MAX) {
                h->b_hold_tick++;
            }
        } else if (inp_action_jp(INP_B) && (h->attack_ID == 1 || (h->attack_ID == 0 && (h->attack_tick == 0 || 8 <= h->attack_tick)))) {
            h->attack_ID       = 0;
            h->attack_flipflop = 1 - h->attack_flipflop;
            h->attack_tick     = 0;
            h->b_hold_tick     = 1;
        }
    } else if (hero_has_upgrade(g, HERO_UPGRADE_HOOK)) {
        if (h->aim_mode) {
            h->hook_aim -= inp_y() * 3000;
        }

        if (inp_action(INP_B)) {
            if (h->aim_mode) { // throw hook or abort aim mode
                if (h->hook_aim_mode_tick && 15 <= ++h->hook_aim_mode_tick) {
                    h->aim_mode           = 0;
                    h->hook_aim_mode_tick = 0;
                }
            } else if (h->hook_aim_mode_tick && 15 <= ++h->hook_aim_mode_tick) { // enter aim mode
                h->hook_aim_mode_tick = 0;
                h->aim_mode           = 1;
                h->hook_aim           = 0x18000;
            }
        } else if (h->aim_mode && h->hook_aim_mode_tick) {
            hero_action_throw_grapple(g, o);
            h->aim_mode = 0;
        } else {
            h->hook_aim_mode_tick = 0;
        }

        if (inp_action_jp(INP_B)) {
            if (o->rope) {
                hero_action_ungrapple(g, o);
            } else if (h->aim_mode) {
                if (!h->hook_aim_mode_tick) {
                    h->hook_aim_mode_tick = 1;
                }
            } else if (inp_x() || inp_y()) {
                hero_action_throw_grapple(g, o);
            } else if (!h->hook_aim_mode_tick) {
                h->hook_aim_mode_tick = 1;
            }
        } else if (o->rope) {
            i32 ll              = o->rope->len_max_q4;
            i32 dt              = inp_crank_dt_q16();
            o->rope->len_max_q4 = clamp_i32(ll + (dt >> 6), 500, 6000);
        }
    }
}

bool32 hero_try_stomp(g_s *g, obj_s *o)
{
    hero_s *h           = (hero_s *)o->heap;
    rec_i32 aabb        = obj_aabb(o);
    i32     hbot        = aabb.y + aabb.h;
    bool32  stomped_obj = 0;

    for (obj_each(g, it)) {
        rec_i32 itrec = {it->pos.x, it->pos.y, it->w, 1};
        rec_i32 iaabb = obj_aabb(it);
        if (!overlap_rec(iaabb, aabb)) continue;
        if (it->ID == OBJ_ID_FLYBLOB) {
            i32 dy = hbot - iaabb.y;
            obj_move(g, o, 0, -dy);
            aabb        = obj_aabb(o);
            h->stomp    = 0;
            o->v_q8.y   = -1500;
            stomped_obj = 1;
        }
    }

    if (stomped_obj) return 1;

    rec_i32 rbot = obj_rec_bottom(o);

    obj_s *stompable_block  = NULL;
    bool32 blocked_by_other = tile_map_solid(g, rbot);
    for (obj_each(g, it)) {
        rec_i32 rit = obj_aabb(it);
        if (!overlap_rec(rit, rbot)) continue;
        if (it->ID == OBJ_ID_STOMPABLE_BLOCK) {
            stompable_block = it;
        } else {
            blocked_by_other |= it->mass;
            break;
        }
    }

    objs_cull_to_delete(g);
    if (blocked_by_other) {
        if (obj_grounded(g, o)) {
            h->stomp = 0;
        }
    } else if (stompable_block) {
        stompable_block_on_destroy(g, stompable_block);
        obj_delete(g, stompable_block);
    }

    objs_cull_to_delete(g);
    return 0;
}

void hero_post_update(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;

    if (h->stomp) {
        // bool32 stomped = hero_try_stomp(g, o);
        if (obj_grounded(g, o)) {
            h->stomp = 0;
        }
    }

    v2_i32  hcenter  = obj_pos_center(o);
    rec_i32 heroaabb = obj_aabb(o);

    // rope
    obj_s *ohook = obj_from_obj_handle(h->hook);
    if (o->rope && ohook) {
        rope_update(g, &g->rope);
        rope_verletsim(g, &g->rope);
        hero_check_rope_intact(g, o);
        if (ohook->ID == OBJ_ID_HOOK && ohook->state == 0) {
            ohook->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, ohook), 0);
        } else {
            if (obj_grounded(g, o)) {
                if (271 <= rope_stretch_q8(g, o->rope))
                    o->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, o), 0);
            } else {
                o->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, o), 0);
            }
        }
        if (!rope_intact(g, o->rope)) {
            hero_action_ungrapple(g, ohook);
        }
    }

    bool32 collected_upgrade = 0;
    i32    hero_dmg          = 0;
    v2_i32 hero_knockback    = {0};

    for (obj_each(g, it)) {
        if (!overlap_rec(heroaabb, obj_aabb(it))) continue;

        switch (it->ID) {
        case OBJ_ID_HERO_POWERUP: {
            hero_powerup_collected(g, hero_powerup_obj_ID(it));
            obj_delete(g, it);
            objs_cull_to_delete(g);
            collected_upgrade = 1;
            break;
        }
        case OBJ_ID_STAMINARESTORER: {
            staminarestorer_try_collect(g, it, o);
            break;
        }
        }

        if (it->flags & OBJ_FLAG_HURT_ON_TOUCH) {
            assert(h->stomp == 0);

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

    if (!collected_upgrade) {
        bool32 t = maptransition_try_hero_slide(g);
        if (t == 0 && inp_action_jp(INP_DU)) { // nothing happended
            obj_s *interactable = obj_from_obj_handle(h->interactable);
            if (interactable && interactable->on_interact) {
                interactable->on_interact(g, interactable);
                h->interactable = obj_handle_from_obj(NULL);
            }
        }
    }

    if (hero_dmg) {
        hero_hurt(g, o, hero_dmg);
        snd_play(SNDID_SWOOSH, 0.5f, 0.5f);
        o->v_q8 = v2_i16_from_i32(hero_knockback, 0);
        o->bumpflags &= ~OBJ_BUMP_Y; // have to clr y bump
        g->events_frame |= EVENT_HERO_DAMAGE;
    }
}

obj_s *hero_pickup_available(g_s *g, obj_s *o)
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

obj_s *hero_interactable_available(g_s *g, obj_s *o)
{
    u32          d_sq = HERO_DISTSQ_INTERACT;
    obj_s       *res  = NULL;
    const v2_i32 hc   = obj_pos_bottom_center(o);

    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_INTERACTABLE)) continue;

        it->interactable_hovered = 0;
        v2_i32 p                 = obj_pos_bottom_center(it);
        u32    d                 = v2_distancesq(hc, p);
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

void hero_start_item_pickup(g_s *g, obj_s *o)
{
}

void hero_drop_item(g_s *g, obj_s *o)
{
}