// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box);

void obj_game_player_attackbox(game_s *g, hitbox_s box)
{
    for (obj_each(g, o)) {
        rec_i32 aabb = obj_aabb(o);
        if (!overlap_rec(aabb, box.r)) continue;
        obj_game_player_attackbox_o(g, o, box);
    }
}

void obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box)
{
    switch (o->ID) {
    case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
    }
}