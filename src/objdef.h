// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
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
    OBJ_ID_SOLID,
    OBJ_ID_DOOR_SWING,
    OBJ_ID_CRUMBLEBLOCK,
    OBJ_ID_SWITCH,
    OBJ_ID_TOGGLEBLOCK,
    OBJ_ID_CLOCKPULSE,
    OBJ_ID_SHROOMY,
    OBJ_ID_CRAWLER,
    OBJ_ID_CRAWLER_CATERPILLAR,
    OBJ_ID_CARRIER,
    OBJ_ID_HERO_POWERUP,
    OBJ_ID_MOVINGPLATFORM,
    OBJ_ID_NPC,
    OBJ_ID_CHARGER,
    OBJ_ID_TELEPORT,
    OBJ_ID_COLLECTIBLE,
    OBJ_ID_HERO_PICKUP,
    OBJ_ID_STALACTITE,
    OBJ_ID_WALKER,
    OBJ_ID_FLYER,
    OBJ_ID_TRIGGERAREA,
    OBJ_ID_PUSHBLOCK,
    OBJ_ID_SPIKES,
    OBJ_ID_KEY,
    OBJ_ID_BOAT,
    OBJ_ID_HOOKLEVER,
    OBJ_ID_SPRITEDECAL,
    OBJ_ID_FLOATER,
    OBJ_ID_WALLWORM,
    OBJ_ID_WALLWORM_PARENT,
    OBJ_ID_HOOKPLANT,
    OBJ_ID_BLOCKSWING,
    OBJ_ID_STEAM_PLATFORM,
    OBJ_ID_BUDPLANT,
    OBJ_ID_PROJECTILE,
    OBJ_ID_STAMINARESTORER,
    OBJ_ID_CRABLER,
    OBJ_ID_CRANKSWITCH,
    OBJ_ID_CAMATTRACTOR,
    OBJ_ID_FROG,
    OBJ_ID_WEAPON_PICKUP,
    OBJ_ID_FLYBLOB,
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

void   floater_load(game_s *g, map_obj_s *mo);
void   boat_load(game_s *g, map_obj_s *mo);
void   box_load(game_s *g, map_obj_s *mo);
void   box_on_lift(game_s *g, obj_s *o);
void   box_on_drop(game_s *g, obj_s *o);
void   clockpulse_load(game_s *g, map_obj_s *mo);
void   crumbleblock_load(game_s *g, map_obj_s *mo);
void   crumbleblock_on_hooked(obj_s *o);
void   crumbleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   switch_load(game_s *g, map_obj_s *mo);
void   switch_on_interact(game_s *g, obj_s *o);
void   toggleblock_load(game_s *g, map_obj_s *mo);
void   toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   shroomy_load(game_s *g, map_obj_s *mo);
void   shroomy_bounced_on(obj_s *o);
void   crawler_load(game_s *g, map_obj_s *mo);
void   crawler_caterpillar_load(game_s *g, map_obj_s *mo);
void   crawler_on_weapon_hit(game_s *g, obj_s *o, hitbox_s hb);
void   carrier_load(game_s *g, map_obj_s *mo);
void   hero_powerup_obj_load(game_s *g, map_obj_s *mo);
void   hero_powerup_obj_on_update(game_s *g, obj_s *o);
void   hero_powerup_obj_on_draw(game_s *g, obj_s *o, v2_i32 cam);
i32    hero_powerup_obj_ID(obj_s *o);
void   movingplatform_load(game_s *g, map_obj_s *mo);
void   npc_load(game_s *g, map_obj_s *mo);
void   npc_on_update(game_s *g, obj_s *o);
void   npc_on_animate(game_s *g, obj_s *o);
void   charger_load(game_s *g, map_obj_s *mo);
obj_s *sign_popup_create(game_s *g);
void   sign_popup_load(game_s *g, map_obj_s *mo);
void   sign_popup_on_update(game_s *g, obj_s *o);
void   sign_popup_on_draw(game_s *g, obj_s *o, v2_i32 cam);
obj_s *sign_create(game_s *g);
void   sign_load(game_s *g, map_obj_s *mo);
void   swingdoor_load(game_s *g, map_obj_s *mo);
void   teleport_load(game_s *g, map_obj_s *mo);
void   stalactite_load(game_s *g, map_obj_s *mo);
void   walker_load(game_s *g, map_obj_s *mo);
void   flyer_load(game_s *g, map_obj_s *mo);
void   triggerarea_load(game_s *g, map_obj_s *mo);
void   spikes_load(game_s *g, map_obj_s *mo);
void   hooklever_load(game_s *g, map_obj_s *mo);
obj_s *spritedecal_create(game_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                          i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip);
void   spritedecal_on_update(game_s *g, obj_s *o);
void   spritedecal_on_animate(game_s *g, obj_s *o);
void   wallworm_load(game_s *g, map_obj_s *mo);
void   hookplant_load(game_s *g, map_obj_s *mo);
void   hookplant_on_hook(obj_s *o);
void   blockswing_load(game_s *g, map_obj_s *mo);
void   pushblock_load(game_s *g, map_obj_s *mo);
void   pushblock_on_update(game_s *g, obj_s *o);
void   pushblock_on_animate(game_s *g, obj_s *o);
void   steam_platform_load(game_s *g, map_obj_s *mo);
void   budplant_load(game_s *g, map_obj_s *mo);
void   budplant_on_update(game_s *g, obj_s *o);
void   budplant_on_animate(game_s *g, obj_s *o);
obj_s *projectile_create(game_s *g, v2_i32 pos, v2_i32 vel, i32 subID);
void   projectile_on_update(game_s *g, obj_s *o);
void   projectile_on_animate(game_s *g, obj_s *o);
void   projectile_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   projectile_on_collision(game_s *g, obj_s *o);
void   flyblob_load(game_s *g, map_obj_s *mo);
void   flyblob_on_update(game_s *g, obj_s *o);
void   flyblob_on_animate(game_s *g, obj_s *o);
void   flyblob_on_hit(game_s *g, obj_s *o, hitbox_s hb);
void   staminarestorer_load(game_s *g, map_obj_s *mo);
void   staminarestorer_try_collect_any(game_s *g, obj_s *ohero);
void   staminarestorer_respawn_all(game_s *g, obj_s *o);
void   crankswitch_load(game_s *g, map_obj_s *mo);
void   crankswitch_on_turn(game_s *g, obj_s *o, i32 angle_q8);
void   camattractor_static_load(game_s *g, map_obj_s *mo);
v2_i32 camattractor_static_closest_pt(obj_s *o, v2_i32 pt);
void   frog_load(game_s *g, map_obj_s *mo);
obj_s *hero_pickup_create(game_s *g, v2_i32 pos, i32 pickupID);
void   hero_pickup_load(game_s *g, map_obj_s *mo);
void   weapon_pickup_load(game_s *g, map_obj_s *mo);
void   weapon_pickup_place(game_s *g, obj_s *ohero);
void   weapon_pickup_on_pickup(game_s *g, obj_s *o, obj_s *ohero);

#endif