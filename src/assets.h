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
    TEXID_TILESET,
    TEXID_UI,
    TEXID_UI_ITEM_CACHE,
    TEXID_UI_ITEMS,
#ifdef SYS_DEBUG
    TEXID_COLLISION_TILES,
#endif
    //
    NUM_TEXID
};

enum {
    FNTID_DEFAULT,
    //
    NUM_FNTID
};

enum {
    SNDID_DEFAULT,
    SNDID_HOOK_ATTACH,
    //
    NUM_SNDID
};

enum {
    ANIMID_HERO,
    //
    NUM_ANIMID
};

typedef struct {
    tex_s            tex[NUM_TEXID];
    snd_s            snd[NUM_SNDID];
    fnt_s            fnt[NUM_FNTID];
    spriteanimdata_s anim[NUM_ANIMID];

    marena_s marena;
    alignas(4) char mem[MKILOBYTE(6144)];
} ASSETS_s;

extern ASSETS_s ASSETS;

void             assets_init();
//
void            *assetmem_alloc(usize s);
tex_s            asset_tex(int ID);
snd_s            asset_snd(int ID);
fnt_s            asset_fnt(int ID);
spriteanimdata_s asset_anim(int ID);
tex_s            asset_tex_load(int ID, const char *filename);
snd_s            asset_snd_load(int ID, const char *filename);
fnt_s            asset_fnt_load(int ID, const char *filename);
spriteanimdata_s asset_anim_load(int ID, const char *filename);
void             asset_tex_put(int ID, tex_s t);
void             asset_anim_put(int ID, spriteanimdata_s a);

#endif