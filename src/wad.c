// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "wad.h"
#include "app.h"
#include "gamedef.h"
#include "pltf/pltf.h"
#include "util/lz.h"

wad_s g_WAD;

err32 wad_init_file(const void *filename)
{
    if (!filename) return WAD_FILE_ERR_BAD_ARG;

    void *f = pltf_file_open_r((const char *)filename);
    if (!f) return WAD_FILE_ERR_OPEN;

    wad_s       *w   = &g_WAD;
    wad_header_s h   = {0};
    err32        err = 0;

    if (pltf_file_r_checked(f, &h, sizeof(wad_header_s))) {
        if (APP_VERSION != APP_VERSION_GEN(h.v_major, h.v_minor, h.v_patch)) {
            err |= WAD_FILE_ERR_VERSION;
        }
        if (h.platformID && APP_VERSION_PLATFORM != h.platformID) {
            err |= WAD_FILE_ERR_PLATFORM;
        }

        if (err == 0) {
            wad_file_info_s *i = &w->files[w->n_files++];
            str_cpy(i->filename, filename);
            i->n_to = w->n_entries + (i32)h.n_entries - 1;

            // use the stack to load entries in chunks of max 256 entries at a time
            i32           n_left = (i32)h.n_entries;
            wad_el_file_s f_entries[256];

            while (n_left) {
                i32 n_read = min_i32(n_left, ARRLEN(f_entries));

                if (!pltf_file_r_checked(f, f_entries, sizeof(wad_el_file_s) * n_read)) {
                    err |= WAD_FILE_ERR_RW;
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
        }
    } else {
        err |= WAD_FILE_ERR_RW;
    }
    if (!pltf_file_close(f)) {
        err |= WAD_FILE_ERR_CLOSE;
    }
    return err;
}

wad_el_s *wad_el_find_ext(u32 h, wad_el_s *efrom, wad_el_s *eto)
{
    if (!h) return 0;

    wad_s *w     = &g_WAD;
    i32    n_beg = efrom ? (i32)(efrom - w->entries) : 0;

    for (i32 n = n_beg; n < w->n_entries; n++) {
        wad_el_s *e = &w->entries[n];

        if (e == eto) return 0;
        if (e->hash != h) continue;

        // if passed efrom only return a result if found in the same file
        return (efrom && e->filename != efrom->filename ? 0 : e);
    }
    return 0;
}

wad_el_s *wad_el_find(u32 h, wad_el_s *efrom)
{
    return wad_el_find_ext(h, efrom, 0);
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
    u32 h = hash_str(name);
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
    return wad_seek(f, efrom, hash_str(name));
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
    return wad_r_spm(f, efrom, hash_str(name));
}

void *wad_rd_spm_str(void *f, wad_el_s *efrom, const void *name)
{
    wad_el_s *e = wad_seek_str(f, efrom, name);
    if (!e) return 0;

    usize s   = lz_decoded_size_file(f);
    void *dst = spm_alloc(s);
    lz_decode_file(f, dst);
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

    usize dec = lz_decode_file(f, dst);
    return dst;
}