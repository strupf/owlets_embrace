// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void sign_interact_read(game_s *g, obj_s *o);

obj_s *sign_create(game_s *g)
{
        obj_s  *o     = obj_create(g);
        flags64 flags = OBJ_FLAG_INTERACT;
        obj_apply_flags(g, o, flags);
        o->oninteract        = sign_interact_read;
        o->interactable_type = INTERACTABLE_TYPE_READ;
        o->ID                = OBJ_ID_SIGN;
        return o;
}

static void sign_interact_read(game_s *g, obj_s *o)
{
        textbox_load_dialog_mode(&g->textbox, o->filename, TEXTBOX_MODE_STATIC_SIGN);
}