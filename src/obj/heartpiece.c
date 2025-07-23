// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void heartpiece_on_animate(g_s *g, obj_s *o);

void heart_or_stamina_piece_load(g_s *g, map_obj_s *mo, bool32 is_stamina)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    obj_s *o = obj_create(g);
    o->ID    = OBJID_HEARTPIECE;
    o->subID = is_stamina != 0;
    o->w     = 32;
    o->h     = 32;
    obj_place_to_map_obj(o, mo, 0, 0);
    o->on_animate = heartpiece_on_animate;
    o->substate   = saveID;
    o->n_sprites  = 1;
}

void heartpiece_on_collect(g_s *g, obj_s *o)
{
    save_event_register(g, o->substate);
    obj_delete(g, o);
    cs_aquire_heartpiece_enter(g, o->subID);
}

void heartpiece_on_animate(g_s *g, obj_s *o)
{
    o->animation++;
    obj_sprite_s *spr = &o->sprites[0];
    i32           fx  = o->subID;
    i32           fy  = (o->animation >> 3) % 5;
    spr->trec         = asset_texrec(TEXID_FEATHERUPGR, fx * 32, fy * 32, 32, 32);
    spr->offs.x       = (32 - 32) / 2;
    spr->offs.y       = -12;

    // spr->offs.y += (((sin_q15(o->animation << 10) * 4) / 32769));
}
