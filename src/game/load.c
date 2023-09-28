// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "util/tmj.h"

bool32 is_autotile(u32 tileID);
void   autotile_calc(game_s *g, u32 *arr, int w, int h, int x, int y, int n, int layerID);

static void game_reset_for_load(game_s *g);
static void load_rendertile_layer(game_s *g, jsn_s jlayer, int w, int h, int layerID);
static void load_obj_from_jsn(game_s *g, jsn_s jobj);
static void load_obj_layer(game_s *g, jsn_s jlayer);

static void game_reset_for_load(game_s *g)
{
        for (int n = 1; n < NUM_OBJS; n++) { // obj at index 0 is "dead"
                obj_generic_s *og      = &g->objs[n];
                obj_s         *o       = (obj_s *)og;
                og->magic              = MAGIC_NUM_OBJ_2;
                o->magic               = MAGIC_NUM_OBJ_1;
                o->index               = n;
                o->gen                 = 1;
                g->objfreestack[n - 1] = o;
        }

        for (int n = 0; n < NUM_OBJ_TAGS; n++) {
                g->obj_tag[n] = NULL;
        }

        g->n_objfree = NUM_OBJS - 1;
        objset_clr(&g->obj_scheduled_delete);
        for (int n = 0; n < NUM_OBJ_BUCKETS; n++) {
                objset_clr(&g->objbuckets[n].set);
        }
        os_memclr(g->tiles, sizeof(g->tiles));
        memheap_init(&g->heap, g->heapmem, GAME_HEAPMEM);

        g->backforeground.n_clouds        = 0;
        g->backforeground.n_windparticles = 0;
        g->backforeground.n_particles     = 0;
}

void game_load_map(game_s *g, const char *filename)
{
        game_reset_for_load(g);
        os_strcpy(g->area_filename, filename);

        os_spmem_push();
        const char *tmjbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s       jroot, jtileset;
        jsn_root(tmjbuf, &jroot);

        int w = jsn_intk(jroot, "width");
        int h = jsn_intk(jroot, "height");
        ASSERT(w * h <= NUM_TILES);
        os_memset4(g->rtiles, 0xFF, sizeof(u32) * w * h);

        bool32 has_tilesets = jsn_key(jroot, "tilesets", &jtileset);
        ASSERT(has_tilesets);

        foreach_jsn_childk (jroot, "layers", jlayer) {
                char name[64] = {0};
                jsn_strk(jlayer, "name", name, ARRLEN(name));
                PRINTF("LOAD LAYER %s\n", name);
                if (0) {
                } else if (streq(name, "autotile")) {
                        load_rendertile_layer(g, jlayer, w, h, TILE_LAYER_MAIN);
                } else if (streq(name, "deco")) {
                        load_rendertile_layer(g, jlayer, w, h, TILE_LAYER_BG);
                } else if (streq(name, "obj")) {
                        load_obj_layer(g, jlayer);
                }
        }
        PRINTF("\n");

        os_spmem_pop();

        g->tiles_x = w;
        g->tiles_y = h;
        g->pixel_x = g->tiles_x << 4;
        g->pixel_y = g->tiles_y << 4;

        /*
        g->backforeground.clouddirection = 1;

        g->ocean.ocean.particles      = (waterparticle_s *)game_heapalloc(g, sizeof(waterparticle_s) * 1024);
        g->ocean.ocean.nparticles     = 1024;
        g->ocean.ocean.dampening_q12  = 4070;
        g->ocean.ocean.fneighbour_q16 = 8000;
        g->ocean.ocean.fzero_q16      = 20;
        g->ocean.ocean.loops          = 1;
        g->ocean.y                    = 48 * 16;

        g->ocean.water.particles      = (waterparticle_s *)game_heapalloc(g, sizeof(waterparticle_s) * 4096);
        g->ocean.water.nparticles     = 4096;
        g->ocean.water.dampening_q12  = 4060;
        g->ocean.water.fneighbour_q16 = 2000;
        g->ocean.water.fzero_q16      = 100;
        g->ocean.water.loops          = 3;
        g->ocean.water.p              = (v2_i32){0};

        g->water.particles      = (waterparticle_s *)game_heapalloc(g, sizeof(waterparticle_s) * 4096);
        g->water.nparticles     = 4096;
        g->water.dampening_q12  = 4060;
        g->water.fneighbour_q16 = 2000;
        g->water.fzero_q16      = 100;
        g->water.loops          = 3;
        g->water.p              = (v2_i32){0};
        */

        obj_s *ohero = hero_create(g);
        ohero->pos.x = 10 * 16;
        ohero->pos.y = 100;

        g->cam.pos = obj_aabb_center(ohero);
        cam_constrain_to_room(g, &g->cam);

        textbox_init(&g->textbox);

        os_strcpy(g->area_name, "Forgotten temple");
        g->area_name_ticks = AREA_NAME_DISPLAY_TICKS;
        backforeground_setup(g, &g->backforeground);

        // mus_play("assets/snd/background.wav");

        g->curr_world      = world_area_parent(filename);
        g->curr_world_area = world_area_by_filename(filename);

        ASSERT(g->curr_world && g->curr_world_area);
}

static void load_obj_from_jsn(game_s *g, jsn_s jobj)
{
        char buf[64];
        jsn_strk(jobj, "name", buf, sizeof(buf));

        obj_s *o = NULL;
        if (0) {
        } else if (streq(buf, "hero")) {
                // o        = hero_create(g, &g->hero);
                // o->pos.x = jsn_intk(jobj, "x");
                // o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "newmap")) {
                o = obj_create(g);
                obj_apply_flags(g, o, OBJ_FLAG_NEW_AREA_COLLIDER);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
                o->w     = jsn_intk(jobj, "width");
                o->h     = jsn_intk(jobj, "height");
        } else if (streq(buf, "camattractor")) {
                o = obj_create(g);
                obj_apply_flags(g, o, OBJ_FLAG_CAM_ATTRACTOR);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "sign")) {
                o        = sign_create(g);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
                o->w     = jsn_intk(jobj, "width");
                o->h     = jsn_intk(jobj, "height");
        } else if (streq(buf, "crumble")) {
                o        = crumbleblock_create(g);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "savepoint")) {
                o        = savepoint_create(g);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        }

        if (!o) return;

        o->tiledID = jsn_intk(jobj, "id");

        jsn_s prop;
        if (tmj_property(jobj, "dialog", &prop) ||
            tmj_property(jobj, "mapfile", &prop)) {
                jsn_strk(prop, "value", o->filename, sizeof(o->filename));
        }
}

static void load_obj_layer(game_s *g, jsn_s jlayer)
{
        foreach_jsn_childk (jlayer, "objects", jobj) {
                load_obj_from_jsn(g, jobj);
        }
}

// Tiled tile IDs encode the tileID, the tileset and possible flipping flags.
// See: doc.mapeditor.org/en/stable/reference/global-tile-ids/
static void load_rendertile_layer(game_s *g, jsn_s jlayer, int w, int h, int layerID)
{
        os_spmem_push();
        u32 *tileIDs = tmj_tileID_array_from_tmj(jlayer, os_spmem_alloc);
        foreach_tile_in_bounds(0, 0, w - 1, h - 1, x, y)
        {
                int n      = x + y * w;
                u32 tileID = tileIDs[n];
                if (tileID == 0) continue;

                if (is_autotile(tileID)) {
                        autotile_calc(g, tileIDs, w, h, x, y, n, layerID);
                } else {
                        u32 tID = (tileID & 0xFFFFFFFU) - TMJ_TILESET_FGID;

                        g->rtiles[n] = rtile_set(g->rtiles[n], tID, layerID);
                        if (layerID != TILE_LAYER_MAIN) continue;

                        if (tID == tileID_encode_ts(4, 0, 0)) {
                                g->tiles[n] = TILE_LADDER;
                        }
                        if (tID == tileID_encode_ts(4, 1, 3)) {
                                g->tiles[n] = TILE_SPIKES;
                        }
                }
        }

        os_spmem_pop();
}
