// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// x and y coordinate IDs in blob pattern
// used to look up tile after marching squares (x, y, x, y, ...)
// x = index * 2, y = index * 2 + 1
// pattern according to:
// opengameart.org/content/seamless-tileset-template
// this pattern allows some duplicated border tiles to include
// more variety later on
static const u8 blobpattern[256 * 2];

static const u8 blob_duplicate[] = {
    0, 1, // 17 vertical
    0, 3, // 17
    0, 4, // 17
    1, 4, // 31 left
    1, 6, // 31
    5, 3, // 31
    1, 0, // 68 horizontal
    3, 0, // 68
    4, 0, // 68
    4, 1, // 124 top
    6, 1, // 124
    3, 5, // 124
    2, 7, // 199 bot
    3, 7, // 199
    4, 2, // 199
    2, 4, // 241 right
    7, 2, // 241
    7, 3, // 241
    2, 6, // 255 mid
    3, 6, // 255
    6, 2, // 255
    6, 3, // 255
};

#define TILESETID_COLLISION 1

enum {
        TILESHAPE_BLOCK,
        TILESHAPE_SLOPE_45,
        TILESHAPE_SLOPE_LO,
        TILESHAPE_SLOPE_HI,
};

typedef struct {
        char name[64];
        char image[64];
        u32  first_gid;
        i32  n_tiles;
        i32  columns;
        i32  img_w;
        i32  img_h;
} tmj_tileset_s;

typedef struct {
        tmj_tileset_s sets[16];
        int           n;
} tmj_tilesets_s;

/*
 * Set tiles automatically while loading.
 * www.cr31.co.uk/stagecast/wang/blob.html
 */
typedef struct {
        const u32 *arr;
        const int  w;
        const int  h;
        const int  x;
        const int  y;
} autotiling_s;

// extract Tiled internal flipping flags and map them into a different format
// doc.mapeditor.org/en/stable/reference/global-tile-ids/
int tiled_decode_flipping_flags(u32 tileID)
{
        bool32 flipx = (tileID & 0x80000000u) > 0;
        bool32 flipy = (tileID & 0x40000000u) > 0;
        bool32 flipz = (tileID & 0x20000000u) > 0; // diagonal flip
        return (flipz << 2) | (flipx << 1) | flipy;
}

bool32 autotile_fits(autotiling_s tiling, int sx, int sy, int ttype)
{
        int u = tiling.x + sx;
        int v = tiling.y + sy;

        // tiles on the edge of a room always have a neighbour
        if (!(0 <= u && u < tiling.w && 0 <= v && v < tiling.h)) return 1;

        u32 tileID = tiling.arr[u + v * tiling.w];
        if (tileID == 0) return 0;
        u32 tileID_nf   = (tileID & 0x0FFFFFFFu) - TILESETID_COLLISION;
        int flags       = tiled_decode_flipping_flags(tileID);
        int terraintype = (tileID_nf / 4);
        int tileshape   = (tileID_nf % 4);
        if (ttype != terraintype) return 0;

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

/*
 * calc bitmask according to neighbours:
 * 128| 1|2
 * --------
 *  64|  |4
 * --------
 *  32|16|8
 */
void autotile_calc(game_s *g, autotiling_s tiling, int n)
{
        static u32 rngseed = 145;

        u32 tileID = tiling.arr[n];
        if (tileID == 0) {
                g->rtiles[n][0].ID = TILEID_NULL;
                return;
        }
        u32  t         = (tileID & 0x0FFFFFFFu) - TILESETID_COLLISION;
        int  ttype     = (t / 4);
        int  tileshape = (t % 4);
        uint flags     = tiled_decode_flipping_flags(tileID);

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

                g->rtiles[n][0].m  = m;
                g->rtiles[n][0].ID = TILEID_NULL;

                // some tiles have variants
                // if one of those is matched choose a random variant
                int tx = 0, ty = 0;
                switch (m) {
                case 17: {
                        int k = rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 31: {
                        int k = 3 + rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 68: {
                        int k = 6 + rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 124: {
                        int k = 9 + rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 199: {
                        int k = 12 + rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 241: {
                        int k = 15 + rng_max_u16(&rngseed, 3);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                case 255: {
                        int k = 18 + rng_max_u16(&rngseed, 4);
                        tx    = blob_duplicate[k * 2];
                        ty    = blob_duplicate[k * 2 + 1];
                } break;
                default:
                        tx = blobpattern[m * 2];
                        ty = blobpattern[m * 2 + 1];
                        break;
                }
                g->rtiles[n][0].ID = tileID_encode(tx, ty);

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
                int z              = 4 - (cn ? 4 : ((yn > 0) << 1) | (xn > 0));
                int tx             = 10 + z;
                int ty             = 12 + flags;
                g->rtiles[n][0].ID = tileID_encode(tx, ty);
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

tmj_tilesets_s tilesets_parse(jsn_s jtilesets)
{
        tmj_tilesets_s sets = {0};
        foreach_jsn_child (jtilesets, jset) {
                ASSERT(sets.n < ARRLEN(sets.sets));

                tmj_tileset_s ts = {0};
                ts.first_gid     = jsn_intk(jset, "firstgid");
                ts.n_tiles       = jsn_intk(jset, "tilecount");
                ts.img_w         = jsn_intk(jset, "imagewidth");
                ts.img_h         = jsn_intk(jset, "imageheight");
                ts.columns       = jsn_intk(jset, "columns");
                jsn_strk(jset, "name", ts.name, sizeof(ts.name));
                jsn_strk(jset, "image", ts.image, sizeof(ts.image));
                sets.sets[sets.n++] = ts;
        }
        return sets;
}

static u32 *tileID_array_from_tmj(jsn_s jlayer, void *(*allocf)(size_t))
{
        int  N   = jsn_intk(jlayer, "width") * jsn_intk(jlayer, "height");
        u32 *IDs = (u32 *)allocf(sizeof(u32) * N);
        {
                int n = 0;
                foreach_jsn_childk (jlayer, "data", jtile) {
                        IDs[n++] = jsn_uint(jtile);
                }
        }
        return IDs;
}

// Tied tile IDs encode the tileID, the tileset and possible flipping flags.
//   See: doc.mapeditor.org/en/stable/reference/global-tile-ids/
void load_rendertile_layer(game_s *g, jsn_s jlayer,
                           int width, int height,
                           tmj_tileset_s *tilesets, int n_tilesets)
{
        os_spmem_push();
        const u32 *tileIDs = tileID_array_from_tmj(jlayer, os_spmem_alloc);
        const int  layer   = 0;
        const int  N       = width * height;
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        int n         = x + y * width;
                        u32 tileID    = tileIDs[n];
                        u32 tileID_nf = (tileID & 0x0FFFFFFFu);

                        int           n_tileset = -1;
                        tmj_tileset_s tileset   = {0};
                        for (int n = n_tilesets - 1; n >= 0; n--) {
                                if (tilesets[n].first_gid <= tileID_nf) {
                                        tileset   = tilesets[n];
                                        n_tileset = n;
                                        break;
                                }
                        }
                        ASSERT(tileID == 0 || n_tileset >= 0);

                        g->tiles[n]         = 0;
                        g->rtiles[n][0].ID  = TILEID_NULL;
                        g->rtiles[n][1].ID  = TILEID_NULL;
                        autotiling_s tiling = {tileIDs, width, height, x, y};
                        autotile_calc(g, tiling, n);
                }
        }

        os_spmem_pop();
}

static bool32 tiled_property(jsn_s j, const char *pname, jsn_s *property)
{
        jsn_s jj = {0};
        if (!jsn_key(j, "properties", &jj)) return 0;
        foreach_jsn_child (jj, jprop) {
                char buf[64];
                jsn_strk(jprop, "name", buf, sizeof(buf));
                if (streq(buf, pname)) {
                        *property = jprop;
                        return 1;
                }
        }
        return 0;
}

static void load_obj_from_jsn(game_s *g, jsn_s jobj)
{
        char buf[64];
        jsn_strk(jobj, "name", buf, sizeof(buf));

        obj_s *o = NULL;
        if (0) {
        } else if (streq(buf, "hero")) {
                o        = hero_create(g, &g->hero);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
        } else if (streq(buf, "sign")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_INTERACT);
                obj_set_flags(g, o, flags);
                o->pos.x      = jsn_intk(jobj, "x");
                o->pos.y      = jsn_intk(jobj, "y");
                o->w          = 16;
                o->h          = 16;
                o->oninteract = interact_open_dialogue;
        } else if (streq(buf, "newmap")) {
                o                = obj_create(g);
                objflags_s flags = objflags_create(
                    OBJ_FLAG_NEW_AREA_COLLIDER);
                obj_set_flags(g, o, flags);
                o->pos.x = jsn_intk(jobj, "x");
                o->pos.y = jsn_intk(jobj, "y");
                o->w     = jsn_intk(jobj, "width");
                o->h     = jsn_intk(jobj, "height");
        } else {
                return;
        }

        jsn_s prop;
        if (tiled_property(jobj, "dialogue", &prop)) {
                jsn_strk(prop, "value", o->filename, sizeof(o->filename));
        }
        if (tiled_property(jobj, "mapfile", &prop)) {
                jsn_strk(prop, "value", o->filename, sizeof(o->filename));
        }
}

void load_obj_layer(game_s *g, jsn_s jlayer)
{
        foreach_jsn_childk (jlayer, "objects", jobj) {
                load_obj_from_jsn(g, jobj);
        }
}

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

        os_spmem_push();

        const char *tmjbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s       jroot, jtileset;
        jsn_root(tmjbuf, &jroot);

        int w = jsn_intk(jroot, "width");
        int h = jsn_intk(jroot, "height");

        ASSERT(w * h <= NUM_TILES);

        bool32 has_tilesets = jsn_key(jroot, "tilesets", &jtileset);
        ASSERT(has_tilesets);

        tmj_tilesets_s tilesets = tilesets_parse(jtileset);
        for (int n = 0; n < tilesets.n; n++) {
                PRINTF("name: %s\n", tilesets.sets[n].name);
        }

        foreach_jsn_childk (jroot, "layers", jlayer) {
                char name[64] = {0};
                jsn_strk(jlayer, "name", name, ARRLEN(name));
                if (0) {
                } else if (streq(name, "autotile")) {
                        PRINTF("LOAD LAYER %s\n", name);
                        load_rendertile_layer(g, jlayer, w, h,
                                              tilesets.sets, tilesets.n);
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

#if 1
        obj_s     *solid1  = obj_create(g);
        objflags_s flagss1 = objflags_create(OBJ_FLAG_SOLID);
        obj_set_flags(g, solid1, flagss1);
        solid1->pos.x = 100;
        solid1->pos.y = 192 - 32;
        solid1->w     = 48;
        solid1->h     = 48;
        solid1->dir   = 1;
        solid1->p2    = 200;
        solid1->p1    = 50;

        obj_s *solid2 = obj_create(g);
        obj_set_flags(g, solid2, flagss1);
        solid2->pos.x = 800;
        solid2->pos.y = 300 - 32;
        solid2->w     = 64;
        solid2->h     = 16;
        solid2->dir   = 1;
        solid2->p2    = 800;
        solid2->p1    = 500;

        obj_s *solid3 = obj_create(g);
        obj_set_flags(g, solid3, flagss1);
        solid3->pos.x = 340;
        solid3->pos.y = 64 + 16;
        solid3->w     = 64;
        solid3->h     = 16;
        solid3->dir   = 1;
        solid3->p2    = 500;
        solid3->p1    = 300;
#endif

        for (int i = 0; i < 10; i++) {
                obj_s     *pickup = obj_create(g);
                objflags_s flags3 = objflags_create(OBJ_FLAG_PICKUP);
                obj_set_flags(g, pickup, flags3);
                pickup->pos.x    = 500 + i * 30;
                pickup->pos.y    = 340;
                pickup->w        = 8;
                pickup->h        = 8;
                pickup->pickup.x = 1;
        }

        textbox_init(&g->textbox);
        static int loadedonce = 1;
        if (!loadedonce) {
                loadedonce = 1;
                PRINTF("load intro\n");
                jsn_s prop;
                if (tiled_property(jroot, "introtext", &prop)) {
                        char filename[64] = {0};
                        jsn_strk(prop, "value", filename, sizeof(filename));
                        textbox_load_dialog(&g->textbox, filename);
                }
        }
        g->cam.pos.x = 400;
        g->cam.pos.y = 150;
}

static const u8 blobpattern[256 * 2] = {
    0, 6, // 0
    7, 6, // 1
    0, 0, // 2
    0, 0, // 3
    0, 7, // 4
    0, 5, // 5
    0, 0, // 6
    3, 4, // 7
    0, 0, // 8
    0, 0, // 9
    0, 0, // 10
    0, 0, // 11
    0, 0, // 12
    0, 0, // 13
    0, 0, // 14
    0, 0, // 15
    7, 0, // 16
    0, 1, // 17
    0, 0, // 18
    0, 0, // 19
    0, 0, // 20
    0, 2, // 21
    0, 0, // 22
    3, 2, // 23
    0, 0, // 24
    0, 0, // 25
    0, 0, // 26
    0, 0, // 27
    1, 1, // 28
    1, 3, // 29
    0, 0, // 30
    1, 4, // 31
    0, 0, // 32
    0, 0, // 33
    0, 0, // 34
    0, 0, // 35
    0, 0, // 36
    0, 0, // 37
    0, 0, // 38
    0, 0, // 39
    0, 0, // 40
    0, 0, // 41
    0, 0, // 42
    0, 0, // 43
    0, 0, // 44
    0, 0, // 45
    0, 0, // 46
    0, 0, // 47
    0, 0, // 48
    0, 0, // 49
    0, 0, // 50
    0, 0, // 51
    0, 0, // 52
    0, 0, // 53
    0, 0, // 54
    0, 0, // 55
    0, 0, // 56
    0, 0, // 57
    0, 0, // 58
    0, 0, // 59
    0, 0, // 60
    0, 0, // 61
    0, 0, // 62
    0, 0, // 63
    6, 7, // 64
    6, 6, // 65
    0, 0, // 66
    0, 0, // 67
    1, 0, // 68
    5, 7, // 69
    0, 0, // 70
    1, 7, // 71
    0, 0, // 72
    0, 0, // 73
    0, 0, // 74
    0, 0, // 75
    0, 0, // 76
    0, 0, // 77
    0, 0, // 78
    0, 0, // 79
    5, 0, // 80
    7, 5, // 81
    0, 0, // 82
    0, 0, // 83
    2, 0, // 84
    5, 6, // 85
    0, 0, // 86
    1, 2, // 87
    0, 0, // 88
    0, 0, // 89
    0, 0, // 90
    0, 0, // 91
    3, 1, // 92
    3, 3, // 93
    0, 0, // 94
    1, 5, // 95
    0, 0, // 96
    0, 0, // 97
    0, 0, // 98
    0, 0, // 99
    0, 0, // 100
    0, 0, // 101
    0, 0, // 102
    0, 0, // 103
    0, 0, // 104
    0, 0, // 105
    0, 0, // 106
    0, 0, // 107
    0, 0, // 108
    0, 0, // 109
    0, 0, // 110
    0, 0, // 111
    4, 3, // 112
    7, 1, // 113
    0, 0, // 114
    0, 0, // 115
    2, 3, // 116
    2, 1, // 117
    0, 0, // 118
    4, 5, // 119
    0, 0, // 120
    0, 0, // 121
    0, 0, // 122
    0, 0, // 123
    4, 1, // 124
    5, 1, // 125
    0, 0, // 126
    5, 4, // 127
    0, 0, // 128
    0, 0, // 129
    0, 0, // 130
    0, 0, // 131
    0, 0, // 132
    0, 0, // 133
    0, 0, // 134
    0, 0, // 135
    0, 0, // 136
    0, 0, // 137
    0, 0, // 138
    0, 0, // 139
    0, 0, // 140
    0, 0, // 141
    0, 0, // 142
    0, 0, // 143
    0, 0, // 144
    0, 0, // 145
    0, 0, // 146
    0, 0, // 147
    0, 0, // 148
    0, 0, // 149
    0, 0, // 150
    0, 0, // 151
    0, 0, // 152
    0, 0, // 153
    0, 0, // 154
    0, 0, // 155
    0, 0, // 156
    0, 0, // 157
    0, 0, // 158
    0, 0, // 159
    0, 0, // 160
    0, 0, // 161
    0, 0, // 162
    0, 0, // 163
    0, 0, // 164
    0, 0, // 165
    0, 0, // 166
    0, 0, // 167
    0, 0, // 168
    0, 0, // 169
    0, 0, // 170
    0, 0, // 171
    0, 0, // 172
    0, 0, // 173
    0, 0, // 174
    0, 0, // 175
    0, 0, // 176
    0, 0, // 177
    0, 0, // 178
    0, 0, // 179
    0, 0, // 180
    0, 0, // 181
    0, 0, // 182
    0, 0, // 183
    0, 0, // 184
    0, 0, // 185
    0, 0, // 186
    0, 0, // 187
    0, 0, // 188
    0, 0, // 189
    0, 0, // 190
    0, 0, // 191
    0, 0, // 192
    2, 2, // 193
    0, 0, // 194
    0, 0, // 195
    0, 0, // 196
    4, 7, // 197
    0, 0, // 198
    2, 7, // 199
    0, 0, // 200
    0, 0, // 201
    0, 0, // 202
    0, 0, // 203
    0, 0, // 204
    0, 0, // 205
    0, 0, // 206
    0, 0, // 207
    0, 0, // 208
    7, 4, // 209
    0, 0, // 210
    0, 0, // 211
    0, 0, // 212
    6, 5, // 213
    0, 0, // 214
    5, 5, // 215
    0, 0, // 216
    0, 0, // 217
    0, 0, // 218
    0, 0, // 219
    0, 0, // 220
    4, 4, // 221
    0, 0, // 222
    5, 2, // 223
    0, 0, // 224
    0, 0, // 225
    0, 0, // 226
    0, 0, // 227
    0, 0, // 228
    0, 0, // 229
    0, 0, // 230
    0, 0, // 231
    0, 0, // 232
    0, 0, // 233
    0, 0, // 234
    0, 0, // 235
    0, 0, // 236
    0, 0, // 237
    0, 0, // 238
    0, 0, // 239
    0, 0, // 240
    7, 2, // 241
    0, 0, // 242
    0, 0, // 243
    0, 0, // 244
    4, 6, // 245
    0, 0, // 246
    6, 4, // 247
    0, 0, // 248
    0, 0, // 249
    0, 0, // 250
    0, 0, // 251
    0, 0, // 252
    2, 5, // 253
    0, 0, // 254
    2, 6, // 255
};