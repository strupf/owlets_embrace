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
    OBJID_HOOK,
    OBJID_SIGN,
    OBJID_SIGN_POPUP,
    OBJID_DOOR,
    OBJID_CRUMBLEBLOCK,
    OBJID_SWITCH,
    OBJID_DUMMYSOLID,
    OBJID_TOGGLEBLOCK,
    OBJID_CLOCKPULSE,
    OBJID_SHROOMY,
    OBJID_WATERCOL,
    OBJID_BITER,
    OBJID_CRAWLER,
    OBJID_CRAWLER_CATERPILLAR,
    OBJID_HERO_POWERUP,
    OBJID_MOVINGPLATFORM,
    OBJID_NPC,
    OBJID_TELEPORT,
    OBJID_COLLECTIBLE,
    OBJID_HERO_PICKUP,
    OBJID_STALACTITE,
    OBJID_STALACTITE_SPAWN,
    OBJID_WALKER,
    OBJID_FLYER,
    OBJID_TRIGGERAREA,
    OBJID_PUSHBLOCK,
    OBJID_SPIKES,
    OBJID_KEY,
    OBJID_HOOKLEVER,
    OBJID_COIN,
    OBJID_ROTOR,
    OBJID_SPRITEDECAL,
    OBJID_FLOATER,
    OBJID_WALLWORM,
    OBJID_WALLWORM_PARENT,
    OBJID_HOOKPLANT,
    OBJID_BLOCKSWING,
    OBJID_STEAM_PLATFORM,
    OBJID_BUDPLANT,
    OBJID_FALLINGSTONE,
    OBJID_PROJECTILE,
    OBJID_STAMINARESTORER,
    OBJID_CRABLER,
    OBJID_CRANKSWITCH,
    OBJID_CAMATTRACTOR,
    OBJID_FROG,
    OBJID_WEAPON_PICKUP,
    OBJID_FLYBLOB,
    OBJID_STOMPABLE_BLOCK,
    OBJID_LIGHT,
    OBJID_FALLINGBLOCK,
    OBJID_CHEST,
    OBJID_WATERLEAF,
    OBJID_WINDAREA,
    OBJID_TRAMPOLINE,
    OBJID_SPIDERBOSS,
    OBJID_SAVEPOINT,
    OBJID_BOULDER,
    OBJID_TIMER,
    OBJID_BOSS_GOLEM,
    OBJID_BOSS_GOLEM_PLATFORM,
};

enum {
    OBJ_TAG_HERO,
    OBJ_TAG_HOOK,
    OBJ_TAG_CARRIED,
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

enum {
    PICKUP_ID_NONE,
    PICKUP_ID_BOMB,
    PICKUP_ID_WEAPON,
};

void      floater_load(g_s *g, map_obj_s *mo);
void      clockpulse_load(g_s *g, map_obj_s *mo);
void      clockpulse_on_update(g_s *g, obj_s *o);
void      clockpulse_on_trigger(g_s *g, obj_s *o, i32 trigger);
void      crumbleblock_load(g_s *g, map_obj_s *mo);
void      crumbleblock_on_hooked(obj_s *o);
void      crumbleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      crumbleblock_on_update(g_s *g, obj_s *o);
void      crumbleblock_on_animate(g_s *g, obj_s *o);
void      switch_load(g_s *g, map_obj_s *mo);
void      switch_on_interact(g_s *g, obj_s *o);
void      toggleblock_load(g_s *g, map_obj_s *mo);
void      toggleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      toggleblock_on_trigger(g_s *g, obj_s *o, i32 trigger);
void      toggleblock_on_animate(g_s *g, obj_s *o);
void      shroomy_load(g_s *g, map_obj_s *mo);
void      shroomy_bounced_on(obj_s *o);
void      crawler_load(g_s *g, map_obj_s *mo);
void      crawler_on_update(g_s *g, obj_s *o);
void      crawler_on_animate(g_s *g, obj_s *o);
void      crawler_caterpillar_load(g_s *g, map_obj_s *mo);
void      crawler_on_weapon_hit(g_s *g, obj_s *o, hitbox_s hb);
void      hero_powerup_obj_load(g_s *g, map_obj_s *mo);
void      hero_powerup_obj_on_update(g_s *g, obj_s *o);
void      hero_powerup_obj_on_draw(g_s *g, obj_s *o, v2_i32 cam);
i32       hero_powerup_obj_ID(obj_s *o);
i32       hero_powerup_saveID(obj_s *o);
void      movingplatform_load(g_s *g, map_obj_s *mo);
void      npc_load(g_s *g, map_obj_s *mo);
void      npc_on_update(g_s *g, obj_s *o);
void      npc_on_animate(g_s *g, obj_s *o);
void      npc_on_interact(g_s *g, obj_s *o);
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
void      walker_load(g_s *g, map_obj_s *mo);
void      walker_on_update(g_s *g, obj_s *o);
void      walker_on_animate(g_s *g, obj_s *o);
void      flyer_load(g_s *g, map_obj_s *mo);
obj_s    *triggerarea_spawn(g_s *g, rec_i32 r, i32 tr_enter, i32 tr_leave, b32 once);
void      triggerarea_load(g_s *g, map_obj_s *mo);
void      triggerarea_on_update(g_s *g, obj_s *o);
void      spikes_load(g_s *g, map_obj_s *mo);
void      hooklever_load(g_s *g, map_obj_s *mo);
void      hooklever_on_update(g_s *g, obj_s *o);
ratio_i32 hooklever_spring_ratio(obj_s *o);
obj_s    *spritedecal_create(g_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                             i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip);
void      spritedecal_on_update(g_s *g, obj_s *o);
void      spritedecal_on_animate(g_s *g, obj_s *o);
void      wallworm_load(g_s *g, map_obj_s *mo);
void      hookplant_load(g_s *g, map_obj_s *mo);
void      hookplant_on_hook(obj_s *o);
void      blockswing_load(g_s *g, map_obj_s *mo);
void      pushblock_load(g_s *g, map_obj_s *mo);
void      pushblock_on_update(g_s *g, obj_s *o);
void      pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      steam_platform_load(g_s *g, map_obj_s *mo);
void      budplant_load(g_s *g, map_obj_s *mo);
void      budplant_on_update(g_s *g, obj_s *o);
void      budplant_on_animate(g_s *g, obj_s *o);
obj_s    *projectile_create(g_s *g, v2_i32 pos, v2_i32 vel, i32 subID);
void      projectile_on_update(g_s *g, obj_s *o);
void      projectile_on_animate(g_s *g, obj_s *o);
void      projectile_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      projectile_on_collision(g_s *g, obj_s *o);
void      flyblob_load(g_s *g, map_obj_s *mo);
void      flyblob_on_update(g_s *g, obj_s *o);
void      flyblob_on_animate(g_s *g, obj_s *o);
void      flyblob_on_hit(g_s *g, obj_s *o, hitbox_s hb);
void      staminarestorer_load(g_s *g, map_obj_s *mo);
void      staminarestorer_on_animate(g_s *g, obj_s *o);
bool32    staminarestorer_try_collect(g_s *g, obj_s *o, obj_s *ohero);
void      staminarestorer_respawn_all(g_s *g, obj_s *o);
void      camattractor_static_load(g_s *g, map_obj_s *mo);
v2_i32    camattractor_static_closest_pt(obj_s *o, v2_i32 pt);
void      frog_load(g_s *g, map_obj_s *mo);
void      frog_on_update(g_s *g, obj_s *o);
void      frog_on_animate(g_s *g, obj_s *o);
obj_s    *hero_pickup_create(g_s *g, v2_i32 pos, i32 pickupID);
void      hero_pickup_load(g_s *g, map_obj_s *mo);
void      stompable_block_load(g_s *g, map_obj_s *mo);
void      stompable_block_on_update(g_s *g, obj_s *o);
void      stompable_block_on_animate(g_s *g, obj_s *o);
void      stompable_block_on_destroy(g_s *g, obj_s *o);
void      light_load(g_s *g, map_obj_s *mo);
void      light_update(g_s *g, obj_s *o);
void      fallingblock_load(g_s *g, map_obj_s *mo);
void      fallingblock_on_update(g_s *g, obj_s *o);
void      fallingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      chest_load(g_s *g, map_obj_s *mo);
void      chest_on_update(g_s *g, obj_s *o);
void      chest_on_animate(g_s *g, obj_s *o);
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
void      coin_on_animate(g_s *g, obj_s *o);
void      coin_on_update(g_s *g, obj_s *o);
void      door_load(g_s *g, map_obj_s *mo);
void      door_on_update(g_s *g, obj_s *o);
void      door_on_animate(g_s *g, obj_s *o);
void      door_on_trigger(g_s *g, obj_s *o, i32 trigger);
void      door_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      trampoline_load(g_s *g, map_obj_s *mo);
void      trampoline_on_update(g_s *g, obj_s *o);
void      trampoline_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      trampolines_do_bounce(g_s *g);
void      spiderboss_load(g_s *g, map_obj_s *mo);
void      spiderboss_on_update(g_s *g, obj_s *o);
void      spiderboss_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      savepoint_load(g_s *g, map_obj_s *mo);
void      savepoint_on_update(g_s *g, obj_s *o);
void      savepoint_on_interact(g_s *g, obj_s *o);
void      savepoint_on_draw(g_s *g, obj_s *o, v2_i32 cam);
obj_s    *boulder_spawn(g_s *g);
void      boulder_on_update(g_s *g, obj_s *o);
void      boulder_on_animate(g_s *g, obj_s *o);
obj_s    *fallingstone_spawn(g_s *g);
void      fallingstone_burst(g_s *g, obj_s *o);
void      fallingstone_on_update(g_s *g, obj_s *o);
void      fallingstone_on_animate(g_s *g, obj_s *o);
void      rotor_load(g_s *g, map_obj_s *mo);
void      rotor_on_update(g_s *g, obj_s *o);
void      rotor_on_animate(g_s *g, obj_s *o);
obj_s    *biter_create(g_s *g, v2_i32 p);
void      biter_load(g_s *g, map_obj_s *mo);
void      biter_on_update(g_s *g, obj_s *o);
void      biter_on_animate(g_s *g, obj_s *o);
void      biter_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      watercol_load(g_s *g, map_obj_s *mo);
void      watercol_on_update(g_s *g, obj_s *o);
void      watercol_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void      dummysolid_load(g_s *g, map_obj_s *mo);
void      dummysolid_on_update(g_s *g, obj_s *o);
#endif