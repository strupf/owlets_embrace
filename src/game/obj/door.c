// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "door.h"
#include "game/game.h"

void door_think(game_s *g, obj_s *o)
{
        if (o->door.moved > 0) {
                solid_move(g, o, 0, -1);
                o->door.moved--;
                if (o->door.moved == 0) {
                        objflags_s flags = o->flags;
                        flags            = objflags_unset(flags,
                                                          OBJ_FLAG_THINK_1);
                        obj_set_flags(g, o, flags);
                }
        }
}

void door_trigger(game_s *g, obj_s *o, int triggerID)
{
        if (o->ID == triggerID && !o->door.triggered) {
                // obj_delete(g, o);
                o->door.triggered = 1;
                objflags_s flags  = o->flags;
                flags             = objflags_set(flags,
                                                 OBJ_FLAG_THINK_1);
                obj_set_flags(g, o, flags);
                o->think_1    = door_think;
                o->door.moved = 100;
                textbox_load_dialog(&g->textbox, "assets/dialog_2.txt");
        }
}

obj_s *door_create(game_s *g)
{
        obj_s     *o     = obj_create(g);
        objflags_s flags = objflags_create(
            OBJ_FLAG_SOLID);
        obj_set_flags(g, o, flags);
        o->pos.x     = 340 + 16;
        o->pos.y     = 192 - 64 - 16;
        o->w         = 16;
        o->h         = 64 + 16;
        o->ontrigger = door_trigger;
        o->ID        = 4;
        return o;
}