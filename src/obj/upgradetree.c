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
} upgradetree_s;

void upgradetree_on_update(g_s *g, obj_s *o);
void upgradetree_on_animate(g_s *g, obj_s *o);
void upgradetree_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void upgradetree_load(g_s *g, map_obj_s *mo)
{
    obj_s         *o = obj_create(g);
    upgradetree_s *p = (upgradetree_s *)o->mem;
    o->UUID          = mo->UUID;
    o->ID            = OBJID_UPGRADETREE;
    o->w             = mo->w;
    o->h             = mo->h;
    o->on_draw       = upgradetree_on_draw;
    o->on_animate    = upgradetree_on_animate;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->substate      = map_obj_i32(mo, "ID");

    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) {
        o->state = 1;
    } else {
        o->on_update   = upgradetree_on_update;
        v2_i32 porigin = obj_pos_bottom_center(o);
        p->orb.x       = porigin.x + 10;
        p->orb.y       = porigin.y - 53;
    }

    tex_from_wad_ID(TEXID_UPGRADE, "T_UPGRADE", game_per_room_allocator(g));
    snd_from_wad_ID(SNDID_UPGRADE, "S_UPGRADE", game_per_room_allocator(g));
    p->saveID = saveID;
}

void upgradetree_on_update(g_s *g, obj_s *o)
{
    obj_s *owl = owl_if_present_and_alive(g);
    if (!owl) return;

    if (overlap_rec(obj_aabb(owl), obj_aabb(o))) {
        cs_powerup_enter(g);
    }
}

v2_i32 upgradetree_orb_pos(obj_s *o)
{
    upgradetree_s *p = (upgradetree_s *)o->mem;
    return p->orb;
}

void upgradetree_move_orb_to(obj_s *o, v2_i32 pos, i32 t)
{
    upgradetree_s *p = (upgradetree_s *)o->mem;
    p->orb_src       = p->orb;
    p->orb_dst       = pos;
    p->orb_t         = 0;
    p->orb_t_total   = t;
}

void upgradetree_put_orb_infront(obj_s *o)
{
    upgradetree_s *p = (upgradetree_s *)o->mem;
    p->orb_infront   = 1;
}

void upgradetree_on_animate(g_s *g, obj_s *o)
{
    upgradetree_s *p = (upgradetree_s *)o->mem;
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

void upgradetree_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    upgradetree_s *p       = (upgradetree_s *)o->mem;
    gfx_ctx_s      ctx     = gfx_ctx_display();
    texrec_s       trtree  = asset_texrec(TEXID_UPGRADE, 0, 0, 256, 256);
    v2_i32         porigin = {o->pos.x + cam.x + o->w / 2,
                              o->pos.y + cam.y + o->h};
    v2_i32         ptree   = {porigin.x - trtree.w / 2,
                              porigin.y - trtree.h};

    gfx_spr(ctx, trtree, ptree, 0, 0);
    trtree.y += 256;

    if (p->orb_infront) {
        gfx_spr(ctx, trtree, ptree, 0, 0);
    }

    if (!o->state) {
        i32      fr     = ani_frame_loop(ANIID_UPGRADE, o->animation);
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

void upgradetree_collect(g_s *g, obj_s *o)
{
    obj_s *owl = owl_if_present_and_alive(g);
    if (!owl) return;

    upgradetree_s *p = (upgradetree_s *)o->mem;
    owl_upgrade_add(owl, (u32)1 << o->substate);
    save_event_register(g, p->saveID);
    game_update_savefile(g);
}

void upgradetree_disable_orb(obj_s *o)
{
    o->state = 1;
}