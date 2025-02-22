// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "app.h"
#include "gamedef.h"
#include "util/lzss.h"
#include "wad.h"

i32 assets_gen_texID();

i32 assets_init()
{
    asset_tex_putID(TEXID_DISPLAY, tex_framebuffer());
    return 0;
}

tex_s asset_tex(i32 ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
    return APP->assets.tex[ID];
}

snd_s asset_snd(i32 ID)
{
    assert(0 <= ID && ID < NUM_SNDID);
    return APP->assets.snd[ID];
}

fnt_s asset_fnt(i32 ID)
{
    assert(0 <= ID && ID < NUM_FNTID);
    return APP->assets.fnt[ID];
}

i32 asset_tex_put(tex_s t)
{
    i32 ID = assets_gen_texID();
    asset_tex_putID(ID, t);
    return ID;
}

tex_s asset_tex_putID(i32 ID, tex_s t)
{
    assert(0 <= ID && ID < NUM_TEXID);
    APP->assets.tex[ID] = t;
    return t;
}

texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h)
{
    texrec_s tr = {asset_tex(ID), x, y, w, h};
    return tr;
}

u32 snd_play(i32 ID, f32 vol, f32 pitch)
{
    return snd_instance_play(asset_snd(ID), vol, pitch);
}

void asset_mus_fade_to(const char *filename, i32 ticks_out, i32 ticks_in)
{
    // mus_fade_to(path, ticks_out, ticks_in);
}

i32 assets_gen_texID()
{
    static i32 texID = 0;
    i32        ID    = NUM_TEXID_EXPLICIT + texID++;
    assert(ID < NUM_TEXID);
    return ID;
}

enum {
    ASSET_ERR_OUT       = 1 << 0,
    ASSET_ERR_WAD_ENTRY = 1 << 1,
    ASSET_ERR_RW        = 1 << 2,
    ASSET_ERR_ALLOC     = 1 << 3,
};

i32 tex_from_wad(void *f, wad_el_s *wf, const void *name,
                 allocator_s a, tex_s *o_t)
{
    if (!o_t) return ASSET_ERR_OUT;

    wad_el_s *e = wad_seek_str(f, wf, name);
    if (!e) return ASSET_ERR_WAD_ENTRY;

    tex_header_s h  = {0};
    i32          rh = pltf_file_r(f, &h, sizeof(tex_header_s));
    if (rh != (i32)sizeof(tex_header_s)) return ASSET_ERR_RW;

    i32 err_t = 0;
    *o_t      = tex_create(h.w, h.h, 1, a, &err_t);
    if (err_t == 0) {
        usize size     = o_t->wword * o_t->h * sizeof(u32);
        usize size_dec = lzss_decode_file(f, o_t->px);
        return (size == size_dec ? 0 : ASSET_ERR_RW);
    }
    return ASSET_ERR_ALLOC;
}

i32 snd_from_wad(void *f, wad_el_s *wf, const void *name,
                 allocator_s a, snd_s *o_s)
{
    if (!o_s) return ASSET_ERR_OUT;

    wad_el_s *e = wad_seek_str(f, wf, name);
    if (!e) return ASSET_ERR_WAD_ENTRY;

    qoa_file_header_s h  = {0};
    i32               rh = pltf_file_r(f, &h, sizeof(qoa_file_header_s));
    if (rh != (i32)sizeof(qoa_file_header_s)) return ASSET_ERR_RW;

    u32   num_slices = (h.num_samples + 20 - 1) / 20;
    usize size       = sizeof(u64) * num_slices;
    void *mem        = a.allocfunc(a.ctx, size, 8);
    if (mem) {
        o_s->num_samples = h.num_samples;
        o_s->dat         = mem;
        i32 rd           = pltf_file_r(f, mem, size);
        return (rd == (i32)size ? 0 : ASSET_ERR_RW);
    }
    return ASSET_ERR_ALLOC;
}
