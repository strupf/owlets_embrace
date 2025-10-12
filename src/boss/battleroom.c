// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define BATTLEROOM_MAX_WAVES 8

// returns number of enemies to spawn in battleroom's current phase
// do_spawn: actually spawn those if true; else just peek number
i32 battleroom_try_spawn_enemies(g_s *g, battleroom_s *b, bool32 do_spawn);

void battleroom_load(g_s *g, map_obj_s *mo)
{
    i32 saveID0 = map_obj_i32(mo, "saveID");
    if (saveID0 && saveID_has(g, saveID0))
        return;
    i32 saveID1 = map_obj_i32(mo, "only_if_not_saveID");
    if (saveID1 && saveID_has(g, saveID1))
        return;
    i32 saveID2 = map_obj_i32(mo, "only_if_saveID");
    if (saveID2 && !saveID_has(g, saveID2))
        return;

    battleroom_s *br     = &g->battleroom;
    br->state            = BATTLEROOM_IDLE;
    br->saveID           = saveID0;
    br->r.x              = mo->x;
    br->r.y              = mo->y;
    br->r.w              = mo->w;
    br->r.h              = mo->h;
    br->ID               = map_obj_i32(mo, "ID");
    br->cam_rec_ID       = map_obj_i32(mo, "cam_rec_ID");
    br->trigger_activate = map_obj_i32(mo, "trigger_activate");
    br->trigger_finish   = map_obj_i32(mo, "trigger_finish");
}

void battleroom_on_update(g_s *g)
{
    battleroom_s *b = &g->battleroom;

    if (b->ticks_to_spawn) {
        b->ticks_to_spawn--;

        if (!b->ticks_to_spawn) {

            for (i32 n = 0; n < b->n_map_obj_to_spawn; n++) {
                map_obj_parse(g, b->map_obj_to_spawn[n]);
            }
            b->n_map_obj_to_spawn = 0;
        }
    }

    switch (b->state) {
    case BATTLEROOM_NONE: break;
    case BATTLEROOM_IDLE: {
        obj_s *owl = owl_if_present_and_alive(g);
        if (!owl || !overlap_rec(obj_aabb(owl), b->r)) break;

        game_on_trigger(g, TRIGGER_BATTLEROOM_ENTER);
        game_on_trigger(g, b->trigger_activate);
        b->state = BATTLEROOM_STARTING;

        for (map_obj_each(g, o)) {
            if (o->hash == hash_str("cam_rec") && map_obj_i32(o, "ID") == b->cam_rec_ID) {
                rec_i32 rc = {o->x, o->y, o->w, o->h};
                cam_clamp_rec_set(g, rc);
                break;
            }
        }

        b->n_killed_prior = g->enemies_killed;
        // mus_play_extx("M_BOSS", 263271, 418236, 0, 100, 500, 256);
        break;
    }
    case BATTLEROOM_STARTING: {
        b->timer++;
        if (b->timer < 120) break;

        b->timer = 0;
        b->state = BATTLEROOM_ACTIVE;

        // load objects with battleroom tag now
        // b->n_enemies = battleroom_try_spawn_enemies(g, b, 1);
        break;
    }
    case BATTLEROOM_ACTIVE: {
        // observe number of killed enemies
        u32 killed_since = g->enemies_killed - b->n_killed_prior;
        if (b->n_enemies <= killed_since) {
            b->n_enemies      = 0;
            b->n_killed_prior = g->enemies_killed;

            while (1) {
                b->phase++;
                if (BATTLEROOM_MAX_WAVES <= b->phase) {
                    b->timer = 0;
                    b->state = BATTLEROOM_ENDING;
                    // mus_play_extv(0, 0, 0, 50, 0, 0);
                    sfx_cuef(SFXID_BOSSWIN, 1.f, 1.f);
                    saveID_put(g, b->saveID);
                    break;
                }
                if (battleroom_try_spawn_enemies(g, b, 0)) {
                    b->state = BATTLEROOM_NEXT_PHASE;
                    b->timer = 0;
                    break;
                }
            }
        }
        break;
    }
    case BATTLEROOM_NEXT_PHASE: {
        b->timer++;
        if (b->timer == 40) {
            b->n_enemies = battleroom_try_spawn_enemies(g, b, 1);
            b->timer     = 0;
            b->state     = BATTLEROOM_ACTIVE;
        }
        break;
    }
    case BATTLEROOM_ENDING:
        b->timer++;
        if (b->timer < 200) break;

        b->timer = 0;
        b->state = BATTLEROOM_ENDING_2;
        cam_clamp_rec_unset(g);
        game_on_trigger(g, TRIGGER_BATTLEROOM_LEAVE);
        game_on_trigger(g, b->trigger_finish);
        break;
    case BATTLEROOM_ENDING_2:
        b->timer++;
        if (b->timer < 50) break;

        game_cue_area_music(g);
        mclr(b, sizeof(battleroom_s));
        break;
    }
}

i32 battleroom_try_spawn_enemies(g_s *g, battleroom_s *b, bool32 do_spawn)
{
    i32 n_enemies         = 0;
    b->ticks_to_spawn     = 70; // delay actual spawning to align with explosion animation
    b->n_map_obj_to_spawn = 0;

    for (map_obj_each(g, o)) {
        i32 brID   = map_obj_i32(o, "battleroom");
        i32 brwave = map_obj_i32(o, "battleroom_wave");

        if (brID == b->ID && brwave == b->phase - 1) {
            n_enemies++;

            if (do_spawn) {
                b->map_obj_to_spawn[b->n_map_obj_to_spawn++] = o;
                v2_i32 poofpos                               = {o->x + o->w / 2, o->y + o->h / 2 - 4};
                animobj_create(g, poofpos, ANIMOBJ_ENEMY_SPAWN);
            }
        }
    }
    return n_enemies;
}