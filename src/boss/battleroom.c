// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define BATTLEROOM_MAX_PHASES 8

// returns number of enemies to spawn in battleroom's current phase
// do_spawn: actually spawn those if true; else just peek number
i32 battleroom_try_spawn_enemies(g_s *g, battleroom_s *b, bool32 do_spawn);

void battleroom_load(g_s *g, map_obj_s *mo)
{
    i32 br_saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, br_saveID)) return;

    battleroom_s *br = &g->battleroom;
    br->state        = BATTLEROOM_IDLE;
    br->saveID       = br_saveID;
    br->r.x          = mo->x;
    br->r.y          = mo->y;
    br->r.w          = mo->w;
    br->r.h          = mo->h;
}

void battleroom_on_update(g_s *g)
{
    battleroom_s *b = &g->battleroom;

    switch (b->state) {
    case BATTLEROOM_NONE: break;
    case BATTLEROOM_IDLE: {
        obj_s *ohero = 0;
        if (!hero_present_and_alive(g, &ohero)) break;
        if (!overlap_rec(obj_aabb(ohero), b->r)) break;

        game_on_trigger(g, TRIGGER_BATTLEROOM_ENTER);
        b->state = BATTLEROOM_STARTING;

        byte *obj_ptr = (byte *)g->map_objs;
        for (i32 n = 0; n < g->n_map_objs; n++) {
            map_obj_s *o = (map_obj_s *)obj_ptr;
            if (str_eq_nc(o->name, "Battleroom_Cam")) {
                g->cam.has_clamp_rec = 1;
                rec_i32 rc           = {o->x, o->y, o->w, o->h};
                g->cam.clamp_rec     = rc;
            }
            obj_ptr += o->bytes;
        }

        b->n_killed_prior = g->enemies_killed;
        mus_play_extx("M_BOSS", 263271, 418236, 0, 100, 500, 256);
        break;
    }
    case BATTLEROOM_STARTING: {
        b->timer++;
        if (b->timer < 170) break;

        b->timer = 0;
        b->state = BATTLEROOM_ACTIVE;

        // load objects with battleroom tag now
        b->n_enemies = battleroom_try_spawn_enemies(g, b, 1);
        break;
    }
    case BATTLEROOM_ACTIVE: {
        // observe number of killed enemies
        i32 killed_since = g->enemies_killed - b->n_killed_prior;
        if (b->n_enemies <= killed_since) {
            b->n_enemies      = 0;
            b->n_killed_prior = g->enemies_killed;

            while (1) {
                b->phase++;
                if (BATTLEROOM_MAX_PHASES <= b->phase) {
                    b->timer = 0;
                    b->state = BATTLEROOM_ENDING;
                    mus_play_extv(0, 0, 0, 50, 0, 0);
                    snd_play(SNDID_BOSSWIN, 1.f, 1.f);
                    save_event_register(g, b->saveID);
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
        if (b->timer == 70) {
            b->n_enemies = battleroom_try_spawn_enemies(g, b, 1);
            b->timer     = 0;
            b->state     = BATTLEROOM_ACTIVE;
        }
        break;
    }
    case BATTLEROOM_ENDING:
        b->timer++;
        if (b->timer < 200) break;

        b->timer             = 0;
        b->state             = BATTLEROOM_ENDING_2;
        g->cam.has_clamp_rec = 0;
        game_on_trigger(g, TRIGGER_BATTLEROOM_LEAVE);
        break;
    case BATTLEROOM_ENDING_2:
        b->timer++;
        if (b->timer < 50) break;

        b->timer = 0;
        b->state = BATTLEROOM_NONE;
        game_cue_area_music(g);
        break;
    }
}

i32 battleroom_try_spawn_enemies(g_s *g, battleroom_s *b, bool32 do_spawn)
{
    i32   n_enemies = 0;
    byte *obj_ptr   = (byte *)g->map_objs;

    for (i32 n = 0; n < g->n_map_objs; n++) {
        map_obj_s *o  = (map_obj_s *)obj_ptr;
        i32        br = map_obj_i32(o, "Battleroom");
        if (br && (br - 1) == b->phase) {
            n_enemies++;
            if (do_spawn) {
                map_obj_parse(g, o);
                v2_i32 poof1 = {o->x + o->w / 2, o->y + o->h / 2};
                objanim_create(g, poof1, OBJANIMID_ENEMY_EXPLODE);
            }
        }
        obj_ptr += o->bytes;
    }
    return n_enemies;
}