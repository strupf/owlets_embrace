// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJDEF_H
#define OBJDEF_H

#include "gamedef.h"
#include "map_loader.h"

enum {
    OBJID_NULL,
    OBJID_HERO,
    OBJID_HERO_COMPANION,
    OBJID_HOOK,
    OBJID_DOOR,
    OBJID_CRUMBLEBLOCK,
    OBJID_SWITCH,
    OBJID_DUMMYSOLID,
    OBJID_TOGGLEBLOCK,
    OBJID_SOLIDLEVER,
    OBJID_HOOKYEETER,
    OBJID_CLOCKPULSE,
    OBJID_MISC,
    OBJID_WATERCOL,
    OBJID_CRAWLER,
    OBJID_HERO_UPGRADE,
    OBJID_MOVINGPLATFORM,
    OBJID_NPC,
    OBJID_TELEPORT,
    OBJID_JUMPER,
    OBJID_COLLECTIBLE,
    OBJID_STALACTITE,
    OBJID_STALACTITE_SPAWN,
    OBJID_FLYER,
    OBJID_TRIGGERAREA,
    OBJID_PUSHBLOCK,
    OBJID_HOOKLEVER,
    OBJID_COIN,
    OBJID_ROTOR,
    OBJID_SPRITEDECAL,
    OBJID_STEAM_PLATFORM,
    OBJID_BUDPLANT,
    OBJID_BUDPLANT_GRENADE,
    OBJID_FALLINGSTONE,
    OBJID_FALLINGSTONE_SPAWN,
    OBJID_PROJECTILE,
    OBJID_STAMINARESTORER,
    OBJID_CRABLER,
    OBJID_CAMATTRACTOR,
    OBJID_FLYBLOB,
    OBJID_DITHERAREA,
    OBJID_STOMPABLE_BLOCK,
    OBJID_LIGHT,
    OBJID_FALLINGBLOCK,
    OBJID_CHEST,
    OBJID_WATERLEAF,
    OBJID_WINDAREA,
    OBJID_TRAMPOLINE,
    OBJID_SAVEPOINT,
    OBJID_BOULDER,
    OBJID_BOULDER_SPAWN,
    OBJID_TIMER,
    OBJID_HEALTHDROP,
    OBJID_CRACKBLOCK,
    OBJID_EXIT_BLOCKER,
    OBJID_BOSS_GOLEM,
    OBJID_BOSS_GOLEM_PLATFORM,
    //
    OBJID_PUPPET_HERO,
    OBJID_PUPPET_COMPANION,
    //
    OBJID_BOSS_PLANT_TENTACLE,
    OBJID_BOSS_PLANT_EYE,
    OBJID_BOSS_PLANT_EYE_FAKE_L,
    OBJID_BOSS_PLANT_EYE_FAKE_R,
};

enum {
    OBJ_TAG_HERO,
    OBJ_TAG_HOOK,
    OBJ_TAG_COMPANION,
    OBJ_TAG_BOSS,
    //
    NUM_OBJ_TAGS
};

enum {
    PROJECTILE_ID_DEFAULT,
    PROJECTILE_ID_ENEMY,
    PROJECTILE_ID_BUDPLANT,
    PROJECTILE_ID_STALACTITE_BREAK,
};

void      clockpulse_load(g_s *g, map_obj_s *mo);
void      crumbleblock_load(g_s *g, map_obj_s *mo);
void      crumbleblock_on_hooked(g_s *g, obj_s *o);
void      crumbleblock_break(g_s *g, obj_s *o);
void      switch_load(g_s *g, map_obj_s *mo);
void      switch_on_interact(g_s *g, obj_s *o);
void      toggleblock_load(g_s *g, map_obj_s *mo);
void      toggleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      toggleblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void      toggleblock_on_animate(g_s *g, obj_s *o);
void      crawler_load(g_s *g, map_obj_s *mo);
void      hero_upgrade_load(g_s *g, map_obj_s *mo);
void      hero_upgrade_put_orb_infront(obj_s *o);
void      hero_upgrade_move_orb_to(obj_s *o, v2_i32 pos, i32 t);
v2_i32    hero_upgrade_orb_pos(obj_s *o);
void      hero_upgrade_disable_orb(obj_s *o);
void      hero_upgrade_collect(g_s *g, obj_s *o);
void      movingplatform_load(g_s *g, map_obj_s *mo);
void      npc_load(g_s *g, map_obj_s *mo);
void      npc_on_update(g_s *g, obj_s *o);
void      npc_on_animate(g_s *g, obj_s *o);
void      npc_on_interact(g_s *g, obj_s *o);
void      crackblock_load(g_s *g, map_obj_s *mo);
obj_s    *sign_popup_create(g_s *g);
void      sign_popup_load(g_s *g, map_obj_s *mo);
void      sign_popup_on_update(g_s *g, obj_s *o);
void      sign_popup_on_draw(g_s *g, obj_s *o, v2_i32 cam);
obj_s    *sign_create(g_s *g);
void      sign_load(g_s *g, map_obj_s *mo);
void      teleport_load(g_s *g, map_obj_s *mo);
void      stalactite_load(g_s *g, map_obj_s *mo);
void      stalactite_on_update(g_s *g, obj_s *o);
void      stalactite_on_animate(g_s *g, obj_s *o);
void      stalactite_burst(g_s *g, obj_s *o);
void      flyer_load(g_s *g, map_obj_s *mo);
obj_s    *triggerarea_spawn(g_s *g, rec_i32 r, i32 tr_enter, i32 tr_leave, b32 once);
void      triggerarea_load(g_s *g, map_obj_s *mo);
void      hooklever_load(g_s *g, map_obj_s *mo);
void      hooklever_on_update(g_s *g, obj_s *o);
ratio_i32 hooklever_spring_ratio(obj_s *o);
obj_s    *spritedecal_create(g_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                             i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip);
void      spritedecal_on_update(g_s *g, obj_s *o);
void      spritedecal_on_animate(g_s *g, obj_s *o);
void      pushblock_load(g_s *g, map_obj_s *mo);
void      steam_platform_load(g_s *g, map_obj_s *mo);
void      budplant_load(g_s *g, map_obj_s *mo);
obj_s    *projectile_create(g_s *g, v2_i32 pos, v2_i32 vel, i32 subID);
void      projectile_on_collision(g_s *g, obj_s *o);
void      flyblob_load(g_s *g, map_obj_s *mo);
void      flyblob_on_hit(g_s *g, obj_s *o, hitbox_s hb);
void      staminarestorer_load(g_s *g, map_obj_s *mo);
bool32    staminarestorer_try_collect(g_s *g, obj_s *o, obj_s *ohero);
void      staminarestorer_respawn_all(g_s *g, obj_s *o);
void      camattractor_static_load(g_s *g, map_obj_s *mo);
v2_i32    camattractor_static_closest_pt(obj_s *o, v2_i32 pt);
void      ditherarea_load(g_s *g, map_obj_s *mo);
void      stompable_block_load(g_s *g, map_obj_s *mo);
void      stompable_block_break(g_s *g, obj_s *o);
void      light_load(g_s *g, map_obj_s *mo);
void      light_update(g_s *g, obj_s *o);
void      fallingblock_load(g_s *g, map_obj_s *mo);
void      chest_load(g_s *g, map_obj_s *mo);
void      chest_on_open(g_s *g, obj_s *o);
void      waterleaf_load(g_s *g, map_obj_s *mo);
void      waterleaf_on_update(g_s *g, obj_s *o);
void      waterleaf_on_animate(g_s *g, obj_s *o);
void      windarea_load(g_s *g, map_obj_s *mo);
void      windarea_on_animate(g_s *g, obj_s *o);
void      windarea_on_update(g_s *g, obj_s *o);
void      windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam);
obj_s    *coin_create(g_s *g);
void      coin_load(g_s *g, map_obj_s *mo);
void      door_load(g_s *g, map_obj_s *mo);
void      trampoline_load(g_s *g, map_obj_s *mo);
void      trampolines_do_bounce(g_s *g);
void      savepoint_load(g_s *g, map_obj_s *mo);
void      rotor_load(g_s *g, map_obj_s *mo);
void      watercol_load(g_s *g, map_obj_s *mo);
void      watercol_on_update(g_s *g, obj_s *o);
void      watercol_on_draw(g_s *g, obj_s *o, v2_i32 cam);
obj_s    *hookyeeter_create(g_s *g);
void      hookyeeter_on_update(g_s *g, obj_s *o);
void      hookyeeter_on_hook(g_s *g, obj_s *o);
void      hookyeeter_on_unhook(g_s *g, obj_s *o);
void      jumper_load(g_s *g, map_obj_s *mo);
void      solidlever_load(g_s *g, map_obj_s *mo);
obj_s    *companion_create(g_s *g);
obj_s    *companion_spawn(g_s *g, obj_s *ohero);
void      fallingstonespawn_load(g_s *g, map_obj_s *mo);
obj_s    *healthdrop_spawn(g_s *g, v2_i32 p);

#endif