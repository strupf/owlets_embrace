// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include "gamedef.h"
#include "hero.h"
#include "map_loader.h"

obj_s *blob_create(game_s *g);
void   blob_on_update(game_s *g, obj_s *o);
void   blob_on_animate(game_s *g, obj_s *o);
void   blob_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
obj_s *clockpulse_create(game_s *g);
void   clockpulse_update(game_s *g, obj_s *o);
//
obj_s *crumbleblock_create(game_s *g);
void   crumbleblock_update(game_s *g, obj_s *o);
//
obj_s *switch_create(game_s *g);
obj_s *switch_load(game_s *g, map_obj_s *mo);
void   switch_on_animate(game_s *g, obj_s *o);
void   switch_on_interact(game_s *g, obj_s *o);
//
obj_s *toggleblock_create(game_s *g);
void   toggleblock_on_animate(game_s *g, obj_s *o);
void   toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   toggleblock_on_trigger(game_s *g, obj_s *o, int trigger);
//
obj_s *fallingblock_create(game_s *g);
void   fallingblock_on_update(game_s *g, obj_s *o);
void   fallingblock_on_animate(game_s *g, obj_s *o);
//
obj_s *shroomy_create(game_s *g);
void   shroomy_on_update(game_s *g, obj_s *o);
void   shroomy_on_animate(game_s *g, obj_s *o);
//
obj_s *crawler_create(game_s *g);
void   crawler_on_update(game_s *g, obj_s *o);
void   crawler_on_animate(game_s *g, obj_s *o);
void   crawler_on_weapon_hit(obj_s *o, hitbox_s hb);
//
obj_s *carrier_create(game_s *g);
void   carrier_on_update(game_s *g, obj_s *o);
//
obj_s *heroupgrade_create(game_s *g);
void   heroupgrade_on_collect(game_s *g, obj_s *o, herodata_s *h);
void   heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam);
//
obj_s *movingplatform_create(game_s *g);
void   movingplatform_on_update(game_s *g, obj_s *o);

#endif