// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

#include "aud.h"
#include "gfx.h"
#include "spriteanim.h"

enum {
    TEXID_DISPLAY,
    TEXID_HERO,
    TEXID_HERO_WHIP,
    TEXID_TILESET,
    TEXID_UI,
    TEXID_UI_ITEM_CACHE,
    TEXID_UI_ITEMS,
    TEXID_UI_TEXTBOX,
    TEXID_PLANTS,
    TEXID_CLOUDS,
    TEXID_TITLE,
    TEXID_BACKGROUND,
    TEXID_PROPS,
//
#ifdef SYS_DEBUG
    TEXID_COLLISION_TILES,
#endif
    //
    NUM_TEXID,
    //
    NUM_TEXID_MAX = 256
};

enum {
    FNTID_SMALL,
    FNTID_MEDIUM,
    FNTID_LARGE,
    //
    NUM_FNTID
};

enum {
    SNDID_DEFAULT,
    SNDID_HOOK_ATTACH,
    SNDID_SPEAK,
    //
    NUM_SNDID
};

typedef struct {
    tex_s tex;
    char  file[32];
} asset_tex_s;

typedef struct {
    snd_s snd;
    char  file[32];
} asset_snd_s;

typedef struct {
    fnt_s fnt;
    char  file[32];
} asset_fnt_s;

typedef struct {
    asset_tex_s tex[NUM_TEXID_MAX];
    asset_snd_s snd[NUM_SNDID];
    asset_fnt_s fnt[NUM_FNTID];

    int next_texID;

    marena_s marena;
    alignas(4) char mem[MKILOBYTE(6144)];
} ASSETS_s;

extern ASSETS_s ASSETS;

void  assets_init();
usize assets_mem_left();
//
void *assetmem_alloc(usize s);
tex_s asset_tex(int ID);
snd_s asset_snd(int ID);
fnt_s asset_fnt(int ID);
int   asset_tex_load(const char *filename, tex_s *tex);
int   asset_tex_loadID(int ID, const char *filename, tex_s *tex);
int   asset_snd_loadID(int ID, const char *filename, snd_s *snd);
int   asset_fnt_loadID(int ID, const char *filename, fnt_s *fnt);
int   asset_tex_put(tex_s t);
tex_s asset_tex_putID(int ID, tex_s t);

texrec_s asset_texrec(int ID, int x, int y, int w, int h);

#endif