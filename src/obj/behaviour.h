// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include "gamedef.h"
#include "hero.h"
#include "map_loader.h"

void   boat_load(game_s *g, map_obj_s *mo);
void   boat_on_update(game_s *g, obj_s *o);
void   boat_on_animate(game_s *g, obj_s *o);
//
void   clockpulse_load(game_s *g, map_obj_s *mo);
void   clockpulse_on_update(game_s *g, obj_s *o);
void   clockpulse_on_trigger(game_s *g, obj_s *o, int trigger);
//
obj_s *crumbleblock_create(game_s *g);
void   crumbleblock_load(game_s *g, map_obj_s *mo);
void   crumbleblock_on_hooked(obj_s *o);
void   crumbleblock_on_update(game_s *g, obj_s *o);
void   crumbleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
obj_s *switch_create(game_s *g);
void   switch_load(game_s *g, map_obj_s *mo);
void   switch_on_animate(game_s *g, obj_s *o);
void   switch_on_interact(game_s *g, obj_s *o);
//
void   toggleblock_load(game_s *g, map_obj_s *mo);
void   toggleblock_on_animate(game_s *g, obj_s *o);
void   toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   toggleblock_on_trigger(game_s *g, obj_s *o, int trigger);
//
obj_s *shroomy_create(game_s *g);
void   shroomy_load(game_s *g, map_obj_s *mo);
void   shroomy_bounced_on(obj_s *o);
void   shroomy_on_update(game_s *g, obj_s *o);
void   shroomy_on_animate(game_s *g, obj_s *o);
//
void   crawler_load(game_s *g, map_obj_s *mo);
void   crawler_caterpillar_load(game_s *g, map_obj_s *mo);
void   crawler_on_update(game_s *g, obj_s *o);
void   crawler_on_animate(game_s *g, obj_s *o);
void   crawler_on_weapon_hit(game_s *g, obj_s *o, hitbox_s hb);
//
obj_s *carrier_create(game_s *g);
void   carrier_load(game_s *g, map_obj_s *mo);
void   carrier_on_update(game_s *g, obj_s *o);
void   carrier_on_animate(game_s *g, obj_s *o);
//
obj_s *heroupgrade_create(game_s *g);
void   heroupgrade_load(game_s *g, map_obj_s *mo);
void   heroupgrade_on_collect(game_s *g, obj_s *o);
void   heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
obj_s *movingplatform_create(game_s *g);
void   movingplatform_load(game_s *g, map_obj_s *mo);
void   movingplatform_on_update(game_s *g, obj_s *o);
void   movingplatform_on_trigger(game_s *g, obj_s *o, int trigger);
//
void   npc_load(game_s *g, map_obj_s *mo);
void   npc_on_update(game_s *g, obj_s *o);
void   npc_on_interact(game_s *g, obj_s *o);
void   npc_on_animate(game_s *g, obj_s *o);
//
obj_s *charger_create(game_s *g);
void   charger_load(game_s *g, map_obj_s *mo);
void   charger_on_update(game_s *g, obj_s *o);
void   charger_on_animate(game_s *g, obj_s *o);
//
obj_s *sign_popup_create(game_s *g);
void   sign_popup_load(game_s *g, map_obj_s *mo);
void   sign_popup_on_update(game_s *g, obj_s *o);
void   sign_popup_on_draw(game_s *g, obj_s *o, v2_i32 cam);
obj_s *sign_create(game_s *g);
void   sign_load(game_s *g, map_obj_s *mo);
void   sign_on_interact(game_s *g, obj_s *o);
//
void   swingdoor_load(game_s *g, map_obj_s *mo);
void   swingdoor_on_update(game_s *g, obj_s *o);
void   swingdoor_on_interact(game_s *g, obj_s *o);
void   swingdoor_on_animate(game_s *g, obj_s *o);
void   swingdoor_on_trigger(game_s *g, obj_s *o, int trigger);
//
obj_s *teleport_create(game_s *g);
void   teleport_load(game_s *g, map_obj_s *mo);
void   teleport_on_interact(game_s *g, obj_s *o);
//
obj_s *juggernaut_create(game_s *g);
void   juggernaut_load(game_s *g, map_obj_s *mo);
void   juggernaut_on_update(game_s *g, obj_s *o);
void   juggernaut_on_animate(game_s *g, obj_s *o);
//
void   stalactite_load(game_s *g, map_obj_s *mo);
void   stalactite_on_update(game_s *g, obj_s *o);
//
void   walker_load(game_s *g, map_obj_s *mo);
void   walker_on_update(game_s *g, obj_s *o);
void   walker_on_animate(game_s *g, obj_s *o);
//
void   flyer_load(game_s *g, map_obj_s *mo);
void   flyer_on_update(game_s *g, obj_s *o);
void   flyer_on_animate(game_s *g, obj_s *o);
//
void   triggerarea_load(game_s *g, map_obj_s *mo);
void   triggerarea_update(game_s *g, obj_s *o);
//
void   pushablebox_load(game_s *g, map_obj_s *mo);
void   pushablebox_update(game_s *g, obj_s *o);
//
void   spikes_load(game_s *g, map_obj_s *mo);
void   spikes_on_trigger(game_s *g, obj_s *o, int trigger);
void   spikes_on_animate(game_s *g, obj_s *o);
void   spikes_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
void   hooklever_load(game_s *g, map_obj_s *mo);
void   hooklever_on_update(game_s *g, obj_s *o);
void   hooklever_on_animate(game_s *g, obj_s *o);
//
obj_s *spritedecal_create(game_s *g, i32 render_priority, v2_i32 pos,
                          i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames);
void   spritedecal_on_update(game_s *g, obj_s *o);
#endif