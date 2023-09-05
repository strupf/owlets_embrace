// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "autotiling.h"
#include "game.h"
#include "util/tmj.h"

static void load_rendertile_layer(game_s *g, jsn_s jlayer, int w, int h, int layerID);
static void load_obj_from_jsn(game_s *g, jsn_s jobj);
static void load_obj_layer(game_s *g, jsn_s jlayer);

void game_load_map(game_s *g, const char *filename)
{
        // reset room
        for (int n = 1; n < NUM_OBJS; n++) { // obj at index 0 is "dead"
                obj_s *o               = &g->objs[n];
                o->index               = n;
                o->gen                 = 1;
                g->objfreestack[n - 1] = o;
        }
        g->n_objfree = NUM_OBJS - 1;
        objset_clr(&g->obj_scheduled_delete);
        for (int n = 0; n < NUM_OBJ_BUCKETS; n++) {
                objset_clr(&g->objbuckets[n].set);
        }
        os_memclr(g->tiles, sizeof(g->tiles));
        memheap_init(&g->heap, g->heapmem, GAME_HEAPMEM);

        os_spmem_push();

        const char *tmjbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s       jroot, jtileset;
        jsn_root(tmjbuf, &jroot);

        int w = jsn_intk(jroot, "width");
        int h = jsn_intk(jroot, "height");

        ASSERT(w * h <= NUM_TILES);

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

        g->tiles_x                       = w;
        g->tiles_y                       = h;
        g->pixel_x                       = g->tiles_x << 4;
        g->pixel_y                       = g->tiles_y << 4;
        g->backforeground.clouddirection = 1;
        g->water.particles               = g->wparticles;
        g->water.nparticles              = 256;
        g->water.dampening_q12           = 4060;
        g->water.fneighbour_q16          = 2000;
        g->water.fzero_q16               = 100;
        g->water.loops                   = 3;
        g->water.p                       = (v2_i32){0, 50};

#if 1
        static int once = 0;
        if (!once) {
                obj_s     *solid1  = obj_create(g);
                objflags_s flagss1 = objflags_create(OBJ_FLAG_SOLID,
                                                     OBJ_FLAG_MOVABLE,
                                                     OBJ_FLAG_THINK_1);
                obj_apply_flags(g, solid1, flagss1);
                solid1->think_1 = solid_think;
                solid1->pos.x   = 170 + 300 - 20 + 5 * 16;
                solid1->pos.y   = 100 + 30;
                solid1->w       = 64;
                solid1->h       = 48;
                solid1->dir     = 1;
                solid1->p2      = 220 + 300 - 20 + 5 * 16;
                solid1->p1      = 130 + 300 - 20 + 5 * 16;
                solid1->ID      = 1;
                once            = 1;
        }

#endif

        obj_s *ohero = hero_create(g, &g->hero);
        ohero->pos.x = 50;
        ohero->pos.y = 100;

        g->cam.pos = obj_aabb_center(ohero);
        cam_constrain_to_room(g, &g->cam);

        textbox_init(&g->textbox);
        static int loadedonce = 1;
        if (!loadedonce) {
                loadedonce = 1;
                jsn_s prop;
                if (tmj_property(jroot, "introtext", &prop)) {
                        char filename[64] = {0};
                        jsn_strk(prop, "value", filename, sizeof(filename));
                        textbox_load_dialog(&g->textbox, filename);
                }
                textbox_load_dialog(&g->textbox, "assets/introtext.txt");
        }

        os_strcpy(g->areaname, "samplename");
        g->areaname_display_ticks = AREA_NAME_DISPLAY_TICKS;
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
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_NEW_AREA_COLLIDER);
                obj_apply_flags(g, o, flags);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
                o->w     = jsn_intk(jobj, "width");
                o->h     = jsn_intk(jobj, "height");
        } else if (streq(buf, "camattractor")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_CAM_ATTRACTOR);
                obj_apply_flags(g, o, flags);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "sign")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_INTERACT);
                obj_apply_flags(g, o, flags);
                o->pos.x      = jsn_intk(jobj, "x");
                o->pos.y      = jsn_intk(jobj, "y");
                o->w          = jsn_intk(jobj, "width");
                o->h          = jsn_intk(jobj, "height");
                o->oninteract = obj_interact_dialog;
                o->ID         = 6;
        }

        if (!o) return;

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

                g->rtiles[n].ID[layerID] = TILEID_NULL;
                if (tileID == 0) continue;

                if (is_autotile(tileID)) {
                        autotile_calc(g, tileIDs, w, h, x, y, n, layerID);
                } else {
                        u32 tID = (tileID & 0xFFFFFFFU) - TMJ_TILESET_FGID;

                        g->rtiles[n].ID[layerID] = tID;
                        if (layerID != TILE_LAYER_MAIN) continue;

                        if (tID == tileID_encode_ts(4, 0, 0)) {
                                g->tiles[n] = TILE_LADDER;
                        }
                }
        }

        os_spmem_pop();
}
