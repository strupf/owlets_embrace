// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app_load.h"

typedef struct {
    err32       err;
    void       *f;
    wad_el_s   *e;
    allocator_s allocator;
} app_load_s;

static void app_load_tex_internal(app_load_s *l, i32 ID, const void *name);
static void app_load_fnt_internal(app_load_s *l, i32 ID, const void *name);
static void app_load_snd_internal(app_load_s *l, i32 ID, const void *name);
static void app_load_ani_internal(app_load_s *l, i32 ID, const void *name);

err32 app_load_assets()
{
    g_s      *g = &APP->game;
    void     *f = 0;
    wad_el_s *e = 0;
    if (!wad_open_str("WAD_MAIN", &f, &e)) return ASSETS_ERR_WAD_OPEN;

    app_load_s l = {0, f, 0, app_allocator()};

    // TEX ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_TEX");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_FNT;
    }
    app_load_tex_internal(&l, TEXID_TILESET_TERRAIN, "T_TSTERR");
    app_load_tex_internal(&l, TEXID_TILESET_BG_AUTO, "T_TSBGA");
    app_load_tex_internal(&l, TEXID_TILESET_PROPS, "T_TSPROP");
    app_load_tex_internal(&l, TEXID_TILESET_DECO, "T_TSDECO");
    app_load_tex_internal(&l, TEXID_HERO, "T_HERO");
    app_load_tex_internal(&l, TEXID_ROTOR, "T_ROTOR");
    app_load_tex_internal(&l, TEXID_BGOLEM, "T_BGOLEM");
    app_load_tex_internal(&l, TEXID_FLSURF, "T_FLSURF");
    app_load_tex_internal(&l, TEXID_FLYBLOB, "T_FBLOB");
    app_load_tex_internal(&l, TEXID_SOLIDLEVER, "T_SOLIDLEVER");
    app_load_tex_internal(&l, TEXID_JUMPER, "T_JUMPER");
    app_load_tex_internal(&l, TEXID_MISCOBJ, "T_MISC");
    app_load_tex_internal(&l, TEXID_PARTICLES, "T_PARTICLES");
    app_load_tex_internal(&l, TEXID_TRAMPOLINE, "T_TRAMPOLINE");
    app_load_tex_internal(&l, TEXID_CRAWLER, "T_CRAWLER");
    app_load_tex_internal(&l, TEXID_BUDPLANT, "T_BUDPLANT");
    app_load_tex_internal(&l, TEXID_CHEST, "T_CHEST");
    app_load_tex_internal(&l, TEXID_STAMINARESTORE, "T_STAMINA");
    app_load_tex_internal(&l, TEXID_HOOK, "T_HOOK");
    app_load_tex_internal(&l, TEXID_EXPLO1, "T_EXPLO1");
    app_load_tex_internal(&l, TEXID_UI, "T_UI_EL");
    app_load_tex_internal(&l, TEXID_BUTTONS, "T_BUTTONS");
    app_load_tex_internal(&l, TEXID_COMPANION, "T_COMPANION");
    app_load_tex_internal(&l, TEXID_COVER, "T_COVER");
    app_load_tex_internal(&l, TEXID_GEMS, "T_GEMS");
    app_load_tex_internal(&l, TEXID_FLUIDS, "T_FLUIDS");
    app_load_tex_internal(&l, TEXID_BOULDER, "T_BOULDER");
    app_load_tex_internal(&l, TEXID_UPGRADE, "T_UPGRADE");

    {
        tex_s t = tex_create(400, 240, 0, app_allocator(), 0);

        APP->assets.tex[TEXID_PAUSE_TEX] = t;
    }
    {
        tex_s t = tex_create(400, 240, 0, app_allocator(), 0);
        mclr(t.px, t.wword * t.h);
        APP->assets.tex[TEXID_DISPLAY_TMP] = t;
    }

LOAD_FNT:;
    // FNT ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_FNT");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_SND;
    }
    app_load_fnt_internal(&l, FNTID_SMALL, "F_SMALL");
    app_load_fnt_internal(&l, FNTID_MEDIUM, "F_MEDIUM");

LOAD_SND:;
    // SND ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_SFX");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_ANI;
    }
    app_load_snd_internal(&l, SNDID_JUMP, "S_JUMP01");
    app_load_snd_internal(&l, SNDID_KLONG, "S_HOOK");
    app_load_snd_internal(&l, SNDID_SPEAR_ATTACK, "S_SPEAR_ATTACK");
    app_load_snd_internal(&l, SNDID_STOMP_LAND, "S_STOMP");
    app_load_snd_internal(&l, SNDID_STOMP_START, "S_STOMP_START");
    app_load_snd_internal(&l, SNDID_ENEMY_EXPLO, "S_ENEMY_EXPLO");

LOAD_ANI:;
    // ANI ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_ANI");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_DONE;
    }
    app_load_ani_internal(&l, ANIID_HERO_ATTACK, "A_HEROATTACK");
    app_load_ani_internal(&l, ANIID_HERO_ATTACK_AIR, "A_HEROATTACK_AIR");
    app_load_ani_internal(&l, ANIID_COMPANION_ATTACK, "A_COMP_ATTACK");
    app_load_ani_internal(&l, ANIID_COMPANION_FLY, "A_COMP_FLY");
    app_load_ani_internal(&l, ANIID_COMPANION_BUMP, "A_COMP_BUMP");
    app_load_ani_internal(&l, ANIID_GEMS, "A_GEMS");
    app_load_ani_internal(&l, ANIID_BUTTON, "A_BUTTON");
    app_load_ani_internal(&l, ANIID_UPGRADE, "A_UPGRADE");
    app_load_ani_internal(&l, ANIID_CURSOR, "A_CURSOR");

LOAD_DONE:;
    if (!pltf_file_close(l.f)) {
        l.err |= ASSETS_ERR_WAD_CLOSE;
    }

    pltf_log("APP LOAD ERR: %i\n", l.err);
    return l.err;
}

static void app_load_tex_internal(app_load_s *l, i32 ID, const void *name)
{
    // if (l->err) return;

    tex_s *t     = &APP->assets.tex[ID];
    err32  err_t = tex_from_wad(l->f, l->e, name, l->allocator, t);
    if (err_t) {
        l->err |= err_t | ASSETS_ERR_TEX;
        pltf_log("ERROR LOADING TEX: %i\n", err_t);
    }
}

typedef struct {
    u16 n_kerning;
    u8  tracking;
    u8  grid_w;
    u8  grid_h;
} fnt_header_s;

static void app_load_fnt_internal(app_load_s *l, i32 ID, const void *name)
{
    // if (l->err) return;

    fnt_s    *f = &APP->assets.fnt[ID];
    wad_el_s *e = wad_seek_str(l->f, l->e, name);
    if (!e) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_WAD_EL;
        return;
    }

    fnt_header_s header = {0};
    if (!pltf_file_r_checked(l->f, &header, sizeof(fnt_header_s))) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        return;
    }

    usize fsize = 256 + sizeof(fnt_kerning_s) * header.n_kerning;
    char *fmem  = (u8 *)l->allocator.allocfunc(l->allocator.ctx, fsize, 1);

    if (!fmem) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_ALLOC;
        return;
    }

    if (!pltf_file_r_checked(l->f, fmem, fsize)) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        return;
    }

    f->widths    = (u8 *)&fmem[0];
    f->kerning   = (fnt_kerning_s *)&fmem[256];
    f->grid_w    = header.grid_w;
    f->grid_h    = header.grid_h;
    f->tracking  = -header.tracking;
    f->n_kerning = header.n_kerning;

    tex_header_s h = {0};
    if (!pltf_file_r_checked(l->f, &h, sizeof(tex_header_s))) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        return;
    }

    err32 err_t = 0;
    f->t        = tex_create(h.w, h.h, 1, l->allocator, &err_t);
    if (err_t) {
        l->err |= err_t | ASSETS_ERR_FNT | ASSETS_ERR_TEX;
        return;
    }

    usize size     = sizeof(u32) * f->t.wword * f->t.h;
    usize size_dec = lzss_decode_file(l->f, f->t.px);
    if (size != size_dec) {
        l->err |= ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        return;
    }
}

static void app_load_snd_internal(app_load_s *l, i32 ID, const void *name)
{
    // if (l->err) return;

    snd_s *s     = &APP->assets.snd[ID];
    err32  err_s = snd_from_wad(l->f, l->e, name, l->allocator, s);
    if (err_s) {
        l->err |= err_s | ASSETS_ERR_SND;
        pltf_log("ERROR LOADING SND: %i | %s\n", err_s, (const char *)name);
    }
}

static void app_load_ani_internal(app_load_s *l, i32 ID, const void *name)
{
    ani_s *a     = &APP->assets.ani[ID];
    err32  err_s = ani_from_wad(l->f, l->e, name, l->allocator, a);
    if (err_s) {
        l->err |= err_s | ASSETS_ERR_ANI;
        pltf_log("ERROR LOADING ANI: %i\n", err_s);
    }
}