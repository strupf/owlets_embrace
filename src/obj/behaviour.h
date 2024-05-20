// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include "gamedef.h"
#include "hero.h"
#include "map_loader.h"

void   floater_load(game_s *g, map_obj_s *mo);
//
void   boat_load(game_s *g, map_obj_s *mo);
//
void   clockpulse_load(game_s *g, map_obj_s *mo);
//
void   crumbleblock_load(game_s *g, map_obj_s *mo);
void   crumbleblock_on_hooked(obj_s *o);
void   crumbleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
void   switch_load(game_s *g, map_obj_s *mo);
void   switch_on_interact(game_s *g, obj_s *o);
//
void   toggleblock_load(game_s *g, map_obj_s *mo);
void   toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
void   shroomy_load(game_s *g, map_obj_s *mo);
void   shroomy_bounced_on(obj_s *o);
//
void   crawler_load(game_s *g, map_obj_s *mo);
void   crawler_caterpillar_load(game_s *g, map_obj_s *mo);
void   crawler_on_weapon_hit(game_s *g, obj_s *o, hitbox_s hb);
//
void   carrier_load(game_s *g, map_obj_s *mo);
//
void   heroupgrade_load(game_s *g, map_obj_s *mo);
void   heroupgrade_on_collect(game_s *g, obj_s *o);
void   heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
void   movingplatform_load(game_s *g, map_obj_s *mo);
//
void   npc_load(game_s *g, map_obj_s *mo);
//
void   charger_load(game_s *g, map_obj_s *mo);
//
obj_s *sign_popup_create(game_s *g);
void   sign_popup_load(game_s *g, map_obj_s *mo);
void   sign_popup_on_update(game_s *g, obj_s *o);
void   sign_popup_on_draw(game_s *g, obj_s *o, v2_i32 cam);
obj_s *sign_create(game_s *g);
void   sign_load(game_s *g, map_obj_s *mo);
//
void   swingdoor_load(game_s *g, map_obj_s *mo);
//
void   teleport_load(game_s *g, map_obj_s *mo);
//
void   juggernaut_load(game_s *g, map_obj_s *mo);
//
void   stalactite_load(game_s *g, map_obj_s *mo);
//
void   walker_load(game_s *g, map_obj_s *mo);
//
void   flyer_load(game_s *g, map_obj_s *mo);
//
void   triggerarea_load(game_s *g, map_obj_s *mo);
//
void   pushablebox_load(game_s *g, map_obj_s *mo);
//
void   spikes_load(game_s *g, map_obj_s *mo);
void   spikes_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
void   hooklever_load(game_s *g, map_obj_s *mo);
//
obj_s *spritedecal_create(game_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                          i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, int flip);
//
void   pot_load(game_s *g, map_obj_s *mo);
void   pot_on_pickup(game_s *g, obj_s *o, obj_s *ohero);
void   pot_on_throw(game_s *g, obj_s *o, i32 dir_x);
#endif