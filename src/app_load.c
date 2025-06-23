// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"

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
    g_s      *g = &APP.game;
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
    app_load_tex_internal(&l, TEXID_FLSURF, "T_FLSURF");
    app_load_tex_internal(&l, TEXID_MUSHROOM, "T_MUSHROOM");
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
    app_load_tex_internal(&l, TEXID_HEARTDROP, "T_HEARTDROP");
    app_load_tex_internal(&l, TEXID_PLANTS, "T_PLANTS");
    app_load_tex_internal(&l, TEXID_FLYING_BUG, "T_FLYBUG");
    app_load_tex_internal(&l, TEXID_WIND, "T_WIND");
    app_load_tex_internal(&l, TEXID_FOREGROUND, "T_FOREGROUND");
    APP.assets.tex[TEXID_PAUSE_TEX] =
        tex_create(400, 240, 0, app_allocator(), 0);

    APP.assets.tex[TEXID_DISPLAY_TMP] =
        tex_create(400, 240, 0, app_allocator(), 0);

    APP.assets.tex[TEXID_DISPLAY_TMP_MASK] =
        tex_create(400, 240, 1, app_allocator(), 0);
    APP.assets.tex[TEXID_BG_PARALLAX_PERF] =
        tex_create(400, 240, 0, app_allocator(), 0);

LOAD_FNT:;
    // FNT ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_FNT");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_SND;
    }
    app_load_fnt_internal(&l, FNTID_SMALL, "F_SMALL");
    app_load_fnt_internal(&l, FNTID_MEDIUM, "F_MEDIUM");
    app_load_fnt_internal(&l, FNTID_LARGE, "F_LARGE");
    app_load_fnt_internal(&l, FNTID_VLARGE, "F_VLARGE");
    app_load_fnt_internal(&l, FNTID_VVLARGE, "F_VVLARGE");

LOAD_SND:;
    // SND ---------------------------------------------------------------------
    l.e = wad_seek_str(f, e, "WAD_SFX");
    if (!l.e) {
        l.err |= ASSETS_ERR_WAD_EL;
        goto LOAD_ANI;
    }
    app_load_snd_internal(&l, SNDID_JUMP, "S_JUMP01");
    app_load_snd_internal(&l, SNDID_WING, "S_DOUBLEJUMP5");
    app_load_snd_internal(&l, SNDID_KLONG, "S_HOOK5");
    app_load_snd_internal(&l, SNDID_SPEAR_ATTACK, "S_SPEAR_ATTACK");
    app_load_snd_internal(&l, SNDID_SPEAK0, "S_SPEAK0");
    app_load_snd_internal(&l, SNDID_SPEAK1, "S_SPEAK1");
    app_load_snd_internal(&l, SNDID_SPEAK2, "S_SPEAK2");
    app_load_snd_internal(&l, SNDID_SPEAK3, "S_SPEAK3");
    app_load_snd_internal(&l, SNDID_SPEAK4, "S_SPEAK4");
    app_load_snd_internal(&l, SNDID_STOMP_LAND, "S_STOMP");
    app_load_snd_internal(&l, SNDID_STOMP_START, "S_STOMP_START");
    app_load_snd_internal(&l, SNDID_ENEMY_EXPLO, "S_ENEMY_EXPLO");
    app_load_snd_internal(&l, SNDID_STOPSPRINT, "S_STOPSPRINT");
    app_load_snd_internal(&l, SNDID_LANDING, "S_LANDING");
    app_load_snd_internal(&l, SNDID_COIN, "S_COINX");
    app_load_snd_internal(&l, SNDID_MENU1, "S_MENU1");
    app_load_snd_internal(&l, SNDID_MENU2, "S_MENU2");
    app_load_snd_internal(&l, SNDID_MENU3, "S_MENU3");
    app_load_snd_internal(&l, SNDID_HOOK_THROW, "S_THROWHOOK");
    app_load_snd_internal(&l, SNDID_PLANTPULSE, "S_PLANTPULSE");
    app_load_snd_internal(&l, SNDID_RUMBLE, "S_RUMBLE");
    app_load_snd_internal(&l, SNDID_EXPLO1, "S_EXPLO1");
    app_load_snd_internal(&l, SNDID_BPLANT_SWOOSH, "S_BPLANT_SWOOSH");
    app_load_snd_internal(&l, SNDID_BPLANT_SHOW, "S_BPLANT_SHOW");
    app_load_snd_internal(&l, SNDID_BPLANT_HIDE, "S_BPLANT_HIDE");
    app_load_snd_internal(&l, SNDID_EXPLOPOOF, "S_EXPLOPOOF");
    app_load_snd_internal(&l, SNDID_WATER_SPLASH_BIG, "S_SPLASH_BIG");
    app_load_snd_internal(&l, SNDID_WATER_OUT_OF, "S_SPLASH_OUT");
    app_load_snd_internal(&l, SNDID_WATER_SWIM_1, "S_SWIM_1");
    app_load_snd_internal(&l, SNDID_WATER_SWIM_2, "S_SWIM_2");
    app_load_snd_internal(&l, SNDID_JUMPON, "S_JUMPON");
    app_load_snd_internal(&l, SNDID_ENEMY_DIE, "S_ENEMY_DIE");
    app_load_snd_internal(&l, SNDID_HURT, "S_HURT");
    app_load_snd_internal(&l, SNDID_BOSSWIN, "S_BOSSWIN");
    app_load_snd_internal(&l, SNDID_WINGATTACK, "S_WINGATTACK");

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
    app_load_ani_internal(&l, ANIID_COMPANION_HUH, "A_COMP_HUH");
    app_load_ani_internal(&l, ANIID_GEMS, "A_GEMS");
    app_load_ani_internal(&l, ANIID_BUTTON, "A_BUTTON");
    app_load_ani_internal(&l, ANIID_UPGRADE, "A_UPGRADE");
    app_load_ani_internal(&l, ANIID_CURSOR, "A_CURSOR");
    app_load_ani_internal(&l, ANIID_HEALTHDROP, "A_HEALTHDROP");
    app_load_ani_internal(&l, ANIID_BPLANT_HOP, "A_BPLANT_HOP");
    app_load_ani_internal(&l, ANIID_PREPARE_SWAP, "A_PREPARE_SWAP");
    app_load_ani_internal(&l, ANIID_FBLOB_ATTACK, "A_FBLOB_ATTACK");

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

    tex_s *t     = &APP.assets.tex[ID];
    err32  err_t = tex_from_wad(l->f, l->e, name, l->allocator, t);
    if (err_t) {
        l->err |= err_t | ASSETS_ERR_TEX;
        pltf_log("ERROR LOADING TEX: %i | %s\n", err_t, (const char *)name);
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

    fnt_s    *f = &APP.assets.fnt[ID];
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
    byte *fmem  = (byte *)l->allocator.allocfunc(l->allocator.ctx, fsize, 1);

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

    snd_s *s     = &APP.assets.snd[ID];
    err32  err_s = snd_from_wad(l->f, l->e, name, l->allocator, s);
    if (err_s) {
        l->err |= err_s | ASSETS_ERR_SND;
        pltf_log("ERROR LOADING SND: %i | %s\n", err_s, (const char *)name);
    }
}

static void app_load_ani_internal(app_load_s *l, i32 ID, const void *name)
{
    ani_s *a     = &APP.assets.ani[ID];
    err32  err_s = ani_from_wad(l->f, l->e, name, l->allocator, a);
    if (err_s) {
        l->err |= err_s | ASSETS_ERR_ANI;
        pltf_log("ERROR LOADING ANI: %i | %s\n", err_s, (const char *)name);
    }
}