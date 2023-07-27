/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "game.h"

void load_rendertile_layer(game_s *g, jsn_s jlayer)
{
        const int layer = 0;
        int       tID   = 0;
        foreach_jsn_childk (jlayer, "data", jtile) {
                u32     tileID = jsn_uint(jtile);
                bool32  flipx  = ((tileID & 0x80000000u) > 0);
                bool32  flipy  = ((tileID & 0x40000000u) > 0);
                bool32  flipz  = ((tileID & 0x20000000u) > 0);
                u8      t      = 0;
                rtile_s rt     = {0};
                if (tileID == 0) {
                        rt.flags = 0xFF;
                } else {
                        rt.ID    = (tileID & 0x0FFFFFFFu) - 1024;
                        rt.flags = (flipz << 2) | (flipy << 1) | flipx;
                        t        = ((tileID & 0x0FFFFFFFu) - 1024) - 1;
                }

                g->rtiles[tID][layer] = rt;
                g->tiles[tID]         = t;
                tID++;
        }
}

void game_load_map(game_s *g, const char *filename)
{
        os_spmem_push();
        const char *tmjbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s       jroot;
        jsn_root(tmjbuf, &jroot);

        g->tiles_x = jsn_intk(jroot, "width");
        g->tiles_y = jsn_intk(jroot, "height");
        ASSERT(g->tiles_x * g->tiles_y <= NUM_TILES);

        foreach_jsn_childk (jroot, "layers", jlayer) {
                load_rendertile_layer(g, jlayer);
        }
        PRINTF("\n");
        os_spmem_pop();

        g->pixel_x = g->tiles_x << 4;
        g->pixel_y = g->tiles_y << 4;
}