// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WAD_H
#define WAD_H

#define WAD_NUM_FILES   8
#define WAD_NUM_ENTRIES 4096

#include "pltf/pltf_types.h"

enum {
    WAD_FILE_ERR_BAD_ARG  = 1 << 0,
    WAD_FILE_ERR_OPEN     = 1 << 1,
    WAD_FILE_ERR_VERSION  = 1 << 2,
    WAD_FILE_ERR_PLATFORM = 1 << 3,
    WAD_FILE_ERR_RW       = 1 << 4,
    WAD_FILE_ERR_CLOSE    = 1 << 5,
    WAD_FILE_ERR_MISC     = 1 << 6,
};

typedef struct wad_header_s {
    ALIGNAS(16)
    u8  platformID; // the intended platformID
    u8  v_major;    // the intended version of the game
    u8  v_minor;    // the intended version of the game
    u8  v_patch;    // the intended version of the game
    u8  str[4];     // "WADF"
    u32 n_entries;
    u8  unused[2];
} wad_header_s;

typedef struct {
    u32 hash; // hash of resource name
    u32 offs; // begin of memory block in file
    u32 size; // size of memory block
} wad_el_file_s;

typedef struct wad_el_s {
    ALIGNAS(16)
    u8 *filename; // filename of the parent WAD file
    u32 hash;     // hash of resource name
    u32 offs;     // begin of memory block in file
    u32 size;     // size of memory block
} wad_el_s;

typedef struct {
    ALIGNAS(32)
    i32 n_to;
    u8  filename[28];
} wad_file_info_s;

static_assert(sizeof(wad_file_info_s) == 32, "Size WAD finfo");

typedef struct {
    i32             n_files;
    i32             n_entries;
    wad_file_info_s files[WAD_NUM_FILES];
    wad_el_s        entries[WAD_NUM_ENTRIES];
} wad_s;

extern wad_s g_WAD;

err32     wad_init_file(const void *filename);                        // initializes a file to be used as a wad, returns wad error code
wad_el_s *wad_el_find(u32 h, wad_el_s *efrom);                        // finds a wad entry; if passed efrom: returns an entry only if found in the same file
wad_el_s *wad_el_find_ext(u32 h, wad_el_s *efrom, wad_el_s *eto);     // finds a wad entry; if passed efrom: returns an entry only if found in the same file
void     *wad_open(u32 h, void **o_f, wad_el_s **o_e);                // opens a file if entry is found, returns file handle seeked to the element or null
void     *wad_open_str(const void *name, void **o_f, wad_el_s **o_e); // opens a file if entry is found, returns file handle or null
wad_el_s *wad_seek(void *f, wad_el_s *efrom, u32 hash);
wad_el_s *wad_seek_str(void *f, wad_el_s *efrom, const void *name);
void     *wad_r_spm(void *f, wad_el_s *efrom, u32 hash);
void     *wad_r_spm_str(void *f, wad_el_s *efrom, const void *name);
void     *wad_rd_spm_str(void *f, wad_el_s *efrom, const void *name);
void     *wad_r_str(void *f, wad_el_s *efrom, const void *name, void *dst);
void     *wad_rd_str(void *f, wad_el_s *efrom, const void *name, void *dst);
#endif