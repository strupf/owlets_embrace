// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJDEF_H
#define OBJDEF_H

#include "gamedef.h"
#include "hitbox.h"
#include "map_loader.h"

enum {
    OBJID_NULL,
    OBJID_OWL,
    OBJID_COMPANION,
    OBJID_HOOK,
    OBJID_DOOR,
    OBJID_BIGCRAB,
    OBJID_MOLE,
    OBJID_CRUMBLEBLOCK,
    OBJID_SHORTCUTBLOCK,
    OBJID_SWITCH,
    OBJID_TENDRILCONNECTION,
    OBJID_MUSHROOMBLOCK,
    OBJID_DRILLER,
    OBJID_DRILLERSPAWN,
    OBJID_SOLIDLEVER,
    OBJID_LOOKAHEAD,
    OBJID_HEARTPIECE,
    OBJID_HOOKYEETER,
    OBJID_VINEBLOCKADE,
    OBJID_FROG,
    OBJID_FROG_TONGUE,
    OBJID_CLOCKPULSE,
    OBJID_MISC,
    OBJID_MOVINGBLOCK,
    OBJID_CRAWLER,
    OBJID_UPGRADETREE,
    OBJID_GEMPILE,
    OBJID_NPC,
    OBJID_TELEPORT,
    OBJID_JUMPER,
    OBJID_STALACTITE,
    OBJID_STALACTITE_SPAWN,
    OBJID_FLYER,
    OBJID_SPRINYBLOCK,
    OBJID_TRIGGERAREA,
    OBJID_PUSHBLOCK,
    OBJID_HOOKLEVER,
    OBJID_COIN,
    OBJID_ROTOR,
    OBJID_SPRITEDECAL,
    OBJID_STEAM_PLATFORM,
    OBJID_TUTORIALTEXT,
    OBJID_BUDPLANT,
    OBJID_BUDPLANT_GRENADE,
    OBJID_FALLINGSTONE,
    OBJID_FALLINGSTONE_SPAWN,
    OBJID_PROJECTILE,
    OBJID_STAMINARESTORER,
    OBJID_CRABLER,
    OBJID_CAMATTRACTOR,
    OBJID_FLYBLOB,
    OBJID_STOMPABLE_BLOCK,
    OBJID_LIGHT,
    OBJID_FALLINGBLOCK,
    OBJID_CHEST,
    OBJID_WATERLEAF,
    OBJID_WINDAREA,
    OBJID_TRAMPOLINE,
    OBJID_PULLEYBLOCK,
    OBJID_SAVEPOINT,
    OBJID_BOULDER,
    OBJID_BOULDER_SPAWN,
    OBJID_TIMER,
    OBJID_HEALTHDROP,
    OBJID_CRAB,
    OBJID_CRACKBLOCK,
    OBJID_MUSHROOM,
    OBJID_LEVERPUSHPULL,
    OBJID_EXIT_BLOCKER,
    OBJID_DRILLJUMPER,
    OBJID_BOMBPLANT,
    OBJID_BOMB,
    //
    OBJID_PUPPET_HERO,
    OBJID_PUPPET_COMPANION,
    OBJID_PUPPET_MOLE,
    //
    OBJID_BOSS_PLANT_TENTACLE,
    OBJID_BOSS_PLANT_EYE,
    OBJID_BOSS_PLANT_EYE_FAKE_L,
    OBJID_BOSS_PLANT_EYE_FAKE_R,
};

enum {
    OBJ_TAG_OWL,
    OBJ_TAG_HOOK,
    OBJ_TAG_COMPANION,
    OBJ_TAG_BOSS,
    //
    NUM_OBJ_TAGS
};

// enemy kills getting tracked
enum {
    ENEMYID_CRAB,
    ENEMYID_FROG,
    ENEMYID_JUMPER,
    ENEMYID_FLYBLOB,
    ENEMYID_CRAWLER,
    //
    NUM_ENEMYID
};

enum {
    PROJECTILE_ID_DEFAULT,
    PROJECTILE_ID_ENEMY,
    PROJECTILE_ID_BUDPLANT,
    PROJECTILE_ID_STALACTITE_BREAK,
};

enum {
    ANIMOBJ_EXPLOSION_1,
    ANIMOBJ_EXPLOSION_2,
    ANIMOBJ_EXPLOSION_3,
    ANIMOBJ_EXPLOSION_4,
    ANIMOBJ_EXPLOSION_5,
    ANIMOBJ_ENEMY_SPAWN,
    ANIMOBJ_STOMP,
};

obj_s    *animobj_create(g_s *g, v2_i32 p, i32 animobjID);
void      bombplant_load(g_s *g, map_obj_s *mo);
void      clockpulse_load(g_s *g, map_obj_s *mo);
void      crumbleblock_load(g_s *g, map_obj_s *mo);
void      crumbleblock_on_hooked(g_s *g, obj_s *o);
void      crumbleblock_break(g_s *g, obj_s *o);
void      switch_load(g_s *g, map_obj_s *mo);
void      switch_on_interact(g_s *g, obj_s *o);
void      mushroomblock_load(g_s *g, map_obj_s *mo);
void      crawler_load(g_s *g, map_obj_s *mo);
void      crawler_on_hurt(g_s *g, obj_s *o);
void      upgradetree_load(g_s *g, map_obj_s *mo);
void      upgradetree_put_orb_infront(obj_s *o);
void      upgradetree_move_orb_to(obj_s *o, v2_i32 pos, i32 t);
v2_i32    upgradetree_orb_pos(obj_s *o);
void      upgradetree_disable_orb(obj_s *o);
void      upgradetree_collect(g_s *g, obj_s *o);
void      drilljumper_load(g_s *g, map_obj_s *mo);
void      npc_load(g_s *g, map_obj_s *mo);
void      npc_on_update(g_s *g, obj_s *o);
void      npc_on_animate(g_s *g, obj_s *o);
void      npc_on_interact(g_s *g, obj_s *o);
void      crackblock_load(g_s *g, map_obj_s *mo);
void      teleport_load(g_s *g, map_obj_s *mo);
void      crab_load(g_s *g, map_obj_s *mo);
void      crab_on_hurt(g_s *g, obj_s *o);
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
void      vineblockade_load(g_s *g, map_obj_s *mo);
obj_s    *spritedecal_create(g_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                             i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip);
void      pushblock_load(g_s *g, map_obj_s *mo);
void      steam_platform_load(g_s *g, map_obj_s *mo);
void      budplant_load(g_s *g, map_obj_s *mo);
obj_s    *projectile_create(g_s *g, v2_i32 pos, v2_i32 vel, i32 subID);
void      projectile_on_collision(g_s *g, obj_s *o);
void      flyblob_load(g_s *g, map_obj_s *mo);
void      flyblob_on_hurt(g_s *g, obj_s *o, hitbox_s *hb);
void      staminarestorer_load(g_s *g, map_obj_s *mo);
bool32    staminarestorer_try_collect(g_s *g, obj_s *o, obj_s *ohero);
void      staminarestorer_respawn_all(g_s *g, obj_s *o);
void      camattractor_load(g_s *g, map_obj_s *mo);
v2_i32    camattractor_closest_pt(obj_s *o, v2_i32 pt);
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
void      windarea_on_update(g_s *g, obj_s *o);
void      windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam);
obj_s    *coin_create(g_s *g);
void      coin_load(g_s *g, map_obj_s *mo);
void      coin_try_collect(g_s *g, obj_s *o, v2_i32 heropos);
void      door_load(g_s *g, map_obj_s *mo);
void      trampoline_load(g_s *g, map_obj_s *mo);
void      trampolines_do_bounce(g_s *g);
void      savepoint_load(g_s *g, map_obj_s *mo);
void      rotor_load(g_s *g, map_obj_s *mo);
obj_s    *hookyeeter_create(g_s *g);
void      hookyeeter_on_update(g_s *g, obj_s *o);
void      hookyeeter_on_hook(g_s *g, obj_s *o);
void      hookyeeter_on_unhook(g_s *g, obj_s *o);
void      jumper_load(g_s *g, map_obj_s *mo);
void      jumper_on_hurt(g_s *g, obj_s *o);
void      solidlever_load(g_s *g, map_obj_s *mo);
obj_s    *companion_create(g_s *g);
obj_s    *companion_spawn(g_s *g, obj_s *ohero);
void      companion_on_enter_mode(g_s *g, obj_s *o, i32 mode);
void      companion_on_owl_died(g_s *g, obj_s *o);
v2_i32    companion_pos_swap(obj_s *ocomp, obj_s *o_owl);
void      fallingstonespawn_load(g_s *g, map_obj_s *mo);
obj_s    *healthdrop_spawn(g_s *g, v2_i32 p);
void      springyblock_load(g_s *g, map_obj_s *mo);
void      pulleyblocks_setup(g_s *g);
void      pulleyblock_load_parent(g_s *g, map_obj_s *mo);
void      pulleyblock_load_child(g_s *g, map_obj_s *mo);
void      gempile_load(g_s *g, map_obj_s *mo);
void      gempile_on_hit(g_s *g, obj_s *o);
void      mushroom_load(g_s *g, map_obj_s *mo);
void      mushroom_on_jump_on(g_s *g, obj_s *o);
void      tutorialtext_load(g_s *g, map_obj_s *mo);
void      lookahead_load(g_s *g, map_obj_s *mo);
void      heart_or_stamina_piece_load(g_s *g, map_obj_s *mo, bool32 is_stamina);
void      heartpiece_on_collect(g_s *g, obj_s *o);
void      bigcrab_load(g_s *g, map_obj_s *mo);
void      drillerspawn_load(g_s *g, map_obj_s *mo);
void      drillers_setup(g_s *g);
void      movingblock_load(g_s *g, map_obj_s *mo);
void      shortcutblock_load(g_s *g, map_obj_s *mo);
obj_s    *tendrilconnection_create(g_s *g);
void      tendrilconnection_setup(g_s *g, obj_s *o, v2_i32 p_start, v2_i32 p_end, i32 l_max);
void      tendrilconnection_constrain_ends(obj_s *o, v2_i32 p_start, v2_i32 p_end);
void      leverpushpull_load(g_s *g, map_obj_s *mo);
void      frog_on_load(g_s *g, map_obj_s *mo);
void      frog_on_hurt(g_s *g, obj_s *o);
void      bombplant_load(g_s *g, map_obj_s *mo);
void      bombplant_on_pickup(g_s *g, obj_s *o);
void      bombplant_on_hit(g_s *g, obj_s *o);
obj_s    *bomb_create(g_s *g);
void      bomb_set_carried(obj_s *o);
void      bomb_set_idle(obj_s *o);
#endif