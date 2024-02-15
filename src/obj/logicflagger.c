// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    int flag;
    int trigger_flag_set;
    int trigger_flag_clr;
} logicflagger_s;

void logicflagger_load(game_s *g, map_obj_s *mo)
{
    obj_s          *o    = obj_create(g);
    logicflagger_s *lf   = (logicflagger_s *)o->mem;
    o->ID                = OBJ_ID_LOGICFLAGGER;
    lf->flag             = map_obj_i32(mo, "F");
    lf->trigger_flag_set = map_obj_i32(mo, "Trigger_Set");
    lf->trigger_flag_clr = map_obj_i32(mo, "Trigger_Clr");
}

void logicflagger_on_trigger(game_s *g, obj_s *o, int trigger)
{
    logicflagger_s *lf = (logicflagger_s *)o->mem;
    if (trigger == lf->trigger_flag_clr) {
        logic_flag_clr(g, lf->flag - 1);
    }
    if (trigger == lf->trigger_flag_set) {
        logic_flag_set(g, lf->flag - 1);
    }
}