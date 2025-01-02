// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero_hook.h"

void hero_on_game_unfreeze(g_s *g)
{
    obj_s  *o      = obj_get_hero(g);
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();
    i32     a      = -dpad_x * 8000;
    hero_action_throw_grapple(g, o, a, 5500);
    h->b_hold_tick = 0;
}

i32 hero_get_actual_state(g_s *g, obj_s *o);

void hero_handle_input(g_s *g, obj_s *o)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inp_x();
    i32     dpad_y = inp_y();
    i32     state  = hero_get_actual_state(g, o);

    if (inp_btn_jp(INP_A)) {
        h->jump_btn_buffer = 6;
    }

    if (!o->rope) {
        if (h->grabbing) {
            if (!inp_btn(INP_B) || !hero_can_grab(g, o, o->facing)) {
                h->grabbing = 0;
            } else {
                h->push_pull = dpad_x;
            }
        } else {
            i32 hgrab = hero_can_grab(g, o, dpad_x);
            if (hgrab) {
                h->push_pull = dpad_x;
                o->facing    = dpad_x;
                h->grabbing  = inp_btn_jp(INP_B);
            }
        }

        if (h->grabbing) return;
    }

    if (state == HERO_ST_WATER || state == HERO_ST_LADDER || h->crawl || h->crouched) {
        h->attack_tick  = 0;
        h->b_hold_tick  = 0;
        h->holds_weapon = 0;
        hero_action_ungrapple(g, o);
    } else {
#if 0
        if (inp_action_jp(INP_B)) {
            if (o->rope) {
                hero_action_ungrapple(g, o);
            } else {
                // h->b_hold_tick     = 1;
                h->thrown_from_alt = o->pos;
                g->hookfreeze      = 1;
            }
        }
#elif 1
        if (h->aim_mode) {
            if (inp_btn_jp(INP_B)) {
                h->b_hold_tick = 1;
            } else if (inp_btn(INP_B) && h->b_hold_tick) {
                h->b_hold_tick = u8_adds(h->b_hold_tick, 1);
            } else if (h->b_hold_tick) {
                if (20 <= h->b_hold_tick) { // abort
                    h->b_hold_tick = 0;
                } else {
                    hero_action_throw_grapple(g, o, inp_crank_q16(), 2500);
                }
                h->b_hold_tick = 0;
                h->aim_mode    = 0;
            }
        } else {
            if (h->b_hold_tick) {
                h->b_hold_tick = u8_adds(h->b_hold_tick, 1);
                if (SETTINGS.ticks_hook_hold <= h->b_hold_tick) {
                    // h->b_hold_tick = 0;
                } else if (!inp_btn(INP_B)) {
                    i32 a = -dpad_x * 8000;
                    hero_action_throw_grapple(g, o, a, 5500);
                    h->b_hold_tick = 0;
                }
            } else if (inp_btn_jp(INP_B)) {
                if (o->rope) {
                    hero_action_ungrapple(g, o);
                } else {
                    h->b_hold_tick = 1;
                }
            }
        }

#else
        if (h->aim_mode) {
            switch (SETTINGS.hook_control) {
            case SETTINGS_HOOK_CONTROL_HOLD: {
                if (inp_action_jp(INP_B)) {
                    h->b_hold_tick = 1;
                } else if (inp_action(INP_B)) {
                    if (h->b_hold_tick && h->b_hold_tick < 255) {
                        h->b_hold_tick++;
                    }
                } else if (h->b_hold_tick) {
                    if (20 <= h->b_hold_tick) {
                        hero_action_throw_grapple(g, o, inp_crank_q16(), 2500);
                    } else { // abort
                        h->b_hold_tick = 0;
                    }
                    h->aim_mode = 0;
                }
                break;
            }
            case SETTINGS_HOOK_CONTROL_TAP: {
                if (inp_action_jp(INP_B)) {
                    h->b_hold_tick = 1;
                } else if (inp_action(INP_B)) {
                    if (h->b_hold_tick && h->b_hold_tick < 255) {
                        h->b_hold_tick++;
                    }
                } else if (h->b_hold_tick) {
                    if (20 <= h->b_hold_tick) { // abort
                        h->b_hold_tick = 0;
                    } else {
                        hero_action_throw_grapple(g, o, inp_crank_q16(), 2500);
                    }
                    h->aim_mode = 0;
                }
                break;
            }
            }
        } else {
            i32 crankdt  = inp_crank_dt_q16();
            i32 crankdta = abs_i32(crankdt);
            if (100 <= crankdta) {
                h->hook_aim_crank_buildup += crankdta;
            } else if (h->hook_aim_crank_buildup) {
                h->hook_aim_crank_buildup -= 1000;
                h->hook_aim_crank_buildup = max_i32(h->hook_aim_crank_buildup, 0);
            }

            if (32768 <= h->hook_aim_crank_buildup) {
                h->aim_mode               = 1;
                h->hook_aim_crank_buildup = 0;
            } else {
                i32 a = -dpad_x * 8000;

                switch (SETTINGS.hook_control) {
                case SETTINGS_HOOK_CONTROL_HOLD: {
                    if (inp_action_jp(INP_B)) {
                        hero_action_throw_grapple(g, o, a, 3500);
                    }

                    if (!inp_action(INP_B)) {
                        hero_action_ungrapple(g, o);
                    }
                    break;
                }
                case SETTINGS_HOOK_CONTROL_TAP: {
                    if (inp_action_jp(INP_B)) {
                        if (o->rope) {
                            hero_action_ungrapple(g, o);
                        } else {
                            hero_action_throw_grapple(g, o, a, 3500);
                        }
                    }
                    break;
                }
                }
            }
        }
#endif
    }
}

void hero_post_update(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;

    trampolines_do_bounce(g);

    // jump or stomped on
    for (i32 n = 0; n < h->n_jumped_or_stomped_on; n++) {
        obj_s *i = obj_from_obj_handle(h->jumped_or_stomped_on[n]);
        if (!i) continue;

        switch (i->ID) {
        default: break;
        case OBJ_ID_SWITCH: switch_on_interact(g, i); break;
        case OBJ_ID_CHEST: chest_on_open(g, i); break;
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
        case OBJ_ID_FLYBLOB: {
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
    obj_s *ohook = obj_from_obj_handle(h->hook);
    if (o->rope && ohook) {
        rope_update(g, &g->rope);
        rope_verletsim(g, &g->rope);
        hero_check_rope_intact(g, o);
        if (ohook->ID == OBJ_ID_HOOK && ohook->state == 0) {
            ohook->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, ohook));
        } else {
            if (obj_grounded(g, o)) {
                if (271 <= rope_stretch_q8(g, o->rope))
                    o->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, o));
            } else {
                o->v_q8 = v2_i16_from_i32(obj_constrain_to_rope(g, o));
            }
        }
        if (!rope_intact(g, o->rope)) {
            hero_action_ungrapple(g, ohook);
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
        case OBJ_ID_COIN: {
            if (!overlap_rec(heroaabb, obj_aabb(it))) break;
            hero_coins_change(g, 1);
            snd_play(SNDID_COIN, 1.f, 1.f);
            obj_delete(g, it);
            break;
        }
        case OBJ_ID_HERO_POWERUP: {
            hero_powerup_collected(g, hero_powerup_obj_ID(it));
            saveID_put(g, hero_powerup_saveID(it));
            obj_delete(g, it);
            objs_cull_to_delete(g);
            collected_upgrade = 1;
            break;
        }
        case OBJ_ID_STAMINARESTORER: {
            staminarestorer_try_collect(g, it, o);
            break;
        }
        case OBJ_ID_PROJECTILE: projectile_on_collision(g, it); break;
        case OBJ_ID_FALLINGSTONE: fallingstone_burst(g, it); break;
        case OBJ_ID_STALACTITE:
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
        if (!t && inp_btn_jp(INP_DU)) { // nothing happended
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
    }
}

obj_s *hero_pickup_available(g_s *g, obj_s *o)
{
    u32    d_sq = HERO_DISTSQ_PICKUP;
    obj_s *res  = 0;
    v2_i32 hc   = obj_pos_bottom_center(o);

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

void hero_start_item_pickup(g_s *g, obj_s *o)
{
}

void hero_drop_item(g_s *g, obj_s *o)
{
}