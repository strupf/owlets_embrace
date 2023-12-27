// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include "gamedef.h"

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
void   switch_on_animate(game_s *g, obj_s *o);
void   switch_on_interact(game_s *g, obj_s *o);
//
obj_s *toggleblock_create(game_s *g);
void   toggleblock_on_animate(game_s *g, obj_s *o);
void   toggleblock_on_draw(game_s *g, obj_s *o, v2_i32 cam);
void   toggleblock_on_trigger(game_s *g, obj_s *o);
//
obj_s *fallingblock_create(game_s *g);
void   fallingblock_on_update(game_s *g, obj_s *o);
void   fallingblock_on_animate(game_s *g, obj_s *o);
//
obj_s *shroomy_create(game_s *g);
void   shroomy_on_update(game_s *g, obj_s *o);
void   shroomy_on_animate(game_s *g, obj_s *o);

#endif