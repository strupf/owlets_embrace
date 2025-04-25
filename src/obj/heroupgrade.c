// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 orb_src;
    v2_i32 orb_dst;
    i32    orb_t;
    i32    orb_t_total;
    i32    orb_infront;
    u32    saveID;
    v2_i32 orb;
} hero_powup_s;

void hero_upgrade_on_animate(g_s *g, obj_s *o);
void hero_upgrade_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void hero_upgrade_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o = obj_create(g);
    hero_powup_s *p = (hero_powup_s *)o->mem;
    o->ID           = OBJID_HERO_UPGRADE;
    o->w            = mo->w;
    o->h            = mo->h;
    o->on_draw      = hero_upgrade_on_draw;
    o->on_animate   = hero_upgrade_on_animate;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->state        = map_obj_i32(mo, "ID");

    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) {
        o->state = 1;
    } else {
        o->state = 0;
    }
    p->saveID      = saveID;
    v2_i32 porigin = obj_pos_bottom_center(o);
    p->orb.x       = porigin.x + 10;
    p->orb.y       = porigin.y - 53;

    rec_i32 rtrigger = {mo->x, mo->y, mo->w, mo->h};
    triggerarea_spawn(g, rtrigger, TRIGGER_CS_UPGRADE, 0, 1);
}

v2_i32 hero_upgrade_orb_pos(obj_s *o)
{
    hero_powup_s *p = (hero_powup_s *)o->mem;
    return p->orb;
}

void hero_upgrade_move_orb_to(obj_s *o, v2_i32 pos, i32 t)
{
    hero_powup_s *p = (hero_powup_s *)o->mem;
    p->orb_src      = p->orb;
    p->orb_dst      = pos;
    p->orb_t        = 0;
    p->orb_t_total  = t;
}

void hero_upgrade_put_orb_infront(obj_s *o)
{
    hero_powup_s *p = (hero_powup_s *)o->mem;
    p->orb_infront  = 1;
}

void hero_upgrade_on_animate(g_s *g, obj_s *o)
{
    hero_powup_s *p = (hero_powup_s *)o->mem;
    o->animation++;

    if (p->orb_t_total) {
        p->orb_t++;

        p->orb = v2_i32_lerp(p->orb_src, p->orb_dst, p->orb_t, p->orb_t_total);
        if (p->orb_t_total == p->orb_t) {
            p->orb_t_total = 0;
            p->orb_t       = 0;
        }
    }
}

void hero_upgrade_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    hero_powup_s *p       = (hero_powup_s *)o->mem;
    gfx_ctx_s     ctx     = gfx_ctx_display();
    texrec_s      trtree  = asset_texrec(TEXID_UPGRADE, 0, 0, 256, 256);
    v2_i32        porigin = {o->pos.x + cam.x + o->w / 2,
                             o->pos.y + cam.y + o->h};
    v2_i32        ptree   = {porigin.x - trtree.w / 2,
                             porigin.y - trtree.h};

    gfx_spr(ctx, trtree, ptree, 0, 0);
    trtree.y += 256;

    if (p->orb_infront) {
        gfx_spr(ctx, trtree, ptree, 0, 0);
    }

    if (!o->state) {
        i32      fr     = ani_frame(ANIID_UPGRADE, o->animation);
        texrec_s trupgr = asset_texrec(TEXID_UPGRADE, 256, fr * 64, 64, 64);
        v2_i32   pupgr  = v2_i32_add(p->orb, cam);
        pupgr.x -= 32;
        pupgr.y -= 32;
        gfx_spr(ctx, trupgr, pupgr, 0, 0);
    }

    if (!p->orb_infront) {
        gfx_spr(ctx, trtree, ptree, 0, 0);
    }
}

i32 hero_upgrade_ID(obj_s *o)
{
    return o->state;
}

i32 hero_upgrade_saveID(obj_s *o)
{
    return o->substate;
}