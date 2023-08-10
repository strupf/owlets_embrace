// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u16 g_tileIDs[0x10000];

typedef struct {
        int ID;
        int frames;
        int cur;
        u16 IDs[4];
} tile_animation_s;

static int       n_tileanimations;
tile_animation_s g_tileanimations[16];

void tileanimations_update()
{
        for (int n = 0; n < n_tileanimations; n++) {
                tile_animation_s *a = &g_tileanimations[n];
                g_tileIDs[a->ID]    = a->IDs[a->cur];
        }
}

static void game_cull_scheduled(game_s *g);
static void game_update_transition(game_s *g);
static void cam_update(game_s *g, cam_s *c);

void game_init(game_s *g)
{
        for (int n = 0; n < ARRLEN(g_tileIDs); n++) {
                g_tileIDs[n] = n;
        }

        gfx_set_inverted(1);

        tex_put(TEXID_FONT_DEFAULT, tex_load("assets/font_mono_8.json"));
        tex_put(TEXID_FONT_DEBUG, tex_load("assets/font_debug.json"));
        tex_put(TEXID_TILESET, tex_load("assets/tilesets.json"));
        tex_put(TEXID_TEXTBOX, tex_load("assets/textbox.json"));
        tex_put(TEXID_ITEMS, tex_load("assets/items.json"));
        tex_put(TEXID_TEST, tex_load("assets/test.json"));
        tex_put(TEXID_PARTICLE, tex_load("assets/particle.json"));
        tex_put(TEXID_TESTSPRITE, tex_load("assets/testsprite.json"));

        tex_s tclouds = tex_load("assets/clouds.json");
        tex_put(TEXID_CLOUDS, tclouds);

        // apply some "grey pattern"
        for (int y = 0; y < tclouds.h; y++) {
                for (int x = 0; x < tclouds.w; x++) {
                        if (((x + y) % 2 == 0) || x % 2 == 0 || y % 4 == 0) {
                                int i = (x >> 3) + y * tclouds.w_byte;
                                int b = (x & 7);
                                tclouds.mask[i] &= ~(1 << (7 - b)); // clear bit
                        }
                }
        }

        fnt_put(FNTID_DEFAULT, fnt_load("assets/fnt/font_default.json"));
        fnt_put(FNTID_DEBUG, fnt_load("assets/fnt/font_debug.json"));

        g->rng    = 213;
        g->cam.w  = 400;
        g->cam.h  = 240;
        g->cam.wh = g->cam.w / 2;
        g->cam.hh = g->cam.h / 2;

        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ALIVE];
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
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
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_NEW_AREA_COLLIDER];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_NEW_AREA_COLLIDER);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_PICKUP];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_PICKUP);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_INTERACT];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_INTERACT);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }

        game_load_map(g, "assets/map/template.tmj");
}

void game_update(game_s *g)
{
        textbox_s *tb = &g->textbox;
        if (tb->active) {
                textbox_update(tb);
                if (os_inp_just_pressed(INP_A) && g->textbox.shows_all) {
                        textbox_next_page(tb);
                }
                return;
        }

        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        float       times  = os_time();
        for (int i = 0; i < solids.n; i++) {
                obj_s *solid = solids.o[i];
                if (solid->pos.x > solid->p2) {
                        solid->dir = -ABS(solid->dir);
                }
                if (solid->pos.x < solid->p1) {
                        solid->dir = +ABS(solid->dir);
                }

                solid_move(g, solid, solid->dir, 0);
        }
        os_debug_time(TIMING_SOLID_UPDATE, os_time() - times);

        obj_s *ohero;
        float  timeh = os_time();
        if (try_obj_from_handle(g->hero.obj, &ohero)) {

                g->hero.inpp = g->hero.inp;
                g->hero.inp  = 0;
                hero_update(g, ohero, &g->hero);
                if (g->transitionphase == TRANSITION_NONE) {
                        hero_check_level_transition(g, ohero);
                }
                hero_pickup_logic(g, &g->hero, ohero);
        }
        os_debug_time(TIMING_HERO_UPDATE, os_time() - timeh);

        // remove all objects scheduled to be deleted
        if (objset_len(&g->obj_scheduled_delete) > 0) {
                game_cull_scheduled(g);
        }

        cam_update(g, &g->cam);

        if (g->transitionphase) {
                game_update_transition(g);
        }

        for (int n = g->n_particles - 1; n >= 0; n--) {
                particle_s *p = &g->particles[n];
                p->ticks--;
                if (p->ticks == 0) {
                        g->particles[n] = g->particles[--g->n_particles];
                        continue;
                }
                p->v_q8 = v2_add(p->v_q8, p->a_q8);
                p->p_q8 = v2_add(p->p_q8, p->v_q8);
        }
}

void game_close(game_s *g)
{
}

void game_map_transition_start(game_s *g, const char *filename)
{
        if (g->transitionphase != 0) return;
        g->transitionphase = TRANSITION_FADE_IN;
        g->transitionticks = 0;

        os_strcpy(g->transitionmap, filename);
}

static void game_update_transition(game_s *g)
{
        g->transitionticks++;
        if (g->transitionticks < TRANSITION_TICKS)
                return;

        char filename[64] = {0};
        switch (g->transitionphase) {
        case TRANSITION_FADE_IN:

                os_strcat(filename, ASSET_PATH_MAPS);
                os_strcat(filename, g->transitionmap);
                game_load_map(g, filename);
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
                for (int i = 0; i < NUM_OBJ_BUCKETS; i++) {
                        objset_del(&g->objbuckets[i].set, o_del);
                }
                o_del->gen++; // invalidate existing handles
                g->objfreestack[g->n_objfree++] = o_del;
        }
        objset_clr(&g->obj_scheduled_delete);
}

enum cam_values {
        CAM_TARGET_SNAP_THRESHOLD = 1,
        CAM_LERP_DISTANCESQ_FAST  = 1000,
        CAM_LERP_DEN              = 8,
        CAM_LERP_DEN_FAST         = 4,
};

static void cam_update(game_s *g, cam_s *c)
{
        obj_s *player;
        bool32 offset_modified = 0;
        if (try_obj_from_handle(g->hero.obj, &player)) {
                v2_i32 targ = obj_aabb_center(player);
                targ.y -= c->h >> 3; // offset camera slightly upwards
                c->target = targ;

                if (os_inp_dpad_y() == 1 &&
                    game_area_blocked(g, obj_rec_bottom(player)) &&
                    ABS(player->vel_q8.x) < 10) {
                        offset_modified = 1;
                        c->offset.y += 5;
                        c->offset.y = MIN(c->offset.y, 100);
                }
        }

        v2_i32 target = v2_add(c->target, c->offset);
        if (!offset_modified) c->offset.y >>= 1;
        v2_i32 dt  = v2_sub(target, c->pos);
        i32    lsq = v2_lensq(dt);
        if (lsq <= CAM_TARGET_SNAP_THRESHOLD) {
                c->pos = target;
        } else if (lsq < CAM_LERP_DISTANCESQ_FAST) {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN);
        } else {
                c->pos = v2_lerp(c->pos, target, 1, CAM_LERP_DEN_FAST);
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
        tilegrid_s tg =
            {g->tiles, g->tiles_x, g->tiles_y, g->pixel_x, g->pixel_y};
        return tg;
}

bool32 solid_occupies(obj_s *solid, rec_i32 r)
{
        return overlap_rec_excl(obj_aabb(solid), r);
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

void game_tile_bounds_minmax(game_s *g, v2_i32 pmin, v2_i32 pmax,
                             i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        *x1 = MAX(pmin.x, 0) >> 4; /* "/ 16" */
        *y1 = MAX(pmin.y, 0) >> 4;
        *x2 = MIN(pmax.x, g->pixel_x - 1) >> 4;
        *y2 = MIN(pmax.y, g->pixel_y - 1) >> 4;
}

void game_tile_bounds_tri(game_s *g, tri_i32 t,
                          i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = v2_min(t.p[0], v2_min(t.p[1], t.p[2]));
        v2_i32 pmax = v2_max(t.p[0], v2_max(t.p[1], t.p[2]));
        game_tile_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}

void game_tile_bounds_rec(game_s *g, rec_i32 r,
                          i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = {r.x, r.y};
        v2_i32 pmax = {r.x + r.w, r.y + r.h};
        game_tile_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}

particle_s *particle_spawn(game_s *g)
{
        ASSERT(g->n_particles < ARRLEN(g->particles));

        particle_s *p = &g->particles[g->n_particles++];
        *p            = (const particle_s){0};
        return p;
}