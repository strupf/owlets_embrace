// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "wad.h"
#include "app.h"
#include "gamedef.h"
#include "pltf/pltf.h"
#include "util/lzss.h"

i32 wad_init_file(const void *filename)
{
    if (!filename) return WAD_ERR_OPEN;

    void *f = pltf_file_open_r(filename);
    if (!f) return WAD_ERR_OPEN;

    wad_s       *w   = &APP->wad;
    i32          res = 0;
    wad_header_s wh  = {0};

    if (pltf_file_rs(f, &wh, sizeof(wad_header_s))) {
        spm_push();
        usize          s_entries = sizeof(wad_el_file_s) * wh.n_entries;
        wad_el_file_s *f_entries = spm_alloct(wad_el_file_s, wh.n_entries);

        if (pltf_file_rs(f, f_entries, s_entries)) {
            wad_file_info_s *i = &w->files[w->n_files++];
            str_cpy(i->filename, filename);
            i->n_to = w->n_entries + wh.n_entries - 1;

            for (u32 n = 0; n < wh.n_entries; n++) {
                wad_el_s      *e = &w->entries[w->n_entries++];
                wad_el_file_s *k = &f_entries[n];
                e->hash          = k->hash;
                e->offs          = k->offs;
                e->size          = k->size;
                e->filename      = i->filename;
            }
        } else {
            res |= WAD_ERR_RW;
        }
        spm_pop();
    } else {
        res |= WAD_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        res |= WAD_ERR_CLOSE;
    }
    return (res);
}

wad_el_s *wad_el_find(u32 h, wad_el_s *efrom)
{
    if (!h) return 0;
    i32 n_beg = efrom ? (i32)(efrom - APP->wad.entries) : 0;

    for (i32 n = n_beg; n < APP->wad.n_entries; n++) {
        wad_el_s *e = &APP->wad.entries[n];

        if (e->hash == h) {
            // if passed efrom only return a result if found in the same file
            return (!efrom || e->filename == efrom->filename ? e : 0);
        }
    }
    return 0;
}

void *wad_open(u32 h, void **o_f, wad_el_s **o_e)
{
    wad_el_s *e = wad_el_find(h, 0);
    if (!e) return 0;

    void *f = pltf_file_open_r((const char *)e->filename);
    if (!f) return 0;

    pltf_file_seek_set(f, e->offs);

    if (o_f) {
        *o_f = f;
    }
    if (o_e) {
        *o_e = e;
    }
    return f;
}

void *wad_open_str(const void *name, void **o_f, wad_el_s **o_e)
{
    u32 h = wad_hash(name);
    return wad_open(h, o_f, o_e);
}

wad_el_s *wad_seek_str(void *f, wad_el_s *efrom, const void *name)
{
    if (!f) return 0;

    wad_el_s *e = wad_el_find(wad_hash(name), efrom);
    if (!e) return 0;

    pltf_file_seek_set(f, e->offs);
    return e;
}

void *wad_r_spm_str(void *f, wad_el_s *efrom, const void *name)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) return 0;

    void *dst = spm_alloc(e->size);
    pltf_file_r(f, dst, e->size);
    return dst;
}

void *wad_rd_spm_str(void *f, wad_el_s *efrom, const void *name)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) return 0;

    void *dst = spm_alloc(e->size);
    lzss_decode_file(f, dst);
    return dst;
}

void *wad_r_str(void *f, wad_el_s *efrom, const void *name, void *dst)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) return 0;

    pltf_file_r(f, dst, e->size);
    return dst;
}

void *wad_rd_str(void *f, wad_el_s *efrom, const void *name, void *dst)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) return 0;

    lzss_decode_file(f, dst);
    return dst;
}

u32 wad_hash(const void *str)
{
    const u8 *s = (const u8 *)str;
    u32       h = 0;
    for (i32 n = 0; s[n] != '\0'; n++) {
        h = h * 101 + (u32)s[n];
    }
    return h;
}