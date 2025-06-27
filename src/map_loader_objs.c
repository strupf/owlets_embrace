// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void map_obj_parse(g_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "MISC")) {
        map_obj_load_misc(g, o);
    } else if (str_eq_nc(o->name, "Gempile")) {
        gempile_load(g, o);
    } else if (str_eq_nc(o->name, "Coin")) {
        coin_load(g, o);
    } else if (str_eq_nc(o->name, "Solidlever")) {
        solidlever_load(g, o);
    } else if (str_eq_nc(o->name, "Rotor")) {
        rotor_load(g, o);
    } else if (str_eq_nc(o->name, "Savepoint")) {
        savepoint_load(g, o);
    } else if (str_eq_nc(o->name, "Mushroom")) {
        mushroom_load(g, o);
    } else if (str_eq_nc(o->name, "Door")) {
        door_load(g, o);
    } else if (str_eq_nc(o->name, "Jumper")) {
        jumper_load(g, o);
    } else if (str_eq_nc(o->name, "Tutorialtext")) {
        tutorialtext_load(g, o);
    } else if (str_eq_nc(o->name, "Crackblock")) {
        crackblock_load(g, o);
    } else if (str_eq_nc(o->name, "Trampoline")) {
        trampoline_load(g, o);
    } else if (str_eq_nc(o->name, "Hangingblock")) {
        hangingblock_load(g, o);
    } else if (str_eq_nc(o->name, "Windarea_U") ||
               str_eq_nc(o->name, "Windarea_D") ||
               str_eq_nc(o->name, "Windarea_L") ||
               str_eq_nc(o->name, "Windarea_R")) {
        windarea_load(g, o);
    } else if (str_eq_nc(o->name, "Pulleyblock_Parent")) {
        pulleyblock_load_parent(g, o);
    } else if (str_eq_nc(o->name, "Pulleyblock_Child")) {
        pulleyblock_load_child(g, o);
    } else if (str_eq_nc(o->name, "Springyblock")) {
        springyblock_load(g, o);
    } else if (str_eq_nc(o->name, "Waterleaf")) {
        waterleaf_load(g, o);
    } else if (str_eq_nc(o->name, "Chest")) {
        chest_load(g, o);
    } else if (str_eq_nc(o->name, "Fallingblock")) {
        fallingblock_load(g, o);
    } else if (str_eq_nc(o->name, "Fallingstone")) {
        fallingstonespawn_load(g, o);
    } else if (str_eq_nc(o->name, "Light")) {
        light_load(g, o);
    } else if (str_eq_nc(o->name, "Ditherarea")) {
        ditherarea_load(g, o);
    } else if (str_eq_nc(o->name, "Stompfloor")) {
        stompable_block_load(g, o);
    } else if (str_eq_nc(o->name, "Staminarestorer")) {
        staminarestorer_load(g, o);
    } else if (str_eq_nc(o->name, "Flyblob")) {
        flyblob_load(g, o);
    } else if (str_eq_nc(o->name, "Switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "Budplant")) {
        budplant_load(g, o);
    } else if (str_eq_nc(o->name, "Steamplatform")) {
        steam_platform_load(g, o);
    } else if (str_eq_nc(o->name, "Hero_Powerup")) {
        hero_upgrade_load(g, o);
    } else if (str_eq_nc(o->name, "NPC")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "Crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "Pushblock")) {
        pushblock_load(g, o);
    } else if (str_eq_nc(o->name, "Toggleblock")) {
        toggleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Teleport")) {
        teleport_load(g, o);
    } else if (str_eq_nc(o->name, "Stalactite")) {
        stalactite_load(g, o);
    } else if (str_eq_nc(o->name, "Flyer")) {
        flyer_load(g, o);
    } else if (str_eq_nc(o->name, "Clockpulse")) {
        clockpulse_load(g, o);
    } else if (str_eq_nc(o->name, "Triggerarea")) {
        triggerarea_load(g, o);
    } else if (str_eq_nc(o->name, "Hooklever")) {
        hooklever_load(g, o);
    } else if (str_eq_nc(o->name, "Cam_Attractor")) {
        camattractor_static_load(g, o);
    } else if (str_eq_nc(o->name, "Battleroom")) {
        battleroom_load(g, o);
    } else if (str_eq_nc(o->name, "Cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "Locked_X");
        cam->locked_y = map_obj_bool(o, "Locked_Y");
        if (cam->locked_x) {
            cam->pos.x = (o->x + PLTF_DISPLAY_W / 2);
        }
        if (cam->locked_y) {
            cam->pos.y = (o->y + PLTF_DISPLAY_H / 2);
        }
    } else if (str_eq_nc(o->name, "Fluid")) {
        rec_i32 rfluid = {o->x, o->y, o->w, o->h};
        i32     type   = map_obj_bool(o, "Lava") ? FLUID_AREA_LAVA
                                                 : FLUID_AREA_WATER;
        fluid_area_create(g, rfluid, type, map_obj_bool(o, "Surface"));
    } else if (str_eq_nc(o->name, "Demosave")) {
        v2_i32 ds                          = {o->x + o->w / 2, o->y + o->h};
        g->save_points[g->n_save_points++] = ds;
    }
}

void map_obj_load_misc(g_s *g, map_obj_s *mo)
{
    i32 s1 = map_obj_i32(mo, "only_if_saveID");
    i32 s2 = map_obj_i32(mo, "only_if_not_saveID");
    if ((s1 && !save_event_exists(g, s1)) ||
        (s2 && save_event_exists(g, s2))) return;

    switch (map_obj_i32(mo, "ID")) {
    default: break;
    case 0: {
        obj_s *o    = obj_create(g);
        o->ID       = OBJID_MISC;
        o->pos.x    = mo->x;
        o->pos.y    = mo->y;
        o->w        = mo->w;
        o->h        = mo->h;
        o->state    = map_obj_i32(mo, "f1");
        o->substate = map_obj_i32(mo, "f2");
        break;
    }
    case 100: {
        obj_s *o = puppet_create(g, OBJID_PUPPET_COMPANION);
        o->pos.x = mo->x + mo->w / 2;
        o->pos.y = mo->y + mo->h / 2;
        puppet_set_anim(o, PUPPET_COMPANION_ANIMID_SIT, -1);
        break;
    }
    }
}