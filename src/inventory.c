// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

bool32 inventory_active(inventory_s *inv)
{
    return inv->active;
}

void inventory_update(game_s *g, inventory_s *inv)
{
    if (inp_just_pressed(INP_A)) {
        inv->active = 0;
    }

    sys_menu_clr(); // clear menu
}

void inventory_draw(game_s *g, inventory_s *inv)
{
    const gfx_ctx_s ctx = gfx_ctx_display();
}

void inventory_open(game_s *g, inventory_s *inv)
{
    inv->active = 1;
    sys_printf("Open Menu\n");
}
