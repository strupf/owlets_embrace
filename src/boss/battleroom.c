// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

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
        if (b->has_camfocus) {
            g->cam.has_trg      = 1;
            g->cam.trg          = b->camfocus;
            g->cam.trg_fade_spd = 50;
        }
        b->n_killed_prior     = g->enemies_killed;
        g->block_hero_control = 1;
        break;
    }
    case BATTLEROOM_STARTING: {
        b->timer++;
        if (b->timer < 100) break;

        g->block_hero_control = 0;
        b->timer              = 0;
        b->state              = BATTLEROOM_ACTIVE;

        // load objects with battleroom tag now
        byte *obj_ptr = (byte *)b->mem_enemies;
        for (i32 n = 0; n < b->n_enemies; n++) {
            map_obj_s *o = (map_obj_s *)obj_ptr;
            if (map_obj_bool(o, "Battleroom")) {
                map_obj_parse(g, o);
                v2_i32 poof1 = {o->x + o->w / 2, o->y + o->h / 2};
                objanim_create(g, poof1, OBJANIMID_ENEMY_EXPLODE);
            }
            obj_ptr += o->bytes;
        }
        break;
    }
    case BATTLEROOM_ACTIVE: {
        // observe number of killed enemies
        i32 killed_since = g->enemies_killed - b->n_killed_prior;
        if (b->n_enemies <= killed_since) {
            b->timer = 0;
            b->state = BATTLEROOM_ENDING;
        }
        break;
    }
    case BATTLEROOM_ENDING:
        b->timer++;
        if (b->timer < 100) break;

        b->timer       = 0;
        b->state       = BATTLEROOM_NONE;
        g->cam.has_trg = 0;
        game_on_trigger(g, TRIGGER_BATTLEROOM_LEAVE);
        break;
    }
}