// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "gamedef.h"
#include "util/mem.h"

ASSETS_s ASSETS;

i32           assets_gen_texID();
void         *assetmem_alloc_ctx(void *arg, u32 s);
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
    u16 texID;
    u8  grid_w;
    u8  grid_h;
} exp_fnt_s;

typedef struct {
    u32 size;

    exp_tex_s tex[NUM_TEXID];
    exp_snd_s snd[NUM_SNDID];
    exp_fnt_s fnt[NUM_FNTID];
} asset_collection_s;

#if ASSETS_EXPORT
void assets_export()
{
    spm_push();
    asset_collection_s *coll = spm_alloctz(asset_collection_s, 1);
    const char         *mbeg = (const char *)&ASSETS.mem[0];
    coll->size               = (u32)((const char *)ASSETS.marena.p - mbeg);

    for (u32 n = 1; n < NUM_TEXID; n++) { // exclude display
        tex_s      t  = ASSETS.tex[n].tex;
        exp_tex_s *et = &coll->tex[n];
        et->offs      = (u32)((const char *)t.px - mbeg);
        et->w         = t.w;
        et->h         = t.h;
        et->wword     = t.wword;
        et->fmt       = t.fmt;
    }
    for (u32 n = 0; n < NUM_SNDID; n++) {
        snd_s      s  = ASSETS.snd[n].snd;
        exp_snd_s *es = &coll->snd[n];
        es->offs      = (u32)((const char *)s.buf - mbeg);
        es->len       = s.len;
    }
    for (u32 n = 0; n < NUM_FNTID; n++) {
        fnt_s      f  = ASSETS.fnt[n].fnt;
        exp_fnt_s *ef = &coll->fnt[n];
        ef->offs      = (u32)((const char *)f.widths - mbeg);
        ef->grid_w    = (u8)f.grid_w;
        ef->grid_h    = (u8)f.grid_h;
        for (u32 i = 0; i < NUM_TEXID; i++) {
            tex_s t = ASSETS.tex[i].tex;
            if (t.px == f.t.px) {
                ef->texID = i;
                break;
            }
        }
    }

    pltf_file_del(ASSET_FILENAME);
    void *fil = pltf_file_open_w(ASSET_FILENAME);
    if (!fil) {
        pltf_log("COULD NOT CREATE ASSET FILE\n");
        spm_pop();
        return;
    }

    pltf_file_w(fil, coll, sizeof(asset_collection_s));
#if 1 // compression
    static char compressed[0x800000];

    u32 stotal = mem_compress_block(compressed, mbeg, coll->size);
    pltf_file_w(fil, compressed, stotal);
    pltf_log("Wrote %u kb asset file!\n", (u32)((stotal + sizeof(asset_collection_s)) / 1024));
#else
    pltf_file_w(fil, mbeg, coll->size);
    pltf_log("Wrote %u kb asset file!\n", (u32)(coll->size / 1024));
#endif
    pltf_file_close(fil);

    spm_pop();
}
#endif

void assets_import()
{
    spm_push();
    asset_collection_s *coll = spm_alloct(asset_collection_s, 1);

    void *fil = pltf_file_open_r(ASSET_FILENAME);
    if (!fil) {
        pltf_log("COULD NOT OPEN ASSET FILE\n");
        spm_pop();
        return;
    }

    char *mbeg = (char *)&ASSETS.mem[0];
    pltf_file_r(fil, coll, sizeof(asset_collection_s));

#define MEM_ASSET_DECOMPRESSION 0x40000
    spm_push();
    void *decompmem = spm_alloc(MEM_ASSET_DECOMPRESSION);
    mem_decompress_block_from_file(fil, mbeg, decompmem, MEM_ASSET_DECOMPRESSION);
    spm_pop();
    pltf_file_close(fil);

    for (u32 n = 1; n < NUM_TEXID; n++) { // exclude display
        tex_s    *t  = &ASSETS.tex[n].tex;
        exp_tex_s et = coll->tex[n];
        t->px        = (u32 *)(mbeg + et.offs);
        t->fmt       = et.fmt;
        t->w         = et.w;
        t->h         = et.h;
        t->wword     = et.wword;
    }
    for (u32 n = 0; n < NUM_SNDID; n++) {
        snd_s    *s  = &ASSETS.snd[n].snd;
        exp_snd_s es = coll->snd[n];
        s->buf       = (u8 *)(mbeg + es.offs);
        s->len       = es.len;
    }
    for (u32 n = 0; n < NUM_FNTID; n++) {
        fnt_s    *f  = &ASSETS.fnt[n].fnt;
        exp_fnt_s ef = coll->fnt[n];
        f->t         = ASSETS.tex[ef.texID].tex;
        f->widths    = (u8 *)(mbeg + ef.offs);
        f->grid_w    = ef.grid_w;
        f->grid_h    = ef.grid_h;
    }
    spm_pop();
}

void *assetmem_alloc_ctx(void *arg, u32 s)
{
    return assetmem_alloc(s);
}

void *assetmem_alloc(u32 s)
{
    void *mem = marena_alloc(&ASSETS.marena, s);
    if (!mem) {
        pltf_log("+++ ran out of asset mem!\n");
        BAD_PATH
    }
    return mem;
}

tex_s asset_tex(i32 ID)
{
    assert(0 <= ID && ID < NUM_TEXID);
    return ASSETS.tex[ID].tex;
}

snd_s asset_snd(i32 ID)
{
    assert(0 <= ID && ID < NUM_SNDID);
    return ASSETS.snd[ID].snd;
}

fnt_s asset_fnt(i32 ID)
{
    assert(0 <= ID && ID < NUM_FNTID);
    return ASSETS.fnt[ID].fnt;
}

i32 asset_tex_load(const char *filename, tex_s *tex)
{
    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);
    str_append(pathname, ".tex");
#if ASSETS_LOG_LOADING
    pltf_log("LOAD TEX: %s (%s)\n", filename, pathname);
#endif
    tex_s t = tex_load(pathname, asset_allocator);
    if (t.px) {
        i32 ID             = assets_gen_texID();
        ASSETS.tex[ID].tex = t;
        if (tex) *tex = t;
        return ID;
    }
    pltf_log("+++ Loading Tex FAILED: %s\n", pathname);
    return -1;
}

i32 asset_tex_loadID(i32 ID, const char *filename, tex_s *tex)
{
    assert(0 <= ID && ID < NUM_TEXID);
    FILEPATH_GEN(pathname, FILEPATH_TEX, filename);
    str_append(pathname, ".tex");
#if ASSETS_LOG_LOADING
    pltf_log("LOAD TEX: %s (%s)\n", filename, pathname);
#endif
    tex_s t = tex_load(pathname, asset_allocator);

    ASSETS.tex[ID].tex = t;
    if (t.px) {
        if (tex) *tex = t;
        return ID;
    }
    pltf_log("+++ Loading Tex FAILED: %s\n", pathname);
    return -1;
}

i32 asset_snd_loadID(i32 ID, const char *filename, snd_s *snd)
{
    assert(0 <= ID && ID < NUM_SNDID);
    FILEPATH_GEN(pathname, FILEPATH_SND, filename);
    str_append(pathname, FILEEXTENSION_AUD);
#if ASSETS_LOG_LOADING
    pltf_log("LOAD SND: %s (%s)\n", filename, pathname);
#endif
    snd_s s            = snd_load(pathname, asset_allocator);
    ASSETS.snd[ID].snd = s;
    if (s.buf) {
        if (snd) *snd = s;
        return ID;
    }
    pltf_log("+++ Loading Snd FAILED: %s\n", pathname);
    return -1;
}

i32 asset_fnt_loadID(i32 ID, const char *filename, fnt_s *fnt)
{
    assert(0 <= ID && ID < NUM_FNTID);

    FILEPATH_GEN(pathname, FILEPATH_FNT, filename);
    str_append(pathname, ".json");
#if ASSETS_LOG_LOADING
    pltf_log("LOAD FNT: %s (%s)\n", filename, pathname);
#endif
    asset_fnt_s af = {0};
    af.fnt         = fnt_load(pathname, asset_allocator);
    asset_tex_put(af.fnt.t);
    ASSETS.fnt[ID] = af;
    if (af.fnt.widths) {
        if (fnt) *fnt = af.fnt;
        return ID;
    }
    pltf_log("+++ Loading Fnt FAILED: %s\n", pathname);
    return -1;
}

i32 asset_tex_put(tex_s t)
{
    i32 ID = assets_gen_texID();
    asset_tex_putID(ID, t);
    return ID;
}

tex_s asset_tex_putID(i32 ID, tex_s t)
{
    assert(0 <= ID && ID < NUM_TEXID);
    ASSETS.tex[ID].tex = t;
    return t;
}

texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h)
{
    texrec_s tr = {0};
    tr.t        = asset_tex(ID);
    tr.r.x      = x;
    tr.r.y      = y;
    tr.r.w      = w;
    tr.r.h      = h;
    return tr;
}

u32 snd_play(i32 ID, f32 vol, f32 pitch)
{
    return snd_instance_play(asset_snd(ID), vol, pitch);
}

void asset_mus_fade_to(const char *filename, i32 ticks_out, i32 ticks_in)
{
    FILEPATH_GEN(path, FILEPATH_MUS, filename);
    // mus_fade_to(path, ticks_out, ticks_in);
}

i32 assets_gen_texID()
{
    static i32 texID = 0;
    i32        ID    = NUM_TEXID_EXPLICIT + texID++;
    assert(ID < NUM_TEXID);
    return ID;
}

fnt_s fnt_load(const char *filename, alloc_s ma)
{
    spm_push();

    fnt_s f = {0};
    char *txt;
    if (!txt_load(filename, spm_alloc, &txt)) {
        pltf_log("+++ error loading font txt\n");
        return f;
    }

    f.widths = (u8 *)ma.allocf(ma.ctx, sizeof(u8) * 256);
    if (!f.widths) {
        pltf_log("+++ allocating font memory\n");
        spm_pop();
        return f;
    }

    json_s j;
    json_root(txt, &j);

    // replace .json with .tex
    char filename_tex[64];
    str_cpy(filename_tex, filename);
    char *fp = &filename_tex[str_len(filename_tex) - 5];
    assert(str_eq(fp, ".json"));
    str_cpy(fp, ".tex");

#if ASSETS_LOG_LOADING
    pltf_log("  loading fnt tex: %s\n", filename_tex);
#endif
    f.t      = tex_load(filename_tex, ma);
    f.grid_w = jsonk_u32(j, "gridwidth");
    f.grid_h = jsonk_u32(j, "gridheight");

    json_key(j, "glyphwidths", &j);
    json_fchild(j, &j);
    for (int i = 0; i < 256; i++) {
        f.widths[i] = json_u32(j);
        json_next(j, &j);
    }

    spm_pop();
    return f;
}