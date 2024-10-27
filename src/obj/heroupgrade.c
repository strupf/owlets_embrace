// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_powerup_obj_on_update(game_s *g, obj_s *o);
void hero_powerup_obj_on_draw(game_s *g, obj_s *o, v2_i32 cam);

void hero_powerup_obj_load(game_s *g, map_obj_s *mo)
{
    i32 upgrade = map_obj_i32(mo, "ID");
    // if (hero_has_upgrade(g, upgrade)) return;

    obj_s *o     = obj_create(g);
    o->on_update = hero_powerup_obj_on_update;
    o->on_draw   = hero_powerup_obj_on_draw;
    o->ID        = OBJ_ID_HERO_POWERUP;
    o->flags     = OBJ_FLAG_COLLECTIBLE |
               OBJ_FLAG_SPRITE;
    o->w = 16;
    o->h = 16;

    o->n_sprites      = 0;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_MISCOBJ, 0, 0, 32, 32);
    spr->offs.x       = -8;
    spr->offs.y       = -8;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->state          = upgrade;
}

void hero_powerup_obj_on_update(game_s *g, obj_s *o)
{
    particle_desc_s prt = {0};
    {

        prt.p.p_q8      = v2_shl(obj_pos_center(o), 8);
        prt.p.v_q8.x    = 0;
        prt.p.v_q8.y    = 0;
        prt.p.size      = 5;
        prt.p.ticks_max = 60;
        prt.ticksr      = 10;
        prt.pr_q8.x     = 2000;
        prt.pr_q8.y     = 2000;
        prt.vr_q8.x     = 100;
        prt.vr_q8.y     = 100;
        prt.ar_q8.x     = 0;
        prt.ar_q8.y     = 0;
        prt.sizer       = 2;
        prt.p.gfx       = PARTICLE_GFX_CIR;
        prt.p.col       = GFX_COL_WHITE;
        particles_spawn(g, prt, 2);
        prt.p.col  = GFX_COL_BLACK;
        prt.p.size = 2;
        prt.sizer  = 1;
        particles_spawn(g, prt, 2);
    }
}

void hero_powerup_obj_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    v2_i32    pos  = v2_add(obj_pos_center(o), cam);
    gfx_ctx_s ctx2 = ctx;
    gfx_ctx_s ctx3 = ctx;
    ctx2.pat       = gfx_pattern_50();
    ctx3.pat       = gfx_pattern_75();
    gfx_cir_fill(ctx, pos, 22 + ((3 * sin_q16((g->gameplay_tick << 11))) >> 16), GFX_COL_BLACK);
    gfx_cir_fill(ctx, pos, 20 + ((3 * sin_q16((g->gameplay_tick << 11))) >> 16), GFX_COL_WHITE);
    gfx_cir_fill(ctx2, pos, 12 + ((3 * sin_q16((g->gameplay_tick << 11))) >> 16), GFX_COL_BLACK);
    gfx_cir_fill(ctx3, pos, 8 + ((2 * sin_q16((g->gameplay_tick << 11))) >> 16), GFX_COL_WHITE);
}

i32 hero_powerup_obj_ID(obj_s *o)
{
    return o->state;
}