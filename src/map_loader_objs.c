// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void map_obj_load_misc(g_s *g, map_obj_s *mo);

void map_obj_parse(g_s *g, map_obj_s *o)
{
    if (0) {
    } else if (o->hash == hash_str("misc")) {
        map_obj_load_misc(g, o);
    } else if (o->hash == hash_str("gempile")) {
        gempile_load(g, o);
    } else if (o->hash == hash_str("heartpiece")) {
        heart_or_stamina_piece_load(g, o, 0);
    } else if (o->hash == hash_str("staminapiece")) {
        heart_or_stamina_piece_load(g, o, 1);
    } else if (o->hash == hash_str("frog")) {
        frog_on_load(g, o);
    } else if (o->hash == hash_str("drillerspawn_u") ||
               o->hash == hash_str("drillerspawn_d") ||
               o->hash == hash_str("drillerspawn_l") ||
               o->hash == hash_str("drillerspawn_r")) {
        drillerspawn_load(g, o);
    } else if (o->hash == hash_str("shortcutblock")) {
        shortcutblock_load(g, o);
    } else if (o->hash == hash_str("solidlever")) {
        solidlever_load(g, o);
    } else if (o->hash == hash_str("rotor")) {
        rotor_load(g, o);
    } else if (o->hash == hash_str("savepoint")) {
        savepoint_load(g, o);
    } else if (o->hash == hash_str("vineblockade_hor") ||
               o->hash == hash_str("vineblockade_ver")) {
        vineblockade_load(g, o);
    } else if (o->hash == hash_str("lever_pushpull_hor")) {
        leverpushpull_load(g, o);
    } else if (o->hash == hash_str("mushroom")) {
        mushroom_load(g, o);
    } else if (o->hash == hash_str("door")) {
        door_load(g, o);
    } else if (o->hash == hash_str("jumper")) {
        jumper_load(g, o);
    } else if (o->hash == hash_str("tutorialtext")) {
        tutorialtext_load(g, o);
    } else if (o->hash == hash_str("crackblock")) {
        crackblock_load(g, o);
    } else if (o->hash == hash_str("trampoline")) {
        trampoline_load(g, o);
    } else if (o->hash == hash_str("lookahead")) {
        lookahead_load(g, o);
    } else if (o->hash == hash_str("bombplant")) {
        bombplant_load(g, o);
    } else if (o->hash == hash_str("windarea_u") ||
               o->hash == hash_str("windarea_d") ||
               o->hash == hash_str("windarea_l") ||
               o->hash == hash_str("windarea_r")) {
        windarea_load(g, o);
    } else if (o->hash == hash_str("pulleyblock_parent")) {
        pulleyblock_load_parent(g, o);
    } else if (o->hash == hash_str("pulleyblock_child")) {
        pulleyblock_load_child(g, o);
    } else if (o->hash == hash_str("springyblock")) {
        springyblock_load(g, o);
    } else if (o->hash == hash_str("waterleaf")) {
        waterleaf_load(g, o);
    } else if (o->hash == hash_str("chest")) {
        chest_load(g, o);
    } else if (o->hash == hash_str("fallingblock")) {
        fallingblock_load(g, o);
    } else if (o->hash == hash_str("fallingstone")) {
        fallingstonespawn_load(g, o);
    } else if (o->hash == hash_str("light")) {
        light_load(g, o);
    } else if (o->hash == hash_str("crab")) {
        crab_load(g, o);
    } else if (o->hash == hash_str("stompfloor")) {
        stompable_block_load(g, o);
    } else if (o->hash == hash_str("staminarestorer")) {
        staminarestorer_load(g, o);
    } else if (o->hash == hash_str("flyblob")) {
        flyblob_load(g, o);
    } else if (o->hash == hash_str("switch")) {
        switch_load(g, o);
    } else if (o->hash == hash_str("budplant")) {
        budplant_load(g, o);
    } else if (o->hash == hash_str("steamplatform")) {
        steam_platform_load(g, o);
    } else if (o->hash == hash_str("upgradetree")) {
        upgradetree_load(g, o);
    } else if (o->hash == hash_str("npc")) {
        npc_load(g, o);
    } else if (o->hash == hash_str("crawler")) {
        crawler_load(g, o);
    } else if (o->hash == hash_str("pushblock")) {
        pushblock_load(g, o);
    } else if (o->hash == hash_str("toggleblock")) {
        mushroomblock_load(g, o);
    } else if (o->hash == hash_str("crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (o->hash == hash_str("teleport")) {
        teleport_load(g, o);
    } else if (o->hash == hash_str("stalactite")) {
        stalactite_load(g, o);
    } else if (o->hash == hash_str("flyer")) {
        flyer_load(g, o);
    } else if (o->hash == hash_str("movingblock")) {
        movingblock_load(g, o);
    } else if (o->hash == hash_str("clockpulse")) {
        clockpulse_load(g, o);
    } else if (o->hash == hash_str("triggerarea")) {
        triggerarea_load(g, o);
    } else if (o->hash == hash_str("hooklever")) {
        hooklever_load(g, o);
    } else if (o->hash == hash_str("cam_attractor")) {
        camattractor_load(g, o);
    } else if (o->hash == hash_str("battleroom")) {
        battleroom_load(g, o);
    } else if (o->hash == hash_str("cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "locked_x");
        cam->locked_y = map_obj_bool(o, "locked_y");
    } else if (o->hash == hash_str("fluid")) {
        rec_i32 rfluid = {o->x, o->y, o->w, o->h};
        i32     type   = map_obj_bool(o, "lava") ? FLUID_AREA_LAVA
                                                 : FLUID_AREA_WATER;
        fluid_area_create(g, rfluid, type, map_obj_bool(o, "surface"));
    } else if (o->hash == hash_str("demosave")) {
        v2_i32 ds                          = {o->x + o->w / 2, o->y + o->h};
        g->save_points[g->n_save_points++] = ds;
    } else if (o->hash == hash_str("saveroom_hero") ||
               o->hash == hash_str("saveroom_comp")) {
        obj_s *oo           = obj_create(g);
        oo->render_priority = RENDER_PRIO_OWL + 1;
        oo->n_sprites       = 1;
        obj_place_to_map_obj(oo, o, 0, +1);
        obj_sprite_s *spr = &oo->sprites[0];
        if (o->hash == hash_str("saveroom_hero")) {
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