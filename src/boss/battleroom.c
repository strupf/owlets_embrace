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
        b->state              = BATTLEROOM_STARTING;
        g->block_hero_control = 1;
        cam_lockon(&g->cam, CINIT(v2_i32){b->r.x, b->r.y}, 0);
        break;
    }
    case BATTLEROOM_STARTING: {
        b->timer++;
        if (b->timer < 100) break;

        g->block_hero_control = 0;
        b->timer              = 0;
        b->state              = BATTLEROOM_ACTIVE;

        byte *obj_ptr = (byte *)b->mem_enemies;
        for (i32 n = 0; n < b->n_enemies; n++) {
            map_obj_s *o = (map_obj_s *)obj_ptr;
            if (map_obj_bool(o, "Battleroom")) {
                map_obj_parse(g, o);
            }
            obj_ptr += o->bytes;
        }
        break;
    }
    case BATTLEROOM_ACTIVE:
        break;
    case BATTLEROOM_ENDING:
        cam_lockon_rem(&g->cam);
        break;
    }
}