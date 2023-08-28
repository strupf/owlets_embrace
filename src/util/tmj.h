// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os/os.h"

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

static tmj_tileset_s tmj_get_tileset(tmj_tilesets_s *sets, u32 ID)
{
        u32 tileID_nf = (ID & 0xFFFFFFFu);
        for (int n = sets->n - 1; n >= 0; n--) {
                if (sets->sets[n].first_gid <= tileID_nf)
                        return sets->sets[n];
        }
        ASSERT(0);
        tmj_tileset_s tileset = {0};
        return tileset;
}

// extract Tiled internal flipping flags and map them into a different format
// doc.mapeditor.org/en/stable/reference/global-tile-ids/
static int tmj_decode_flipping_flags(u32 tileID)
{
        bool32 flipx = (tileID & 0x80000000u) > 0;
        bool32 flipy = (tileID & 0x40000000u) > 0;
        bool32 flipz = (tileID & 0x20000000u) > 0; // diagonal flip

        return (flipz << 2) | (flipx << 1) | flipy;
}

static tmj_tilesets_s tmj_tilesets_parse(jsn_s jtilesets)
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

static u32 *tmj_tileID_array_from_tmj(jsn_s jlayer, void *(*allocf)(size_t))
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

static bool32 tmj_property(jsn_s j, const char *pname, jsn_s *property)
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