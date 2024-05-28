// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

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
            if (0 < o->vel_q8.y &&
                (heroaabb.y + heroaabb.h) < (rs.y + rs.h) &&
                o->posprev.y < o->pos.y) {
                o->vel_q8.y = -2000;
                o->tomove.y -= ri.h;
                shroomy_bounced_on(it);
            }
            break;
        }
        }
    }

    // touched hurting things?
    if (o->invincible_tick <= 0) {
        i32    hero_dmg       = 0;
        v2_i32 hero_knockback = {0};

        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_HURT_ON_TOUCH)) continue;
            if (!overlap_rec(heroaabb, obj_aabb(it))) continue;
            if (hero->thrusting) {
                hero->thrusting = 0;
                o->vel_q8.y     = -2000;
                continue;
            }
            continue; // disable
            v2_i32 ocenter   = obj_pos_center(it);
            v2_i32 dt        = v2_sub(hcenter, ocenter);
            hero_knockback.x = sgn_i(dt.x) * 1000;
            hero_knockback.y = -1000;
            hero_dmg         = max_i(hero_dmg, 1);

            switch (it->ID) {
            case OBJ_ID_CHARGER: {
                int pushs        = sgn_i(hcenter.x - ocenter.x);
                hero_knockback.x = pushs * 2000;
                break;
            }
            }
        }

        if (hero_dmg) {
            hero_hurt(g, o, hero_dmg);
            snd_play_ext(SNDID_SWOOSH, 0.5f, 0.5f);
            o->vel_q8 = hero_knockback;
            o->bumpflags &= ~OBJ_BUMPED_Y; // have to clr y bump
            g->events_frame |= EVENT_HERO_DAMAGE;
        }
    }

    // possibly enter new substates
    hero_check_rope_intact(g, o);
    if (o->health <= 0) {
        gameover_start(g);
    } else {
        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_HOVER_TEXT)) continue;
            v2_i32 ocenter = obj_pos_center(it);
            u32    dist    = v2_distancesq(hcenter, ocenter);

            if (dist < 3000 && it->hover_text_tick < OBJ_HOVER_TEXT_TICKS) {
                it->hover_text_tick++;
            } else if (it->hover_text_tick) {
                it->hover_text_tick--;
            }
        }

        i32 roomtilex = hcenter.x / PLTF_DISPLAY_W;
        i32 roomtiley = hcenter.y / PLTF_DISPLAY_H;
        hero_set_visited_tile(g, g->map_worldroom, roomtilex, roomtiley);

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
}