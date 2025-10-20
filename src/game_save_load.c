// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void game_load_savefile(g_s *g)
{
    save_file_s *s = g->save;
    if (s->tick == 0) { // new game

    } else { // continue
    }

    obj_delete(g, obj_get_owl(g));
    mclr(&g->owl, sizeof(owl_s));
    objs_cull_to_delete(g);

    obj_s *o = owl_create(g);
    owl_s *h = (owl_s *)o->heap;
    {
        mcpy(g->saveIDs, s->save, sizeof(g->saveIDs));
        mcpy(h->name, s->name, sizeof(h->name));
        mcpy(g->minimap.pins, s->pins, sizeof(g->minimap.pins));
        for (i32 n = 0; n < ARRLEN(s->map_visited); n++) {
            u32 v                            = s->map_visited[n];
            g->minimap.visited[(n << 1) + 0] = v;
            g->minimap.visited[(n << 1) + 1] = v;
        }
        g->tick             = s->tick;
        h->upgrades         = s->upgrades;
        g->minimap.n_pins   = s->n_map_pins;
        g->coins.n          = s->coins;
        o->health_max       = s->health_max;
        o->health           = o->health_max;
        h->stamina_upgrades = s->stamina;
        h->stamina_max      = h->stamina_upgrades * OWL_STAMINA_PER_CONTAINER;
        h->stamina          = h->stamina_max;
        mcpy(g->map_name, s->map_name, sizeof(s->map_name));
    }

    game_load_map(g, s->map_name);

    obj_s *ocomp = 0;
    if (saveID_has(g, SAVEID_COMPANION_FOUND)) {
        ocomp = companion_create(g);
    }

    bool32 saveroom_pos_hero = 0;
    bool32 saveroom_pos_comp = 0;
    for (map_obj_each(g, i)) {
        if (i->hash == hash_str("saveroom_hero")) {
            obj_place_to_map_obj(o, i, 0, +1);
            saveroom_pos_hero = 1;
        }
        if (i->hash == hash_str("saveroom_comp") && ocomp) {
            obj_place_to_map_obj(ocomp, i, 0, +1);
            ocomp->pos.y -= 4;
            saveroom_pos_comp = 1;
        }
    }

    if (!saveroom_pos_hero) {
        o->pos.x = s->hero_pos.x - o->w / 2;
        o->pos.y = s->hero_pos.y - o->h;
    }
    if (!saveroom_pos_comp && ocomp) {
        ocomp->pos.x  = o->pos.x + 0;
        ocomp->pos.y  = o->pos.y - 30;
        ocomp->facing = o->facing;
    }

    cam_hard_set_positon(g, &g->cam);
    sfx_block_new(1); // disable sounds (foot steps etc.)
    objs_animate(g);
    sfx_block_new(0);
    if (saveroom_pos_hero) {
        cs_on_load_enter(g);
    }
    pltf_timestep_reset();
}

void game_update_savefile(g_s *g)
{
    save_file_s *s = g->save;
    owl_s       *h = &g->owl;
    {
        mcpy(s->save, g->saveIDs, sizeof(s->save));
        mcpy(s->name, h->name, sizeof(s->name));
        mcpy(s->pins, g->minimap.pins, sizeof(s->pins));
        for (i32 n = 0; n < ARRLEN(s->map_visited); n++) {
            s->map_visited[n] = g->minimap.visited[(n << 1) + 0] |
                                g->minimap.visited[(n << 1) + 1];
        }
        s->tick       = g->tick;
        s->upgrades   = h->upgrades;
        s->n_map_pins = g->minimap.n_pins;
        s->coins      = coins_total(g);
        s->health_max = h->health_max;
    }
    pltf_timestep_reset();
}

bool32 game_save_savefile(g_s *g, v2_i32 pos)
{
    save_file_s *s = g->save;
    owl_s       *h = &g->owl;
    {
        game_update_savefile(g);
        str_cpy(s->map_name, g->map_name);
        s->hero_pos.x = pos.x;
        s->hero_pos.y = pos.y;
    }

    err32 err = 0;
    if (!(g->flags & GAME_FLAG_SPEEDRUN)) {
        err = save_file_w_slot(s, g->save_slot);
    }
    pltf_timestep_reset();
    return (err == 0);
}