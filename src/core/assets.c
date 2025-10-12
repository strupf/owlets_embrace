// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/assets.h"
#include "app.h"
#include "gamedef.h"
#include "util/lzss.h"
#include "wad.h"

assets_s g_ASSETS;

tex_s *asset_texptr(i32 ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
    return &g_ASSETS.tex[ID];
}

tex_s asset_tex(i32 ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
    return g_ASSETS.tex[ID];
}

sfx_s asset_sfx(i32 ID)
{
    assert(0 <= ID && ID < NUM_SFXID);
    return g_ASSETS.sfx[ID];
}

fnt_s asset_fnt(i32 ID)
{
    assert(0 <= ID && ID < NUM_FNTID);
    return g_ASSETS.fnt[ID];
}

ani_s asset_ani(i32 ID)
{
    assert(0 <= ID && ID < NUM_ANIID);
    return g_ASSETS.ani[ID];
}

texrec_s asset_texrec_from_tex(i32 ID)
{
    tex_s    t  = asset_tex(ID);
    texrec_s tr = {t, 0, 0, t.w, t.h};
    return tr;
}

texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h)
{
    tex_s    t  = asset_tex(ID);
    texrec_s tr = {t, x, y, w ? min_i32(w, t.w) : t.w, h ? min_i32(h, t.h) : t.h};
    return tr;
}

err32 tex_from_wad_ID(i32 ID, const void *name, allocator_s a)
{
    return tex_from_wad_ext(name, a, asset_texptr(ID));
}

err32 sfx_from_wad_ID(i32 ID, const void *name, allocator_s a)
{
    void     *f;
    wad_el_s *el;
    if (wad_open(hash_str(name), &f, &el)) {
        sfx_s *s = &g_ASSETS.sfx[ID];
        err32  e = sfx_from_wad(f, el, name, a, s);
        pltf_file_close(f);
        return e;
    }
    return ASSETS_ERR_WAD_EL;
}

err32 tex_from_wad_ext(const void *name, allocator_s a, tex_s *o_t)
{
    if (!o_t) return ASSETS_ERR_MISC;

    void     *f;
    wad_el_s *e;
    if (!wad_open_str(name, &f, &e)) return ASSETS_ERR_WAD_EL;

    tex_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(tex_header_s))) {
        pltf_file_close(f);
        return ASSETS_ERR_WAD_READ;
    }

    err32 err_t = 0;
    tex_s t     = tex_create(h.w, h.h, 1, a, &err_t);
    if (err_t == 0) {
        usize size_dec = lzss_decode_file(f, t.px);
        pltf_file_close(f);
        *o_t = t;
        return (tex_size_bytes(t) == size_dec ? 0 : ASSETS_ERR_WAD_READ);
    }
    pltf_file_close(f);
    return ASSETS_ERR_ALLOC;
}

err32 tex_from_wad_ID_ext(void *f, i32 ID, const void *name, allocator_s a)
{
    return tex_from_wad(f, 0, name, a, asset_texptr(ID));
}

err32 tex_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, tex_s *o_t)
{
    return tex_from_wadh(f, wf, hash_str(name), a, o_t);
}

err32 sfx_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, sfx_s *o_s)
{
    return sfx_from_wadh(f, wf, hash_str(name), a, o_s);
}

typedef struct {
    ALIGNAS(4)
    u16 ticks; // total length in ticks
    u16 n;     // number of frames
} ani_header_s;

err32 ani_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, ani_s *o_a)
{
    return ani_from_wadh(f, wf, hash_str(name), a, o_a);
}

err32 tex_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, tex_s *o_t)
{
    if (!o_t) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek(f, wf, hash);
    if (!e) return ASSETS_ERR_WAD_EL;

    tex_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(tex_header_s))) {
        return ASSETS_ERR_WAD_READ;
    }

    err32 err_t = 0;
    tex_s t     = tex_create(h.w, h.h, h.fmt, a, &err_t);
    if (err_t == 0) {
        usize size_dec = lzss_decode_file(f, t.px);
        *o_t           = t;
        return (tex_size_bytes(t) == size_dec ? 0 : ASSETS_ERR_WAD_READ);
    }
    return ASSETS_ERR_ALLOC;
}

err32 sfx_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, sfx_s *o_s)
{
    if (!o_s) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek(f, wf, hash);
    if (!e) return ASSETS_ERR_WAD_EL;

    void *mem = a.allocfunc(a.ctx, e->size, 8);
    if (mem) {
        o_s->data = mem;
        if (!pltf_file_r_checked(f, mem, e->size)) {
            return ASSETS_ERR_WAD_READ;
        }
    } else {
        return ASSETS_ERR_ALLOC;
    }
    return 0;
}

err32 ani_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, ani_s *o_a)
{
    if (!o_a) return ASSETS_ERR_MISC;

    wad_el_s *e = wad_seek(f, wf, hash);
    if (!e) return ASSETS_ERR_WAD_EL;

    ani_header_s h = {0};
    if (!pltf_file_r_checked(f, &h, sizeof(ani_header_s))) {
        return ASSETS_ERR_WAD_READ;
    }

    usize size = sizeof(ani_frame_s) * h.n;
    void *mem  = a.allocfunc(a.ctx, size, 32);
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

i32 ani_frame_loop(i32 ID, i32 ticks)
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

i32 ani_frame(i32 ID, i32 ticks)
{
    ani_s a = asset_ani(ID);
    return (a.ticks <= ticks ? -1 : ani_frame_loop(ID, ticks));
}

i32 ani_len(i32 ID)
{
    return asset_ani(ID).ticks;
}