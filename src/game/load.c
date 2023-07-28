/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "game.h"

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
                jsn_strk(jset, "name", &ts.name, sizeof(ts.name));
                jsn_strk(jset, "image", &ts.image, sizeof(ts.image));
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
        for (int n = 0; n < N; n++) {
                u32    tileID    = tileIDs[n];
                u32    tileID_nf = (tileID & 0x0FFFFFFFu);
                bool32 flipx     = (tileID & 0x80000000u) > 0;
                bool32 flipy     = (tileID & 0x40000000u) > 0;
                bool32 flipz     = (tileID & 0x20000000u) > 0;

                int           n_tileset = -1;
                tmj_tileset_s tileset   = {0};
                for (int n = n_tilesets - 1; n >= 0; n--) {
                        if (tilesets[n].first_gid <= tileID_nf) {
                                tileset   = tilesets[n];
                                n_tileset = n;
                                break;
                        }
                }

                u8      t  = 0;
                rtile_s rt = {0};
                if (tileID == 0) {
                        rt.flags = 0xFF;
                } else {
                        rt.ID    = tileID_nf - 1024;
                        rt.flags = (flipz << 2) | (flipy << 1) | flipx;
                        t        = (tileID_nf - 1024) - 1;
                }

                g->rtiles[n][layer] = rt;
                g->tiles[n]         = t;
        }

        os_spmem_pop();
}

void game_load_map(game_s *g, const char *filename)
{
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
                load_rendertile_layer(g, jlayer, w, h,
                                      tilesets.sets, tilesets.n);
        }
        PRINTF("\n");

        os_spmem_pop();

        g->tiles_x = w;
        g->tiles_y = h;
        g->pixel_x = g->tiles_x << 4;
        g->pixel_y = g->tiles_y << 4;
}