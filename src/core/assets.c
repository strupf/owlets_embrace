// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "gamedef.h"
#include "util/mem.h"

ASSETS_s ASSETS;

static int    assets_gen_texID();
static void  *assetmem_alloc_ctx(void *arg, usize s);
const alloc_s asset_allocator = {assetmem_alloc_ctx, NULL};

void assets_init()
{
    marena_init(&ASSETS.marena, ASSETS.mem, sizeof(ASSETS.mem));
    asset_tex_putID(TEXID_DISPLAY, tex_framebuffer());
}

typedef struct {
    u32 offs;
    u16 fmt;
    u16 wword;
    u16 w;
    u16 h;
} exp_tex_s;

typedef struct {
    u32 offs;
    u32 len;
} exp_snd_s;

typedef struct {
    u32 offs;
    u32 texID;
    u16 grid_w;
    u16 grid_h;
} exp_fnt_s;

typedef struct {
    u32 size;

    exp_tex_s tex[NUM_TEXID];
    exp_snd_s snd[NUM_SNDID];
    exp_fnt_s fnt[NUM_FNTID];
} asset_collection_s;

void assets_export()
{
    asset_collection_s coll = {0};
    const char        *mbeg = (const char *)&ASSETS.mem[0];
    coll.size               = (u32)((const char *)ASSETS.marena.p - mbeg);

    for (int n = 1; n < NUM_TEXID; n++) { // exclude display
        tex_s      t  = ASSETS.tex[n].tex;
        exp_tex_s *et = &coll.tex[n];
        et->offs      = (u32)((const char *)t.px - mbeg);
        et->w         = t.w;
        et->h         = t.h;
        et->wword     = t.wword;
        et->fmt       = t.fmt;
    }
    for (int n = 0; n < NUM_SNDID; n++) {
        snd_s      s  = ASSETS.snd[n].snd;
        exp_snd_s *es = &coll.snd[n];
        es->offs      = (u32)((const char *)s.buf - mbeg);
        es->len       = s.len;
    }
    for (int n = 0; n < NUM_FNTID; n++) {
        fnt_s      f  = ASSETS.fnt[n].fnt;
        exp_fnt_s *ef = &coll.fnt[n];
        ef->offs      = (u32)((const char *)f.widths - mbeg);
        ef->grid_w    = f.grid_w;
        ef->grid_h    = f.grid_h;
        for (int i = 0; i < NUM_TEXID; i++) {
            tex_s t = ASSETS.tex[i].tex;
            if (t.px == f.t.px) {
                ef->texID = i;
                break;
            }
        }
    }

    sys_file_remove(ASSET_FILENAME);
    void *fil = sys_file_open(ASSET_FILENAME, SYS_FILE_W);
    if (!fil) {
        sys_printf("COULD NOT CREATE ASSET FILE\n");
        return;
    }
    sys_file_write(fil, &coll, sizeof(coll));
    sys_file_write(fil, mbeg, coll.size);
    sys_file_close(fil);
    sys_printf("Wrote %i kb asset file!\n", (int)(coll.size / 1024));
}

void assets_import()
{
    asset_collection_s coll = {0};
    char              *mbeg = (char *)&ASSETS.mem[0];

    void *fil = sys_file_open(ASSET_FILENAME, SYS_FILE_R);
    sys_file_read(fil, &coll, sizeof(coll));
    sys_file_read(fil, mbeg, coll.size);
    sys_file_close(fil);

    for (int n = 1; n < NUM_TEXID; n++) { // exclude display
        tex_s    *t  = &ASSETS.tex[n].tex;
        exp_tex_s et = coll.tex[n];
        t->px        = (u32 *)(mbeg + et.offs);
        t->fmt       = et.fmt;
        t->w         = et.w;
        t->h         = et.h;
        t->wword     = et.wword;
    }
    for (int n = 0; n < NUM_SNDID; n++) {
        snd_s    *s  = &ASSETS.snd[n].snd;
        exp_snd_s es = coll.snd[n];
        s->buf       = (i8 *)(mbeg + es.offs);
        s->len       = es.len;
    }
    for (int n = 0; n < NUM_FNTID; n++) {
        fnt_s    *f  = &ASSETS.fnt[n].fnt;
        exp_fnt_s ef = coll.fnt[n];
        f->t         = ASSETS.tex[ef.texID].tex;
        f->widths    = (u8 *)(mbeg + ef.offs);
        f->grid_w    = ef.grid_w;
        f->grid_h    = ef.grid_h;
    }
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
    return mem;
}

tex_s asset_tex(int ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
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
    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);
    str_append(pathname, ".tex");
    sys_printf("LOAD TEX: %s (%s)\n", filename, pathname);

    tex_s t = tex_load(pathname, asset_allocator);
    if (t.px != NULL) {
        int ID             = assets_gen_texID();
        ASSETS.tex[ID].tex = t;
        if (tex) *tex = t;
        return ID;
    }
    sys_printf("Loading Tex FAILED\n");
    return -1;
}

int asset_tex_loadID(int ID, const char *filename, tex_s *tex)
{
    assert(0 <= ID && ID < NUM_TEXID);
    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);
    str_append(pathname, ".tex");
    sys_printf("LOAD TEX: %s (%s)\n", filename, pathname);
    usize memprev = ASSETS.marena.rem;
    tex_s t       = tex_load(pathname, asset_allocator);

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
    str_append(pathname, ".aud");

    sys_printf("LOAD SND: %s (%s)\n", filename, pathname);
    usize memprev      = ASSETS.marena.rem;
    snd_s s            = snd_load(pathname, asset_allocator);
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
    str_append(pathname, ".json");

    sys_printf("LOAD FNT: %s (%s)\n", filename, pathname);

    asset_fnt_s af = {0};
    af.fnt         = fnt_load(pathname, asset_allocator);
    asset_tex_put(af.fnt.t);
    ASSETS.fnt[ID] = af;
    if (af.fnt.widths) {
        if (fnt) *fnt = af.fnt;
        return ID;
    }
    return -1;
}

int asset_tex_put(tex_s t)
{
    int ID = assets_gen_texID();
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
    snd_play(asset_snd(ID), vol, pitch);
}

void asset_mus_fade_to(const char *filename, int ticks_out, int ticks_in)
{
    FILEPATH_GEN(path, FILEPATH_MUS, filename);
    mus_fade_to(path, ticks_out, ticks_in);
}

static int assets_gen_texID()
{
    static int texID = 0;
    int        ID    = NUM_TEXID_EXPLICIT + texID++;
    return ID;
}