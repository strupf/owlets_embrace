// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "util/mem.h"

ASSETS_s ASSETS;

void assets_init()
{
    marena_init(&ASSETS.marena, ASSETS.mem, sizeof(ASSETS.mem));
}

void *assetmem_alloc(usize s)
{
    void *mem = marena_alloc(&ASSETS.marena, s);
    if (!mem) {
        sys_printf("+++ ran out of asset mem!\n");
        BAD_PATH
    }
    return mem;
}

tex_s asset_tex(int ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
    return ASSETS.tex[ID];
}

snd_s asset_snd(int ID)
{
    assert(0 <= ID && ID < NUM_SNDID);
    return ASSETS.snd[ID];
}

fnt_s asset_fnt(int ID)
{
    assert(0 <= ID && ID < NUM_FNTID);
    return ASSETS.fnt[ID];
}

spriteanimdata_s asset_anim(int ID)
{
    assert(0 <= ID && ID < NUM_ANIMID);
    return ASSETS.anim[ID];
}

tex_s asset_tex_load(int ID, const char *filename)
{
    assert(0 <= ID && ID < NUM_TEXID);
    tex_s t        = tex_load(filename, assetmem_alloc);
    ASSETS.tex[ID] = t;
    return t;
}

snd_s asset_snd_load(int ID, const char *filename)
{
    assert(0 <= ID && ID < NUM_SNDID);
    snd_s s        = snd_load(filename, assetmem_alloc);
    ASSETS.snd[ID] = s;
    return s;
}

fnt_s asset_fnt_load(int ID, const char *filename)
{
    assert(0 <= ID && ID < NUM_FNTID);
    fnt_s f        = fnt_load(filename, assetmem_alloc);
    ASSETS.fnt[ID] = f;
    return f;
}

spriteanimdata_s asset_anim_load(int ID, const char *filename)
{
    spriteanimdata_s a = {0};
    return a;
}

void asset_tex_put(int ID, tex_s t)
{
    assert(0 <= ID && ID < NUM_TEXID);
    ASSETS.tex[ID] = t;
}

void asset_anim_put(int ID, spriteanimdata_s a)
{
    assert(0 <= ID && ID < NUM_ANIMID);
    ASSETS.anim[ID] = a;
}