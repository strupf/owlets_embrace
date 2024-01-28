// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "shop.h"
#include "game.h"

bool32 shop_active(game_s *g)
{
    return g->shop.active;
}

void shop_open(game_s *g)
{
    g->shop.active = 1;
}

void shop_update(game_s *g)
{
    if (inp_just_pressed(INP_A)) {
        g->shop.active = 0;
    }
}

void shop_draw(game_s *g)
{
    gfx_ctx_s ctxd = gfx_ctx_display();

    gfx_rec_fill(ctxd, (rec_i32){100, 100, 100, 40}, PRIM_MODE_BLACK);
}