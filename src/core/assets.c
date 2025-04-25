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

ani_s asset_ani(i32 ID)
{
    assert(0 <= ID && ID < NUM_ANIID);
    return APP->assets.ani[ID];
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

i32 snd_play(i32 ID, f32 vol, f32 pitch)
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

err32 tex_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, tex_s *o_t)
{
    if (!o_t) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek_str(f, wf, name);
    if (!e) return ASSETS_ERR_WAD_EL;

    tex_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(tex_header_s))) {
        return ASSETS_ERR_WAD_READ;
    }

    err32 err_t = 0;
    *o_t        = tex_create(h.w, h.h, 1, a, &err_t);
    if (err_t == 0) {
        usize size     = sizeof(u32) * o_t->wword * o_t->h;
        usize size_dec = lzss_decode_file(f, o_t->px);
        return (size == size_dec ? 0 : ASSETS_ERR_WAD_READ);
    }
    return ASSETS_ERR_ALLOC;
}

err32 snd_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, snd_s *o_s)
{
    if (!o_s) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek_str(f, wf, name);
    if (!e) return ASSETS_ERR_WAD_EL;

    qoa_file_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(qoa_file_header_s))) {
        return ASSETS_ERR_WAD_READ;
    }

    u32   num_slices = (h.num_samples + 20 - 1) / 20;
    usize size       = sizeof(u64) * num_slices;
    void *mem        = a.allocfunc(a.ctx, size, 8);
    if (mem) {
        o_s->num_samples = h.num_samples;
        o_s->dat         = mem;
        if (!pltf_file_r_checked(f, mem, size)) {
            return ASSETS_ERR_WAD_READ;
        }
    } else {
        return ASSETS_ERR_ALLOC;
    }
    return 0;
}

typedef struct {
    ALIGNAS(4)
    u16 ticks; // total length in ticks
    u16 n;     // number of frames
} ani_header_s;

err32 ani_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, ani_s *o_a)
{
    if (!o_a) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek_str(f, wf, name);
    if (!e) return ASSETS_ERR_WAD_EL;

    ani_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(ani_header_s))) {
        return ASSETS_ERR_WAD_READ;
    }

    usize size = sizeof(ani_frame_s) * h.n;
    void *mem  = a.allocfunc(a.ctx, size, 2);
    if (mem) {
        o_a->n     = h.n;
        o_a->ticks = h.ticks;
        o_a->f     = (ani_frame_s *)mem;
        if (!pltf_file_r_checked(f, mem, size)) {
            return ASSETS_ERR_WAD_READ;
        }
    } else {
        return ASSETS_ERR_ALLOC;
    }
    return 0;
}

i32 ani_frame(i32 ID, i32 ticks)
{
    ani_s a = asset_ani(ID);
    i32   t = ticks % a.ticks;

    for (i32 n = 0, t_acc = 0; n < a.n; n++) {
        ani_frame_s f = a.f[n];
        t_acc += f.t;
        if (t < t_acc) {
            return f.i;
        }
    }
    return ((i32)a.n - 1);
}

i32 ani_len(i32 ID)
{
    ani_s a = asset_ani(ID);
    return a.ticks;
}