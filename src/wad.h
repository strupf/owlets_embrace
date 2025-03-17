// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WAD_H
#define WAD_H

#define WAD_NUM_FILES   8
#define WAD_NUM_ENTRIES 4096

#include "pltf/pltf_types.h"

enum {
    WAD_ERR_OPEN    = 1 << 0,
    WAD_ERR_CLOSE   = 1 << 1,
    WAD_ERR_RW      = 1 << 2,
    WAD_ERR_VERSION = 1 << 3,
    WAD_ERR_EXISTS  = 1 << 4,
};

typedef struct {
    i32 n_entries;
    u32 version;
} wad_header_s;

typedef struct {
    u32 hash; // really a 8-character string
    u32 offs; // begin of memory block in file
    u32 size; // size of memory block
} wad_el_file_s;

typedef struct {
    ALIGNAS(16)
    u8 *filename;
    u32 hash; // really a 8-character string
    u32 offs; // begin of memory block in file
    u32 size; // size of memory block
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

// initializes a file to be used as a wad
// returns wad error code
err32 wad_init_file(const void *filename);

// finds a wad entry
// if passed efrom: returns an entry only if found in the same file
wad_el_s *wad_el_find(u32 h, wad_el_s *efrom);

// opens a file if entry is found
// returns file handle seeked to the element or null
void *wad_open(u32 h, void **o_f, wad_el_s **o_e);

// opens a file if entry is found
// returns file handle or null
void *wad_open_str(const void *name, void **o_f, wad_el_s **o_e);

wad_el_s *wad_seek_str(void *f, wad_el_s *efrom, const void *name);

void *wad_r_spm_str(void *f, wad_el_s *efrom, const void *name);
void *wad_rd_spm_str(void *f, wad_el_s *efrom, const void *name);
void *wad_r_str(void *f, wad_el_s *efrom, const void *name, void *dst);
void *wad_rd_str(void *f, wad_el_s *efrom, const void *name, void *dst);

u32 wad_hash(const void *str);

#endif