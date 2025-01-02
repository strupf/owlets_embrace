// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJDEF_H
#define OBJDEF_H

#include "gamedef.h"
#include "map_loader.h"

enum {
    OBJ_ID_NULL,
    OBJ_ID_HERO,
    OBJ_ID_HOOK,
    OBJ_ID_SIGN,
    OBJ_ID_SIGN_POPUP,
    OBJ_ID_DOOR,
    OBJ_ID_CRUMBLEBLOCK,
    OBJ_ID_SWITCH,
    OBJ_ID_TOGGLEBLOCK,
    OBJ_ID_CLOCKPULSE,
    OBJ_ID_SHROOMY,
    OBJ_ID_BITER,
    OBJ_ID_CRAWLER,
    OBJ_ID_CRAWLER_CATERPILLAR,
    OBJ_ID_HERO_POWERUP,
    OBJ_ID_MOVINGPLATFORM,
    OBJ_ID_NPC,
    OBJ_ID_TELEPORT,
    OBJ_ID_COLLECTIBLE,
    OBJ_ID_HERO_PICKUP,
    OBJ_ID_STALACTITE,
    OBJ_ID_STALACTITE_SPAWN,
    OBJ_ID_WALKER,
    OBJ_ID_FLYER,
    OBJ_ID_TRIGGERAREA,
    OBJ_ID_PUSHBLOCK,
    OBJ_ID_SPIKES,
    OBJ_ID_KEY,
    OBJ_ID_HOOKLEVER,
    OBJ_ID_COIN,
    OBJ_ID_ROTOR,
    OBJ_ID_SPRITEDECAL,
    OBJ_ID_FLOATER,
    OBJ_ID_WALLWORM,
    OBJ_ID_WALLWORM_PARENT,
    OBJ_ID_HOOKPLANT,
    OBJ_ID_BLOCKSWING,
    OBJ_ID_STEAM_PLATFORM,
    OBJ_ID_BUDPLANT,
    OBJ_ID_FALLINGSTONE,
    OBJ_ID_PROJECTILE,
    OBJ_ID_STAMINARESTORER,
    OBJ_ID_CRABLER,
    OBJ_ID_CRANKSWITCH,
    OBJ_ID_CAMATTRACTOR,
    OBJ_ID_FROG,
    OBJ_ID_WEAPON_PICKUP,
    OBJ_ID_FLYBLOB,
    OBJ_ID_STOMPABLE_BLOCK,
    OBJ_ID_LIGHT,
    OBJ_ID_FALLINGBLOCK,
    OBJ_ID_CHEST,
    OBJ_ID_WATERLEAF,
    OBJ_ID_WINDAREA,
    OBJ_ID_TRAMPOLINE,
    OBJ_ID_SPIDERBOSS,
    OBJ_ID_SAVEPOINT,
    OBJ_ID_BOULDER,
    OBJ_ID_TIMER,
    OBJ_ID_BOSS_GOLEM,
    OBJ_ID_BOSS_GOLEM_PLATFORM,
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
#endif