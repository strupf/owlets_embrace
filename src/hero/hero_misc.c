// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero_hook.h"

void hero_process_hurting_things(game_s *g, obj_s *o)
{
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
            v2_i32 ocenter   = {(x << 4) + 8, (y << 4) + 8};
            v2_i32 dt        = v2_sub(hcenter, ocenter);
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
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
    if (!o->invincible_tick) {
        hero_process_hurting_things(g, o);
    }

    // possibly enter new substates
    hero_check_rope_intact(g, o);
    if (o->rope) {
        obj_s *ohook = obj_from_obj_handle(hero->hook);
        if (ohook) {

            if (ohook->ID == OBJ_ID_HOOK && ohook->state == 0) {

                ohook->vel_q8 = obj_constrain_to_rope(g, ohook);
            } else {
                o->vel_q8 = obj_constrain_to_rope(g, o);
            }
            if (!rope_intact(g, o->rope)) {
                hero_unhook(g, ohook);
                return;
            }
        }
    }
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