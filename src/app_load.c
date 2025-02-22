// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app_load.h"

typedef struct {
    void       *f;
    wad_el_s   *e;
    allocator_s allocator;
} app_load_s;

i32        app_texID_create_put(i32 ID, i32 w, i32 h, b32 mask, allocator_s a, tex_s *o_t);
static i32 app_load_tex_internal(app_load_s l, i32 ID, const void *name);
static i32 app_load_fnt_internal(app_load_s l, i32 ID, const void *name);
static i32 app_load_snd_internal(app_load_s l, i32 ID, const void *name);

i32 app_load_assets()
{
    void     *f = 0;
    wad_el_s *e = 0;
    if (!wad_open_str("WAD_MAIN", &f, &e)) return -1;

    app_load_s l = {f, 0, app_allocator()};
    i32        r = 0;

    // TEX ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_TEX");
    if (l.e) {
        r |= app_load_tex_internal(l, TEXID_TILESET_TERRAIN, "T_TSTERR");
        r |= app_load_tex_internal(l, TEXID_TILESET_BG_AUTO, "T_TSBGA");
        r |= app_load_tex_internal(l, TEXID_TILESET_PROPS, "T_TSPROP");
        r |= app_load_tex_internal(l, TEXID_TILESET_DECO, "T_TSDECO");
        r |= app_load_tex_internal(l, TEXID_HERO, "T_HERO");
        r |= app_load_tex_internal(l, TEXID_ROTOR, "T_ROTOR");
        r |= app_load_tex_internal(l, TEXID_BGOLEM, "T_BGOLEM");
        r |= app_load_tex_internal(l, TEXID_FLSURF, "T_FLSURF");
        r |= app_load_tex_internal(l, TEXID_FLYBLOB, "T_FBLOB");
        r |= app_load_tex_internal(l, TEXID_SOLIDLEVER, "T_SOLIDLEVER");
        r |= app_load_tex_internal(l, TEXID_JUMPER, "T_JUMPER");
        r |= app_load_tex_internal(l, TEXID_MISCOBJ, "T_MISC");
        r |= app_load_tex_internal(l, TEXID_PARTICLES, "T_PARTICLES");
        r |= app_load_tex_internal(l, TEXID_STAMINARESTORE, "T_RESTORE");
        r |= app_load_tex_internal(l, TEXID_TRAMPOLINE, "T_TRAMPOLINE");
        r |= app_load_tex_internal(l, TEXID_CRAWLER, "T_CRAWLER");
        r |= app_load_tex_internal(l, TEXID_BUDPLANT, "T_BUDPLANT");
        r |= app_load_tex_internal(l, TEXID_CHEST, "T_CHEST");
        r |= app_load_tex_internal(l, TEXID_STAMINARESTORE, "T_STAMINA");
        r |= app_load_tex_internal(l, TEXID_HOOK, "T_HOOK");
        r |= app_load_tex_internal(l, TEXID_EXPLO1, "T_EXPLO1");
        r |= app_load_tex_internal(l, TEXID_UI, "T_UI_EL");
        r |= app_load_tex_internal(l, TEXID_BUTTONS, "T_BUTTONS");
        r |= app_load_tex_internal(l, TEXID_COMPANION, "T_COMPANION");
    }

    // FNT ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_FNT");
    if (l.e) {
        r |= app_load_fnt_internal(l, FNTID_12, "F_A_12");
        r |= app_load_fnt_internal(l, FNTID_16, "F_A_16");
        r |= app_load_fnt_internal(l, FNTID_20, "F_A_20");
    }

    // SND ---------------------------------------------------------------------
    // r |= app_load_snd_internal(l, TEXID_TILESET_TERRAIN, "TSTERR");
    l.e = wad_seek_str(f, e, "WAD_SFX");
    r |= app_load_snd_internal(l, SNDID_JUMP, "S_JUMP01");
    r |= app_load_snd_internal(l, SNDID_KLONG, "S_HOOK");

    pltf_file_close(l.f);
    return r;
}

i32 app_texID_create_put(i32 ID, i32 w, i32 h, b32 mask, allocator_s a, tex_s *o_t)
{
    i32   r = 0;
    tex_s t = tex_create(w, h, mask, a, &r);
    if (r == 0) {
        if (o_t) {
            *o_t = t;
        }
    }
    APP->assets.tex[ID] = t;
    return r;
}

static i32 app_load_tex_internal(app_load_s l, i32 ID, const void *name)
{
    tex_s *t = &APP->assets.tex[ID];
    i32    r = tex_from_wad(l.f, l.e, name, l.allocator, t);
    if (r != 0) {
        pltf_log("ERROR LOADING TEX: %i\n", r);
    }
    return r;
}

static i32 app_load_fnt_internal(app_load_s l, i32 ID, const void *name)
{
    fnt_s    *f = &APP->assets.fnt[ID];
    wad_el_s *e = wad_seek_str(l.f, l.e, name);
    if (!e) return 1;

    f->widths = (u8 *)l.allocator.allocfunc(l.allocator.ctx, 256, 1);
    if (!f->widths) return 2;

    if (!pltf_file_rs(l.f, &f->tracking, sizeof(i16)) ||
        !pltf_file_rs(l.f, &f->grid_w, sizeof(u8)) ||
        !pltf_file_rs(l.f, &f->grid_h, sizeof(u8)) ||
        !pltf_file_rs(l.f, f->widths, 256))
        return 3;

#if 0
        if (!pltf_file_rs(l.f, &f->n_kerning, sizeof(u16)))
        return 3;
    f->kerning = l.allocator.allocfunc(l.allocator.ctx,
                                       sizeof(fnt_kerning_s) * f->n_kerning,
                                       1);
#endif

    tex_header_s h = {0};
    if (!pltf_file_rs(l.f, &h, sizeof(tex_header_s))) return 4;

    i32 err_t = 0;
    f->t      = tex_create(h.w, h.h, 1, l.allocator, &err_t);
    if (err_t != 0) return 5;

    usize size     = sizeof(u32) * f->t.wword * f->t.h;
    usize size_dec = lzss_decode_file(l.f, f->t.px);
    if (size != size_dec) return 6;
    return 0;
}

static i32 app_load_snd_internal(app_load_s l, i32 ID, const void *name)
{
    snd_s *s = &APP->assets.snd[ID];
    i32    r = snd_from_wad(l.f, l.e, name, l.allocator, s);
    if (r != 0) {
        pltf_log("ERROR LOADING SND: %i\n", r);
    }
    return r;
}
