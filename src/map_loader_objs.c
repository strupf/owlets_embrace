// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void map_obj_load_misc(g_s *g, map_obj_s *mo);

void map_obj_parse(g_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "misc")) {
        map_obj_load_misc(g, o);
    } else if (str_eq_nc(o->name, "gempile")) {
        gempile_load(g, o);
    } else if (str_eq_nc(o->name, "heartpiece")) {
        heart_or_stamina_piece_load(g, o, 0);
    } else if (str_eq_nc(o->name, "staminapiece")) {
        heart_or_stamina_piece_load(g, o, 1);
    } else if (str_contains(o->name, "drillerspawn")) {
        drillerspawn_load(g, o);
    } else if (str_contains(o->name, "shortcutblock")) {
        shortcutblock_load(g, o);
    } else if (str_eq_nc(o->name, "solidlever")) {
        solidlever_load(g, o);
    } else if (str_eq_nc(o->name, "rotor")) {
        rotor_load(g, o);
    } else if (str_eq_nc(o->name, "Savepoint")) {
        savepoint_load(g, o);
    } else if (str_eq_nc(o->name, "lever_pushpull_hor")) {
        leverpushpull_load(g, o);
    } else if (str_eq_nc(o->name, "mushroom")) {
        mushroom_load(g, o);
    } else if (str_eq_nc(o->name, "door")) {
        door_load(g, o);
    } else if (str_eq_nc(o->name, "jumper")) {
        jumper_load(g, o);
    } else if (str_eq_nc(o->name, "tutorialtext")) {
        tutorialtext_load(g, o);
    } else if (str_eq_nc(o->name, "crackblock")) {
        crackblock_load(g, o);
    } else if (str_eq_nc(o->name, "trampoline")) {
        trampoline_load(g, o);
    } else if (str_eq_nc(o->name, "lookahead")) {
        lookahead_load(g, o);
    } else if (str_eq_nc(o->name, "windarea_u") ||
               str_eq_nc(o->name, "windarea_d") ||
               str_eq_nc(o->name, "windarea_l") ||
               str_eq_nc(o->name, "windarea_r")) {
        windarea_load(g, o);
    } else if (str_eq_nc(o->name, "pulleyblock_parent")) {
        pulleyblock_load_parent(g, o);
    } else if (str_eq_nc(o->name, "pulleyblock_child")) {
        pulleyblock_load_child(g, o);
    } else if (str_eq_nc(o->name, "springyblock")) {
        springyblock_load(g, o);
    } else if (str_eq_nc(o->name, "waterleaf")) {
        waterleaf_load(g, o);
    } else if (str_eq_nc(o->name, "chest")) {
        chest_load(g, o);
    } else if (str_eq_nc(o->name, "fallingblock")) {
        fallingblock_load(g, o);
    } else if (str_eq_nc(o->name, "fallingstone")) {
        fallingstonespawn_load(g, o);
    } else if (str_eq_nc(o->name, "light")) {
        light_load(g, o);
    } else if (str_eq_nc(o->name, "ditherarea")) {
        ditherarea_load(g, o);
    } else if (str_eq_nc(o->name, "stompfloor")) {
        stompable_block_load(g, o);
    } else if (str_eq_nc(o->name, "staminarestorer")) {
        staminarestorer_load(g, o);
    } else if (str_eq_nc(o->name, "flyblob")) {
        flyblob_load(g, o);
    } else if (str_eq_nc(o->name, "switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "budplant")) {
        budplant_load(g, o);
    } else if (str_eq_nc(o->name, "steamplatform")) {
        steam_platform_load(g, o);
    } else if (str_eq_nc(o->name, "upgradetree")) {
        upgradetree_load(g, o);
    } else if (str_eq_nc(o->name, "npc")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "pushblock")) {
        pushblock_load(g, o);
    } else if (str_eq_nc(o->name, "toggleblock")) {
        mushroomblock_load(g, o);
    } else if (str_eq_nc(o->name, "crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (str_eq_nc(o->name, "teleport")) {
        teleport_load(g, o);
    } else if (str_eq_nc(o->name, "stalactite")) {
        stalactite_load(g, o);
    } else if (str_eq_nc(o->name, "flyer")) {
        flyer_load(g, o);
    } else if (str_eq_nc(o->name, "movingblock")) {
        movingblock_load(g, o);
    } else if (str_eq_nc(o->name, "clockpulse")) {
        clockpulse_load(g, o);
    } else if (str_eq_nc(o->name, "triggerarea")) {
        triggerarea_load(g, o);
    } else if (str_eq_nc(o->name, "hooklever")) {
        hooklever_load(g, o);
    } else if (str_eq_nc(o->name, "cam_attractor")) {
        camattractor_load(g, o);
    } else if (str_eq_nc(o->name, "battleroom")) {
        battleroom_load(g, o);
    } else if (str_eq_nc(o->name, "cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "locked_x");
        cam->locked_y = map_obj_bool(o, "locked_y");
    } else if (str_eq_nc(o->name, "fluid")) {
        rec_i32 rfluid = {o->x, o->y, o->w, o->h};
        i32     type   = map_obj_bool(o, "lava") ? FLUID_AREA_LAVA
                                                 : FLUID_AREA_WATER;
        fluid_area_create(g, rfluid, type, map_obj_bool(o, "surface"));
    } else if (str_eq_nc(o->name, "demosave")) {
        v2_i32 ds                          = {o->x + o->w / 2, o->y + o->h};
        g->save_points[g->n_save_points++] = ds;
    } else if (str_eq_nc(o->name, "saveroom_hero") ||
               str_eq_nc(o->name, "saveroom_comp")) {
        obj_s *oo           = obj_create(g);
        oo->render_priority = RENDER_PRIO_OWL + 1;
        oo->n_sprites       = 1;
        obj_place_to_map_obj(oo, o, 0, +1);
        obj_sprite_s *spr = &oo->sprites[0];
        if (str_eq_nc(o->name, "saveroom_hero")) {
            spr->trec   = asset_texrec(TEXID_SAVEROOM, 0, 0, 64, 32);
            spr->offs.x = -32;
            spr->offs.y = -32;
        } else {
            spr->trec   = asset_texrec(TEXID_SAVEROOM, 64, 0, 64, 32);
            spr->offs.x = -32;
            spr->offs.y = -32;
        }
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
    case 60: {
        v2_i32 pfeet = {mo->x + (mo->w >> 1), mo->y + (mo->h)};
        puppet_mole_create(g, pfeet);
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