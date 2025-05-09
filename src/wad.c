// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "wad.h"
#include "app.h"
#include "gamedef.h"
#include "pltf/pltf.h"
#include "util/lzss.h"

err32 wad_init_file(const void *filename)
{
    if (!filename) return WAD_ERR_OPEN;

    void *f = pltf_file_open_r((const char *)filename);
    if (!f) return WAD_ERR_OPEN;

    wad_s       *w  = &APP->wad;
    err32        r  = 0;
    wad_header_s wh = {0};

    if (pltf_file_r_checked(f, &wh, sizeof(wad_header_s))) {
        wad_file_info_s *i = &w->files[w->n_files++];
        str_cpy(i->filename, filename);
        i->n_to = w->n_entries + wh.n_entries - 1;

        // don't rely on SPM being initialized here, streamed loading -> stack
        i32           n_left = wh.n_entries;
        wad_el_file_s f_entries[128];

        while (n_left) {
            i32   n_read = min_i32(n_left, ARRLEN(f_entries));
            usize sread  = sizeof(wad_el_file_s) * n_read;

            if (!pltf_file_r_checked(f, f_entries, sread)) {
                r |= WAD_ERR_RW;
                break;
            }

            n_left -= n_read;

            for (i32 n = 0; n < n_read; n++) {
                wad_el_s      *el = &w->entries[w->n_entries++];
                wad_el_file_s *ef = &f_entries[n];
                el->hash          = ef->hash;
                el->offs          = ef->offs;
                el->size          = ef->size;
                el->filename      = i->filename;
            }
        }
    } else {
        r |= WAD_ERR_RW;
    }
    if (!pltf_file_close(f)) {
        r |= WAD_ERR_CLOSE;
    }
    return r;
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

wad_el_s *wad_seek(void *f, wad_el_s *efrom, u32 hash)
{
    if (!f) return 0;

    wad_el_s *e = wad_el_find(hash, efrom);
    if (!e) return 0;

    pltf_file_seek_set(f, e->offs);
    return e;
}

wad_el_s *wad_seek_str(void *f, wad_el_s *efrom, const void *name)
{
    return wad_seek(f, efrom, wad_hash(name));
}

void *wad_r_spm(void *f, wad_el_s *efrom, u32 hash)
{
    wad_el_s *e = wad_seek(f, efrom, hash);
    if (!e) return 0;

    void *dst = spm_alloc(e->size);
    if (!pltf_file_r_checked(f, dst, e->size))
        return 0;
    return dst;
}

void *wad_r_spm_str(void *f, wad_el_s *efrom, const void *name)
{
    return wad_r_spm(f, efrom, wad_hash(name));
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

    if (!pltf_file_r_checked(f, dst, e->size))
        return 0;
    return dst;
}

void *wad_rd_str(void *f, wad_el_s *efrom, const void *name, void *dst)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) {
        pltf_log("WAD EL NOT FOUND: %s\n", (const char *)name);
        return 0;
    }

    usize dec = lzss_decode_file(f, dst);
    return dst;
}

u32 wad_hash(const void *str)
{
    if (!str) return 0;
    const u8 *s = (const u8 *)str;
    u32       h = 0;
    for (i32 n = 0; s[n] != '\0'; n++) {
        h = h * 101 + (u32)s[n];
    }
    return h;
}