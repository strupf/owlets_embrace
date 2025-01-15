// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

i32 hero_get_actual_state(g_s *g, obj_s *o);

void hero_post_update(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h = (hero_s *)o->heap;

    trampolines_do_bounce(g);

    // jump or stomped on
    for (i32 n = 0; n < h->n_jumped_or_stomped_on; n++) {
        obj_s *i = obj_from_obj_handle(h->jumped_or_stomped_on[n]);
        if (!i) continue;

        switch (i->ID) {
        default: break;
        case OBJID_SWITCH: switch_on_interact(g, i); break;
        case OBJID_CHEST: chest_on_open(g, i); break;
        }

        if (i->flags & OBJ_FLAG_ENEMY) {
            i->enemy.hurt_tick = 30;
        }
    }

    b32 stomped_on = 0;
    for (i32 n = 0; n < h->n_stomped_on; n++) {
        obj_s *i = obj_from_obj_handle(h->stomped_on[n]);
        if (!i) continue;

        stomped_on = 1;
        switch (i->ID) {
        default: break;
        case OBJID_FLYBLOB: {
            hitbox_s hb = {0};
            hb.damage   = 1;
            flyblob_on_hit(g, i, hb);
        } break;
        }
    }

    b32 jumped_on = 0;
    for (i32 n = 0; n < h->n_jumped_on; n++) {
        obj_s *i = obj_from_obj_handle(h->jumped_on[n]);
        if (!i) continue;

        jumped_on = 1;
        switch (i->ID) {
        default: break;
        }
    }

    h->n_jumped_or_stomped_on = 0;
    h->n_stomped_on           = 0;
    h->n_jumped_on            = 0;

    if (stomped_on) {
        o->v_q8.y = min_i32(o->v_q8.y, -1000);
        o->bumpflags &= ~OBJ_BUMP_Y_POS;
        h->stomp = 0;
    }

    if (jumped_on) {
        o->v_q8.y = min_i32(o->v_q8.y, -1000);
        o->bumpflags &= ~OBJ_BUMP_Y_POS;
    }

    if (h->stomp) {
        if (obj_grounded(g, o)) {
            h->stomp               = 0;
            h->stomp_landing_ticks = 1;
        }
    }

    v2_i32  hcenter  = obj_pos_center(o);
    rec_i32 heroaabb = obj_aabb(o);

    // rope
    rope_s          *r  = o->rope;
    grapplinghook_s *gh = &g->ghook;
    if (r && grapplinghook_rope_intact(g, gh)) {
        grapplinghook_update(g, gh);
        rope_update(g, r);
        grapplinghook_animate(g, gh);

        if (gh->state) {
            if (gh->state == GRAPPLINGHOOK_FLYING) {
                v2_i32 v_hook = rope_recalc_v(g, r, gh->rn,
                                              v2_i32_from_i16(gh->p_q8),
                                              v2_i32_from_i16(gh->v_q8));
                gh->v_q8      = v2_i16_from_i32(v_hook);
            } else {
                bool32 calc_v = 0;
                calc_v |= obj_grounded(g, o) && 271 <= rope_stretch_q8(g, r);
                calc_v |= !obj_grounded(g, o);
                if (calc_v) {
                    v2_i32 v_hero = rope_recalc_v(g, r, o->ropenode,
                                                  v2_i32_from_i16(o->subpos_q8),
                                                  v2_i32_from_i16(o->v_q8));
                    o->v_q8       = v2_i16_from_i32(v_hero);
                }
            }
        }
    }

    bool32 collected_upgrade = 0;
    i32    hero_dmg          = 0;
    v2_i16 hero_knockback    = {0};

    for (obj_each(g, it)) {
        if (!overlap_rec(heroaabb, obj_aabb(it))) continue;

        if (it->flags & OBJ_FLAG_HURT_ON_TOUCH) {
            v2_i32 ocenter   = obj_pos_center(it);
            v2_i32 dt        = v2_sub(hcenter, ocenter);
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
        }

        switch (it->ID) {
        case OBJID_COIN: {
            if (!overlap_rec(heroaabb, obj_aabb(it))) break;
            hero_coins_change(g, 1);
            snd_play(SNDID_COIN, 1.f, 1.f);
            obj_delete(g, it);
            break;
        }
        case OBJID_HERO_POWERUP: {
            hero_powerup_collected(g, hero_powerup_obj_ID(it));
            saveID_put(g, hero_powerup_saveID(it));
            obj_delete(g, it);
            objs_cull_to_delete(g);
            collected_upgrade = 1;
            break;
        }
        case OBJID_STAMINARESTORER: {
            staminarestorer_try_collect(g, it, o);
            break;
        }
        case OBJID_PROJECTILE: projectile_on_collision(g, it); break;
        case OBJID_FALLINGSTONE: fallingstone_burst(g, it); break;
        case OBJID_STALACTITE:
            if (it->flags & OBJ_FLAG_HURT_ON_TOUCH) {
                stalactite_burst(g, it);
            }
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
            v2_i32 ptilec    = {(x << 4) + 8, (y << 4) + 8};
            v2_i32 dt        = v2_sub(hcenter, ptilec);
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
        }
    }

    if (!collected_upgrade) {
        bool32 t = maptransition_try_hero_slide(g);
        if (!t && inps_btn_jp(inp, INP_DU)) { // nothing happended
            obj_s *interactable = obj_from_obj_handle(h->interactable);
            obj_interact(g, interactable, o);
            h->interactable = obj_handle_from_obj(0);
        }
    }

    if (hero_dmg && !h->invincibility_ticks) {
        hero_hurt(g, o, hero_dmg);
        snd_play(SNDID_SWOOSH, 0.5f, 0.5f);
        o->v_q8 = hero_knockback;
        o->bumpflags &= ~(OBJ_BUMP_Y | OBJ_BUMP_X);
        g->events_frame |= EVENT_HERO_DAMAGE;
        if (o->health == 0) {
            g->events_frame |= EVENT_HERO_DEATH;
        }
    }
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