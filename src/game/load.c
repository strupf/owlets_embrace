// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "tab_autotiles.h"
#include "util/tmj.h"

enum {
        TMJ_TILESET_FGID = 1025,
};

enum {
        TILESHAPE_BLOCK,
        TILESHAPE_SLOPE_45,
        TILESHAPE_SLOPE_LO,
        TILESHAPE_SLOPE_HI,
};

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html
typedef struct {
        u32 *arr;
        int  w;
        int  h;
        int  x;
        int  y;
} autotiling_s;

static inline bool32 is_autotile(u32 ID);

static void autotile_calc(game_s *g, autotiling_s tiling, int n);
static void load_rendertile_layer(game_s *g, jsn_s jlayer, int w, int h);
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
                if (0) {
                } else if (streq(name, "autotile")) {
                        PRINTF("LOAD LAYER %s\n", name);
                        load_rendertile_layer(g, jlayer, w, h);
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
                obj_set_flags(g, solid1, flagss1);
                solid1->think_1 = solid_think;
                solid1->pos.x   = 170 + 300 - 20 + 5 * 16;
                solid1->pos.y   = 100 - 20;
                solid1->w       = 64;
                solid1->h       = 48;
                solid1->dir     = 1;
                solid1->p2      = 220 + 300 - 20 + 5 * 16;
                solid1->p1      = 130 + 300 - 20 + 5 * 16;
                solid1->ID      = 1;
                once            = 1;

                /*
                obj_s     *osign   = obj_create(g);
                objflags_s flagss2 = objflags_create(OBJ_FLAG_INTERACT);
                obj_set_flags(g, osign, flagss2);
                osign->pos.x      = 240 + 5 * 16;
                osign->pos.y      = 160;
                osign->w          = 16;
                osign->h          = 24;
                osign->ID         = 5;
                osign->oninteract = obj_interact_dialog;
                os_strcat(osign->filename, "assets/introtext.txt");
                */
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
                obj_set_flags(g, o, flags);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
                o->w     = jsn_intk(jobj, "width");
                o->h     = jsn_intk(jobj, "height");
        } else if (streq(buf, "camattractor")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_CAM_ATTRACTOR);
                obj_set_flags(g, o, flags);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "sign")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_INTERACT);
                obj_set_flags(g, o, flags);
                o->pos.x      = jsn_intk(jobj, "x");
                o->pos.y      = jsn_intk(jobj, "y");
                o->w          = jsn_intk(jobj, "width");
                o->h          = jsn_intk(jobj, "height");
                o->oninteract = obj_interact_dialog;
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

static inline bool32 is_autotile(u32 ID)
{
        int t = (ID & 0xFFFFFFFu);
        return (0 < t && t < TMJ_TILESET_FGID);
}

static bool32 autotile_fits(autotiling_s tiling, int sx, int sy, int ttype)
{
        int u = tiling.x + sx;
        int v = tiling.y + sy;

        // tiles on the edge of a room always have a neighbour
        if (!(0 <= u && u < tiling.w && 0 <= v && v < tiling.h)) return 1;

        u32 tileID = tiling.arr[u + v * tiling.w];
        if (tileID == 0) return 0;
        if (!is_autotile(tileID)) return 0;
        u32 tileID_nf   = (tileID & 0xFFFFFFFu) - 1;
        int terraintype = (tileID_nf / 16);
        int tileshape   = (tileID_nf % 16);
        // if (ttype != terraintype) return 0;
        int flags       = tmj_decode_flipping_flags(tileID);

        // combine sx and sy into a lookup index
        // ((sx + 1) << 2) | (sy + 1)
        static const int adjtab[] = {
            0x1, // sx = -1, sy = -1
            0x5, // sx = -1, sy =  0
            0x4, // sx = -1, sy = +1
            0,
            0x3, // sx =  0, sy = -1
            0,   // sx =  0, sy =  0
            0xC, // sx =  0, sy = +1
            0,
            0x2, // sx = +1, sy = -1
            0xA, // sx = +1, sy =  0
            0x8, // sx = +1, sy = +1
        };

        // bitmask indicating if a shape is considered adjacent
        // eg. slope going upwards: /| 01 = 0111
        //                         /_| 11
        // index: (shape << 3) | (flipping flags - xyz = 3 variants)
        static const int adjacency[] = {
            0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, // block
            0x7, 0xD, 0xB, 0xE, 0x7, 0xD, 0xB, 0xE, // slope 45
            0x3, 0xC, 0x3, 0xC, 0x5, 0x5, 0xA, 0xA, // slope LO
            0x7, 0xD, 0xB, 0xE, 0x7, 0xD, 0xB, 0xE  // slope HI
        };

        int i = (tileshape << 3) | flags;
        int l = ((sx + 1) << 2) | ((sy + 1));
        return (adjacency[i] & adjtab[l]) == adjtab[l];
}

/* calc bitmask according to neighbours:
 * 128| 1|2
 * --------
 *  64|  |4
 * --------
 *  32|16|8
 */
static void autotile_calc(game_s *g, autotiling_s tiling, int n)
{
        static u32 rngseed = 145;

        u32  tileID    = tiling.arr[n];
        u32  t         = (tileID & 0xFFFFFFFu) - 1;
        int  ttype     = (t / 16);
        int  tileshape = (t % 16);
        uint flags     = tmj_decode_flipping_flags(tileID);
        int  ytype     = ttype * 8;

        switch (tileshape) {
        case TILESHAPE_BLOCK: {
                g->tiles[n] = TILE_BLOCK;
                int m       = 0;
                // edges
                if (autotile_fits(tiling, -1, +0, ttype)) m |= 0x40; // left
                if (autotile_fits(tiling, +1, +0, ttype)) m |= 0x04; // right
                if (autotile_fits(tiling, +0, -1, ttype)) m |= 0x01; // top
                if (autotile_fits(tiling, +0, +1, ttype)) m |= 0x10; // down

                // corners only if there are the two corresponding edge neighbours
                if ((m & 0x41) == 0x41 && autotile_fits(tiling, -1, -1, ttype))
                        m |= 0x80; // top left
                if ((m & 0x50) == 0x50 && autotile_fits(tiling, -1, +1, ttype))
                        m |= 0x20; // bot left
                if ((m & 0x05) == 0x05 && autotile_fits(tiling, +1, -1, ttype))
                        m |= 0x02; // top right
                if ((m & 0x14) == 0x14 && autotile_fits(tiling, +1, +1, ttype))
                        m |= 0x08; // bot right

                // g->rtiles[n][0].m  = m;
                g->rtiles[n].ID = TILEID_NULL;

                // some tiles have variants
                // if one of those is matched choose a random variant
                int tx = 0, ty = 0;

                switch (m) {
                case 17: {
                        int k = rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 31: {
                        int k = 3 + rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 68: {
                        int k = 6 + rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 124: {
                        int k = 9 + rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 199: {
                        int k = 12 + rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 241: {
                        int k = 15 + rngs_max_u32(&rngseed, 2);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 255: {
                        int k = 18 + rngs_max_u32(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                default:
                        tx = blobpattern[m * 2];
                        ty = blobpattern[m * 2 + 1];
                        break;
                }
                g->rtiles[n].ID = tileID_encode(tx, ty + ytype);

        } break;
        case TILESHAPE_SLOPE_45: {
                flags %= 4; // last 4 transformations are the same
                g->tiles[n] = TILE_SLOPE_45 + flags;
                bool32 xn = 0, yn = 0, cn = 0; // neighbour in x, y and corner

                switch (flags) {
                case 0: {
                        xn = autotile_fits(tiling, +1, +0, ttype);
                        yn = autotile_fits(tiling, +0, +1, ttype);
                        if (xn && yn)
                                cn = autotile_fits(tiling, +1, +1, ttype);
                } break;
                case 1: {
                        xn = autotile_fits(tiling, +1, +0, ttype);
                        yn = autotile_fits(tiling, +0, -1, ttype);
                        if (xn && yn)
                                cn = autotile_fits(tiling, +1, -1, ttype);
                } break;
                case 2: {
                        xn = autotile_fits(tiling, -1, +0, ttype);
                        yn = autotile_fits(tiling, +0, +1, ttype);
                        if (xn && yn)
                                cn = autotile_fits(tiling, -1, +1, ttype);
                } break;
                case 3: {
                        xn = autotile_fits(tiling, -1, +0, ttype);
                        yn = autotile_fits(tiling, +0, -1, ttype);
                        if (xn && yn)
                                cn = autotile_fits(tiling, -1, -1, ttype);
                } break;
                }

                // choose appropiate variant
                int z           = 4 - (cn ? 4 : ((yn > 0) << 1) | (xn > 0));
                int tx          = 18 + z;
                int ty          = 0 + flags;
                g->rtiles[n].ID = tileID_encode(tx, ty + ytype);
        } break;
        case TILESHAPE_SLOPE_LO: {
                g->tiles[n] = TILE_SLOPE_LO + flags;
                NOT_IMPLEMENTED
        } break;
        case TILESHAPE_SLOPE_HI: {
                g->tiles[n] = TILE_SLOPE_HI + flags;
                NOT_IMPLEMENTED
        } break;
        }
}

// Tiled tile IDs encode the tileID, the tileset and possible flipping flags.
// See: doc.mapeditor.org/en/stable/reference/global-tile-ids/
static void load_rendertile_layer(game_s *g, jsn_s jlayer, int w, int h)
{
        os_spmem_push();
        u32 *tileIDs = tmj_tileID_array_from_tmj(jlayer,
                                                 os_spmem_alloc);
        int  layer   = 0;
        for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                        int n      = x + y * w;
                        u32 tileID = tileIDs[n];

                        g->tiles[n]      = 0;
                        g->rtiles[n].ID  = TILEID_NULL;
                        g->rtiles[n].ID2 = TILEID_NULL;
                        if (tileID == 0) continue;

                        if (is_autotile(tileID)) {
                                autotiling_s tiling = {tileIDs,
                                                       w, h, x, y};
                                autotile_calc(g, tiling, n);
                                continue;
                        }

                        u32 tileID_nf = (tileID & 0xFFFFFFFu) -
                                        (TMJ_TILESET_FGID);
                        g->rtiles[n].ID = tileID_nf;
                }
        }

        os_spmem_pop();
}
