// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_powerup_obj_on_update(g_s *g, obj_s *o);
void hero_powerup_obj_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void hero_powerup_obj_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    obj_s *o = obj_create(g);
    o->ID    = OBJID_HERO_POWERUP;
    o->flags = OBJ_FLAG_COLLECTIBLE |
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
    o->state          = map_obj_i32(mo, "ID");
    o->substate       = saveID;
}

void hero_powerup_obj_on_update(g_s *g, obj_s *o)
{
}

void hero_powerup_obj_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    v2_i32    pos  = v2_i32_add(obj_pos_center(o), cam);
    gfx_ctx_s ctx2 = ctx;
    gfx_ctx_s ctx3 = ctx;
    ctx2.pat       = gfx_pattern_50();
    ctx3.pat       = gfx_pattern_75();
    gfx_cir_fill(ctx, pos, 22 + ((3 * sin_q16((g->tick << 11))) >> 16), GFX_COL_BLACK);
    gfx_cir_fill(ctx, pos, 20 + ((3 * sin_q16((g->tick << 11))) >> 16), GFX_COL_WHITE);
    gfx_cir_fill(ctx2, pos, 12 + ((3 * sin_q16((g->tick << 11))) >> 16), GFX_COL_BLACK);
    gfx_cir_fill(ctx3, pos, 8 + ((2 * sin_q16((g->tick << 11))) >> 16), GFX_COL_WHITE);
}

i32 hero_powerup_obj_ID(obj_s *o)
{
    return o->state;
}

i32 hero_powerup_saveID(obj_s *o)
{
    return o->substate;
}