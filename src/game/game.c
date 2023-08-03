// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

static void game_cull_scheduled(game_s *g);
static void game_update_transition(game_s *g);
static void cam_update(game_s *g, cam_s *c);

void game_init(game_s *g)
{
        gfx_set_inverted(1);
        tex_put(TEXID_FONT_DEFAULT, tex_load("assets/font_mono_8.json"));
        tex_put(TEXID_TILESET, tex_load("assets/tilesets.json"));
        fnt_put(FNTID_DEFAULT, fnt_load("assets/fnt/font1.json"));
        g->cam.w  = 400;
        g->cam.h  = 240;
        g->cam.wh = g->cam.w / 2;
        g->cam.hh = g->cam.h / 2;

        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ACTOR];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_ACTOR);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_SOLID];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_SOLID);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }

        game_load_map(g, "assets/map/template.tmj");

        obj_s     *solid = obj_create(g);
        objflags_s flags = objflags_create(OBJ_FLAG_SOLID);
        obj_set_flags(g, solid, flags);
        solid->pos.x = 0;
        solid->pos.y = 100;
        solid->w     = 64;
        solid->h     = 32;
}

void game_update(game_s *g)
{
        if (debug_inp_enter() && g->transitionphase == 0) {
                g->transitionphase = TRANSITION_FADE_IN;
                g->transitionticks = 0;
        }
        if (g->transitionphase) {
                game_update_transition(g);
        }

        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        if (solids.n > 0) {
                static int dir   = 1;
                obj_s     *solid = solids.o[0];
                if (solid->pos.x > 300) {
                        dir = -1;
                }
                if (solid->pos.x < 5) {
                        dir = +1;
                }
                solid_move(g, solid, dir * 2, 0);
        }

        obj_s *o;
        if (try_obj_from_handle(g->hero.obj, &o)) {
                g->hero.inpp = g->hero.inp;
                g->hero.inp  = 0;
                hero_update(g, o, &g->hero);
        }

        // remove all objects scheduled to be deleted
        if (objset_len(&g->obj_scheduled_delete) > 0) {
                game_cull_scheduled(g);
        }

        cam_update(g, &g->cam);
}

void game_close(game_s *g)
{
}

static void game_update_transition(game_s *g)
{
        g->transitionticks++;
        if (g->transitionticks < TRANSITION_TICKS)
                return;

        switch (g->transitionphase) {
        case TRANSITION_FADE_IN:
                game_load_map(g, g->transitionmap);
                g->transitionphase = TRANSITION_FADE_OUT;
                g->transitionticks = 0;
                break;
        case TRANSITION_FADE_OUT:
                g->transitionphase = TRANSITION_NONE;
                break;
        }
}

static void game_cull_scheduled(game_s *g)
{
        for (int n = 0; n < objset_len(&g->obj_scheduled_delete); n++) {
                obj_s *o_del = objset_at(&g->obj_scheduled_delete, n);
                objset_del(&g->obj_active, o_del);
                for (int i = 0; i < NUM_OBJ_BUCKETS; i++) {
                        objset_del(&g->objbuckets[i].set, o_del);
                }
                o_del->gen++; // invalidate existing handles
                g->objfreestack[g->n_objfree++] = o_del;
        }
        objset_clr(&g->obj_scheduled_delete);
}

enum {
        CAM_TARGET_SNAP_THRESHOLD = 1,
        CAM_LERP_DEN              = 8,
};

static void cam_update(game_s *g, cam_s *c)
{
        obj_s *player;
        if (try_obj_from_handle(g->hero.obj, &player)) {
                v2_i32 target = obj_aabb_center(player);
                target.y -= c->h / 8; // offset camera slightly upwards
                c->target = target;
        }

        v2_i32 dt  = v2_sub(c->target, c->pos);
        i32    lsq = v2_lensq(dt);
        if (lsq <= CAM_TARGET_SNAP_THRESHOLD) {
                c->pos = c->target;
        } else {
                c->pos = v2_lerp(c->pos, c->target, 1, CAM_LERP_DEN);
        }

        int x1 = c->pos.x - c->wh;
        int y1 = c->pos.y - c->hh;
        if (x1 < 0) {
                c->pos.x = c->wh;
        }
        if (y1 < 0) {
                c->pos.y = c->hh;
        }

        // avoids round errors on uneven camera sizes
        int x2 = (c->pos.x - c->wh) + c->w;
        int y2 = (c->pos.y - c->hh) + c->h;
        if (x2 > g->pixel_x) {
                c->pos.x = g->pixel_x - c->w + c->wh;
        }
        if (y2 > g->pixel_y) {
                c->pos.y = g->pixel_y - c->h + c->hh;
        }
}

tilegrid_s game_tilegrid(game_s *g)
{
        tilegrid_s tg = {g->tiles,
                         g->tiles_x, g->tiles_y,
                         g->pixel_x, g->pixel_y};
        return tg;
}

bool32 game_area_blocked(game_s *g, rec_i32 r)
{
        if (tiles_area(game_tilegrid(g), r)) return 1;
        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int n = 0; n < solids.n; n++) {
                obj_s *o = solids.o[n];
                if (overlap_rec_excl(obj_aabb(o), r))
                        return 1;
        }
        return 0;
}

obj_listc_s objbucket_list(game_s *g, int bucketID)
{
        ASSERT(0 <= bucketID && bucketID < NUM_OBJ_BUCKETS);
        return objset_list(&g->objbuckets[bucketID].set);
}