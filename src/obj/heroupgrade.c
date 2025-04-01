// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_powerup_obj_on_update(g_s *g, obj_s *o);
void hero_powerup_obj_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void hero_powerup_obj_load(g_s *g, map_obj_s *mo)
{

    obj_s *o   = obj_create(g);
    o->ID      = OBJID_HERO_POWERUP;
    o->w       = 16;
    o->h       = 16;
    o->on_draw = hero_powerup_obj_on_draw;
    o->pos.x   = mo->x;
    o->pos.y   = mo->y;
    o->state   = map_obj_i32(mo, "ID");

    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) {
        o->state = 1;
    } else {
        o->state = 0;
    }
    o->substate = saveID;
}

void hero_powerup_obj_on_update(g_s *g, obj_s *o)
{
}

void hero_powerup_obj_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx     = gfx_ctx_display();
    texrec_s  trtree  = asset_texrec(TEXID_UPGRADE, 0, 0, 256, 256);
    v2_i32    porigin = {o->pos.x + cam.x + o->w / 2,
                         o->pos.y + cam.y + o->h};
    v2_i32    ptree   = {porigin.x - trtree.w / 2,
                         porigin.y - trtree.h};

    gfx_spr(ctx, trtree, ptree, 0, 0);

    if (!o->state) {
        i32      fr     = ani_frame(ANIID_UPGRADE, g->tick);
        texrec_s trupgr = asset_texrec(TEXID_UPGRADE, 256, fr * 64, 64, 64);
        v2_i32   pupgr  = {porigin.x - 22, porigin.y - 85};
        gfx_spr(ctx, trupgr, pupgr, 0, 0);
    }

    trtree.y += 256;
    gfx_spr(ctx, trtree, ptree, 0, 0);
}

i32 hero_powerup_obj_ID(obj_s *o)
{
    return o->state;
}

i32 hero_powerup_saveID(obj_s *o)
{
    return o->substate;
}