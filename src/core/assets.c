// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "util/mem.h"

ASSETS_s ASSETS;

static void  *assetmem_alloc_ctx(void *arg, usize s);
const alloc_s asset_allocator = {assetmem_alloc_ctx, NULL};

void assets_init()
{
    marena_init(&ASSETS.marena, ASSETS.mem, sizeof(ASSETS.mem));
    ASSETS.next_texID = NUM_TEXID;
}

static void *assetmem_alloc_ctx(void *arg, usize s)
{
    return assetmem_alloc(s);
}

void *assetmem_alloc(usize s)
{
    void *mem = marena_alloc(&ASSETS.marena, s);
    if (!mem) {
        sys_printf("+++ ran out of asset mem!\n");
        BAD_PATH
    }
    sys_printf("Asset mem left: %u kb\n", (u32)marena_size_rem(&ASSETS.marena) / 1024);
    return mem;
}

tex_s asset_tex(int ID)
{
    assert(0 <= ID && ID < NUM_TEXID_MAX);
    return ASSETS.tex[ID].tex;
}

snd_s asset_snd(int ID)
{
    assert(0 <= ID && ID < NUM_SNDID);
    return ASSETS.snd[ID].snd;
}

fnt_s asset_fnt(int ID)
{
    assert(0 <= ID && ID < NUM_FNTID);
    return ASSETS.fnt[ID].fnt;
}

int asset_tex_load(const char *filename, tex_s *tex)
{
    for (int i = 0; i < ASSETS.next_texID; i++) {
        asset_tex_s *at = &ASSETS.tex[i];
        if (str_eq(at->file, filename)) {
            if (tex) *tex = at->tex;
            return i;
        }
    }

    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);
    sys_printf("LOAD TEX: %s (%s)\n", filename, pathname);

    tex_s t = tex_load(pathname, asset_allocator);
    if (t.px != NULL) {
        int ID = ASSETS.next_texID++;
        str_cpy(ASSETS.tex[ID].file, filename);
        ASSETS.tex[ID].tex = t;
        if (tex) *tex = t;
        return ID;
    }
    sys_printf("Loading Tex FAILED\n");
    return -1;
}

int asset_tex_loadID(int ID, const char *filename, tex_s *tex)
{
    assert(0 <= ID && ID < NUM_TEXID_MAX);
    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);

    sys_printf("LOAD TEX: %s (%s)\n", filename, pathname);

    tex_s t = tex_load(pathname, asset_allocator);
    str_cpy(ASSETS.tex[ID].file, filename);
    ASSETS.tex[ID].tex = t;
    if (t.px) {
        if (tex) *tex = t;
        return ID;
    }
    return -1;
}

int asset_snd_loadID(int ID, const char *filename, snd_s *snd)
{
    assert(0 <= ID && ID < NUM_SNDID);
    FILEPATH_GEN(pathname, FILEPATH_SND, filename);

    sys_printf("LOAD SND: %s (%s)\n", filename, pathname);

    str_cpy(ASSETS.snd[ID].file, filename);
    snd_s s            = aud_snd_load(pathname, asset_allocator);
    ASSETS.snd[ID].snd = s;
    if (s.buf) {
        if (snd) *snd = s;
        return ID;
    }
    return -1;
}

int asset_fnt_loadID(int ID, const char *filename, fnt_s *fnt)
{
    assert(0 <= ID && ID < NUM_FNTID);

    FILEPATH_GEN(pathname, FILEPATH_FNT, filename);

    sys_printf("LOAD FNT: %s (%s)\n", filename, pathname);

    asset_fnt_s af = {0};
    str_cpy(af.file, filename);
    af.fnt         = fnt_load(pathname, asset_allocator);
    ASSETS.fnt[ID] = af;
    if (af.fnt.widths) {
        if (fnt) *fnt = af.fnt;
        return ID;
    }
    return -1;
}

int asset_tex_put(tex_s t)
{
    int ID = ASSETS.next_texID++;
    asset_tex_putID(ID, t);
    return ID;
}

tex_s asset_tex_putID(int ID, tex_s t)
{
    assert(0 <= ID && ID < NUM_TEXID);
    ASSETS.tex[ID].tex = t;
    return t;
}

texrec_s asset_texrec(int ID, int x, int y, int w, int h)
{
    texrec_s tr = {0};
    tr.t        = asset_tex(ID);
    tr.r.x      = x;
    tr.r.y      = y;
    tr.r.w      = w;
    tr.r.h      = h;
    return tr;
}

void snd_play_ext(int ID, f32 vol, f32 pitch)
{
    aud_snd_play(asset_snd(ID), vol, pitch);
}

void mus_fade_to(const char *filename, int ticks_out, int ticks_in)
{
    FILEPATH_GEN(path, FILEPATH_MUS, filename);
    aud_mus_fade_to(path, ticks_out, ticks_in);
}

void mus_stop()
{
    aud_mus_stop();
}

bool32 mus_playing()
{
    return aud_mus_playing();
}
