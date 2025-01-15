// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app_load.h"

typedef struct {
    void       *f;
    wad_el_s   *e;
    allocator_s allocator;
} app_load_s;

i32 app_texID_create_put(i32 ID, i32 w, i32 h, b32 mask, allocator_s a, tex_s *o_t)
{
    tex_s t = {0};
    i32   r = tex_create_ext(w, h, mask, a, &t);
    if (r == 0) {
        if (o_t) {
            *o_t = t;
        }
    }
    APP->assets.tex[ID].tex = t;
    return r;
}

static i32 app_load_tex_internal(app_load_s l, i32 ID, const void *name)
{
    tex_s *t = &APP->assets.tex[ID].tex;
    i32    r = tex_from_wad(l.f, l.e, name, l.allocator, t);
    if (r != 0) {
        pltf_log("ERROR LOADING TEX: %i\n", r);
    }
    return r;
}

static i32 app_load_snd_internal(app_load_s l, i32 ID, const void *name)
{
    snd_s *s = &APP->assets.snd[ID].snd;
    i32    r = snd_from_wad(l.f, l.e, name, l.allocator, s);
    if (r != 0) {
        pltf_log("ERROR LOADING SND: %i\n", r);
    }
    return r;
}

i32 app_load_assets()
{
    void     *f = 0;
    wad_el_s *e = 0;

    if (!wad_open_str("WAD_MAIN", &f, &e)) return -1;

    app_load_s l = {f, 0, app_allocator()};
    i32        r = 0;

    // TEX ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_TEX");
    if (l.e) {
        r |= app_load_tex_internal(l, TEXID_TILESET_TERRAIN, "T_TSTERR");
        r |= app_load_tex_internal(l, TEXID_TILESET_BG_AUTO, "T_TSBGA");
        r |= app_load_tex_internal(l, TEXID_TILESET_PROPS, "T_TSPROP");
        r |= app_load_tex_internal(l, TEXID_TILESET_DECO, "T_TSDECO");
        r |= app_load_tex_internal(l, TEXID_HERO, "T_HERO");
    }

    water_prerender_tiles();

    // SND ---------------------------------------------------------------------
    // r |= app_load_snd_internal(l, TEXID_TILESET_TERRAIN, "TSTERR");

    pltf_file_close(l.f);
    return r;
}