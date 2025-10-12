// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app_load.h"
#include "app.h"

static err32 app_load_tex(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_fnt(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_sfx(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_ani(app_load_s *l, i32 ID, u32 wad_hash);

static void app_load_task_add_tex(app_load_s *l, i32 texID, const void *name);
static void app_load_task_add_sfx(app_load_s *l, i32 sfxID, const void *name);
static void app_load_task_add_fnt(app_load_s *l, i32 fntID, const void *name);
static void app_load_task_add_ani(app_load_s *l, i32 aniID, const void *name);
static void app_load_task_add_func(app_load_s *l, void (*func)());

static void app_load_task_do(app_load_s *l, app_load_task_s *t);

static void app_load_setup_tasks(app_load_s *l)
{
    // compile a list of all resources to be loaded at startup
    // TEX
    app_load_task_add_tex(l, TEXID_TILESET_TERRAIN, "T_TSTERR");
    app_load_task_add_tex(l, TEXID_TILESET_BG_AUTO, "T_TSBGA");
    app_load_task_add_tex(l, TEXID_TILESET_PROPS, "T_TSPROP");
    app_load_task_add_tex(l, TEXID_TILESET_DECO, "T_TSDECO");
    app_load_task_add_tex(l, TEXID_HERO, "T_HERO");
    app_load_task_add_tex(l, TEXID_ROTOR, "T_ROTOR");
    app_load_task_add_tex(l, TEXID_FLSURF, "T_FLSURF");
    app_load_task_add_tex(l, TEXID_MUSHROOM, "T_MUSHROOM");
    app_load_task_add_tex(l, TEXID_FLYBLOB, "T_FBLOB");
    app_load_task_add_tex(l, TEXID_SOLIDLEVER, "T_SOLIDLEVER");
    app_load_task_add_tex(l, TEXID_JUMPER, "T_JUMPER");
    app_load_task_add_tex(l, TEXID_MISCOBJ, "T_MISC");
    app_load_task_add_tex(l, TEXID_PARTICLES, "T_PARTICLES");
    app_load_task_add_tex(l, TEXID_TRAMPOLINE, "T_TRAMPOLINE");
    app_load_task_add_tex(l, TEXID_CRAWLER, "T_CRAWLER");
    app_load_task_add_tex(l, TEXID_BUDPLANT, "T_BUDPLANT");
    app_load_task_add_tex(l, TEXID_CHEST, "T_CHEST");
    app_load_task_add_tex(l, TEXID_STAMINARESTORE, "T_STAMINA");
    app_load_task_add_tex(l, TEXID_HOOK, "T_HOOK");
    app_load_task_add_tex(l, TEXID_EXPLO1, "T_EXPLO1");
    app_load_task_add_tex(l, TEXID_UI, "T_UI_EL");
    app_load_task_add_tex(l, TEXID_BUTTONS, "T_BUTTONS");
    app_load_task_add_tex(l, TEXID_COMPANION, "T_COMPANION");
    app_load_task_add_tex(l, TEXID_GEMS, "T_GEMS");
    app_load_task_add_tex(l, TEXID_FLUIDS, "T_FLUIDS");
    app_load_task_add_tex(l, TEXID_BOULDER, "T_BOULDER");
    app_load_task_add_tex(l, TEXID_HEARTDROP, "T_HEARTDROP");
    app_load_task_add_tex(l, TEXID_PLANTS, "T_PLANTS");
    app_load_task_add_tex(l, TEXID_FLYING_BUG, "T_FLYBUG");
    app_load_task_add_tex(l, TEXID_WIND, "T_WIND");
    app_load_task_add_tex(l, TEXID_FOREGROUND, "T_FOREGROUND");
    app_load_task_add_tex(l, TEXID_FEATHERUPGR, "T_FEATHERUPGR");
    app_load_task_add_tex(l, TEXID_DRILLER, "T_DRILLER");
    app_load_task_add_tex(l, TEXID_ROOTS, "T_ROOT");
    app_load_task_add_tex(l, TEXID_MOLE, "T_MOLE");
    app_load_task_add_tex(l, TEXID_LOOKAHEAD, "T_LOOKAHEAD");
    app_load_task_add_tex(l, TEXID_FROG, "T_FROG");
    app_load_task_add_tex(l, TEXID_USECRANK, "T_USECRANK");
    app_load_task_add_tex(l, TEXID_SAVEROOM, "T_SAVEROOM");
    app_load_task_add_tex(l, TEXID_CRAB, "T_CRAB");
    app_load_task_add_tex(l, TEXID_VINES, "T_VINES");
    app_load_task_add_tex(l, TEXID_EXPLOSIONS, "T_EXPLOSIONS");
    app_load_task_add_tex(l, TEXID_ANIM_MISC, "T_ANIM_MISC");
    app_load_task_add_tex(l, TEXID_BOMBPLANT, "T_BOMBPLANT");
    //
    // FONTS
    app_load_task_add_fnt(l, FNTID_SMALL, "F_SMALL");
    app_load_task_add_fnt(l, FNTID_MEDIUM, "F_MEDIUM");
    app_load_task_add_fnt(l, FNTID_LARGE, "F_LARGE");
    app_load_task_add_fnt(l, FNTID_VLARGE, "F_VLARGE");
    app_load_task_add_fnt(l, FNTID_VVLARGE, "F_VVLARGE");
    //
    // SFX
    app_load_task_add_sfx(l, SFXID_JUMP, "S_JUMP01");
    app_load_task_add_sfx(l, SFXID_WING, "S_DOUBLEJUMP5");
    app_load_task_add_sfx(l, SFXID_KLONG, "S_HOOK5");
    app_load_task_add_sfx(l, SFXID_SPEAR_ATTACK, "S_SPEAR_ATTACK");
    app_load_task_add_sfx(l, SFXID_SPEAK0, "S_SPEAK0");
    app_load_task_add_sfx(l, SFXID_SPEAK1, "S_SPEAK1");
    app_load_task_add_sfx(l, SFXID_SPEAK2, "S_SPEAK2");
    app_load_task_add_sfx(l, SFXID_SPEAK3, "S_SPEAK3");
    app_load_task_add_sfx(l, SFXID_SPEAK4, "S_SPEAK4");
    app_load_task_add_sfx(l, SFXID_STOMP_LAND, "S_STOMP");
    app_load_task_add_sfx(l, SFXID_STOMP_START, "S_STOMP_START");
    app_load_task_add_sfx(l, SFXID_ENEMY_EXPLO, "S_ENEMY_EXPLO");
    app_load_task_add_sfx(l, SFXID_STOPSPRINT, "S_STOPSPRINT");
    app_load_task_add_sfx(l, SFXID_LANDING, "S_LANDING");
    app_load_task_add_sfx(l, SFXID_COIN, "S_COINX");
    app_load_task_add_sfx(l, SFXID_MENU1, "S_MENU1");
    app_load_task_add_sfx(l, SFXID_MENU2, "S_MENU2");
    app_load_task_add_sfx(l, SFXID_MENU3, "S_MENU3");
    app_load_task_add_sfx(l, SFXID_HOOK_THROW, "S_THROWHOOK");
    app_load_task_add_sfx(l, SFXID_PLANTPULSE, "S_PLANTPULSE");
    app_load_task_add_sfx(l, SFXID_RUMBLE, "S_RUMBLE");
    app_load_task_add_sfx(l, SFXID_EXPLO1, "S_EXPLO1");
    app_load_task_add_sfx(l, SFXID_BPLANT_SWOOSH, "S_BPLANT_SWOOSH");
    app_load_task_add_sfx(l, SFXID_BPLANT_SHOW, "S_BPLANT_SHOW");
    app_load_task_add_sfx(l, SFXID_BPLANT_HIDE, "S_BPLANT_HIDE");
    app_load_task_add_sfx(l, SFXID_EXPLOPOOF, "S_EXPLOPOOF");
    app_load_task_add_sfx(l, SFXID_WATER_SPLASH_BIG, "S_SPLASH_BIG");
    app_load_task_add_sfx(l, SFXID_WATER_OUT_OF, "S_SPLASH_OUT");
    app_load_task_add_sfx(l, SFXID_WATER_SWIM_1, "S_SWIM_1");
    app_load_task_add_sfx(l, SFXID_WATER_SWIM_2, "S_SWIM_2");
    app_load_task_add_sfx(l, SFXID_JUMPON, "S_JUMPON");
    app_load_task_add_sfx(l, SFXID_ENEMY_DIE, "S_ENEMY_DIE");
    app_load_task_add_sfx(l, SFXID_HURT, "S_HURT");
    app_load_task_add_sfx(l, SFXID_BOSSWIN, "S_BOSSWIN");
    app_load_task_add_sfx(l, SFXID_WINGATTACK, "S_WINGATTACK");
    //
    // ANIMATION
    app_load_task_add_ani(l, ANIID_OWL_ATTACK, "A_OWL_ATTACK");
    app_load_task_add_ani(l, ANIID_OWL_ATTACK_UP, "A_OWL_ATTACK_UP");
    app_load_task_add_ani(l, ANIID_COMPANION_ATTACK, "A_COMP_ATTACK");
    app_load_task_add_ani(l, ANIID_COMPANION_FLY, "A_COMP_FLY");
    app_load_task_add_ani(l, ANIID_COMPANION_BUMP, "A_COMP_BUMP");
    app_load_task_add_ani(l, ANIID_COMPANION_HUH, "A_COMP_HUH");
    app_load_task_add_ani(l, ANIID_GEMS, "A_GEMS");
    app_load_task_add_ani(l, ANIID_BUTTON, "A_BUTTON");
    app_load_task_add_ani(l, ANIID_UPGRADE, "A_UPGRADE");
    app_load_task_add_ani(l, ANIID_CURSOR, "A_CURSOR");
    app_load_task_add_ani(l, ANIID_HEALTHDROP, "A_HEALTHDROP");
    app_load_task_add_ani(l, ANIID_BPLANT_HOP, "A_BPLANT_HOP");
    app_load_task_add_ani(l, ANIID_PREPARE_SWAP, "A_PREPARE_SWAP");
    app_load_task_add_ani(l, ANIID_FBLOB_ATTACK, "A_FBLOB_ATTACK");
    app_load_task_add_ani(l, ANIID_FBLOB_LAND_GROUND, "A_FBLOB_LAND_GROUND");
    app_load_task_add_ani(l, ANIID_FBLOB_POP, "A_FBLOB_POP");
    app_load_task_add_ani(l, ANIID_FBLOB_REGROW, "A_FBLOB_REGROW");
    app_load_task_add_ani(l, ANIID_FBLOB_GROUND_IDLE, "A_FBLOB_GROUND_IDLE");
    app_load_task_add_ani(l, ANIID_MOLE_DIG_OUT, "A_MOLE_DIG_OUT");
    app_load_task_add_ani(l, ANIID_MOLE_DIG_IN, "A_MOLE_DIG_IN");
    app_load_task_add_ani(l, ANIID_LOOKAHEAD, "A_LOOKAHEAD");
    app_load_task_add_ani(l, ANIID_FALLASLEEP, "A_FALLASLEEP");
    app_load_task_add_ani(l, ANIID_WAKEUP, "A_WAKEUP");
    app_load_task_add_ani(l, ANIID_FROG_WALK, "A_FROG_WALK");
    app_load_task_add_ani(l, ANIID_FROG_PREPARE, "A_FROG_PREPARE");
    app_load_task_add_ani(l, ANIID_CRAB_ATTACK, "A_CRAB_ATTACK");
    app_load_task_add_ani(l, ANIID_EXPLOSION_1, "A_EXPLOSION_1");
    app_load_task_add_ani(l, ANIID_EXPLOSION_2, "A_EXPLOSION_2");
    app_load_task_add_ani(l, ANIID_EXPLOSION_3, "A_EXPLOSION_3");
    app_load_task_add_ani(l, ANIID_EXPLOSION_4, "A_EXPLOSION_4");
    app_load_task_add_ani(l, ANIID_EXPLOSION_5, "A_EXPLOSION_5");
    app_load_task_add_ani(l, ANIID_STOMP_PARTICLE, "A_STOMP_PARTICLE");
    app_load_task_add_ani(l, ANIID_ENEMY_SPAWN_1, "A_ENEMY_SPAWN");
}

bool32 app_load_init(app_load_s *l)
{
    DEBUG_LOG("LOAD: APP_LOAD starting...\n");
    g_ASSETS.tex[TEXID_DISPLAY] = tex_framebuffer();

    g_s      *g = &APP.game;
    void     *f = 0;
    wad_el_s *e = 0;
    if (wad_open_str("WAD_MAIN", &f, &e)) {
        l->f = f;

        // load resources which have custom initialization, and
        // also which have to be available right away for the loading screen
        g_ASSETS.tex[TEXID_PAUSE_TEX]              = tex_create(400, 240, 0, app_allocator(), 0);
        g_ASSETS.tex[TEXID_DISPLAY_TMP]            = tex_create(400, 240, 0, app_allocator(), 0);
        g_ASSETS.tex[TEXID_DISPLAY_TMP_MASK]       = tex_create(400, 240, 1, app_allocator(), 0);
        g_ASSETS.tex[TEXID_DISPLAY_WHITE_OUTLINED] = tex_create(400, 240, 1, app_allocator(), 0);
        app_load_tex(l, TEXID_COVER, hash_str("T_COVER"));
        app_load_setup_tasks(l);
        pltf_log("tasks: %i\n", l->n_task_total);
        return 1;
    } else {
        l->err |= ASSETS_ERR_WAD_OPEN;
        return 0;
    }
}

bool32 app_load_tasks_timed(app_load_s *l, i32 millis_max)
{
    if (l->n_task == l->n_task_total) return 1;

    f32 t1     = pltf_seconds();
    f32 td_max = (f32)millis_max * 0.001f;

    while ((l->n_task < l->n_task_total) &&
           (millis_max ? (pltf_seconds() - t1) < td_max : 1)) {
        app_load_task_do(l, &l->tasklist[l->n_task++]);
    }

    if (l->n_task == l->n_task_total) {
        if (!pltf_file_close(l->f)) {
            l->err |= ASSETS_ERR_WAD_CLOSE;
        }
        return 1;
    }
    return 0;
}

i32 app_load_progress_mul(app_load_s *l, i32 v)
{
    if (l->n_task_total == 0) return v;
    return (((i32)l->n_task * v) / l->n_task_total);
}

err32 app_load_get_err(app_load_s *l)
{
    return l->err;
}

static void app_load_task_do(app_load_s *l, app_load_task_s *t)
{
#if PLTF_DEBUG
    if (t->name[0] != 0) {
        pltf_log("LOAD asset: %s\n", t->name);
    }
#endif
    switch (t->type) {
    case APP_LOAD_TASK_FUNCTION: {
        t->func();
        break;
    }
    case APP_LOAD_TASK_TEX: {
        app_load_tex(l, t->assetID, t->hash);
        break;
    }
    case APP_LOAD_TASK_SFX: {
        app_load_sfx(l, t->assetID, t->hash);
        break;
    }
    case APP_LOAD_TASK_FNT: {
        app_load_fnt(l, t->assetID, t->hash);
        break;
    }
    case APP_LOAD_TASK_ANI: {
        app_load_ani(l, t->assetID, t->hash);
        break;
    }
    }
}

static void app_load_task_add_tex(app_load_s *l, i32 texID, const void *name)
{
    app_load_task_s t = {0};
    t.type            = APP_LOAD_TASK_TEX;
    t.hash            = hash_str(name);
    t.assetID         = texID;
    DEBUG_CODE(str_cpys(t.name, sizeof(t.name), name));
    l->tasklist[l->n_task_total++] = t;
}

static void app_load_task_add_sfx(app_load_s *l, i32 sfxID, const void *name)
{
    app_load_task_s t = {0};
    t.type            = APP_LOAD_TASK_SFX;
    t.hash            = hash_str(name);
    t.assetID         = sfxID;
    DEBUG_CODE(str_cpys(t.name, sizeof(t.name), name));
    l->tasklist[l->n_task_total++] = t;
}

static void app_load_task_add_fnt(app_load_s *l, i32 fntID, const void *name)
{
    app_load_task_s t = {0};
    t.type            = APP_LOAD_TASK_FNT;
    t.hash            = hash_str(name);
    t.assetID         = fntID;
    DEBUG_CODE(str_cpys(t.name, sizeof(t.name), name));
    l->tasklist[l->n_task_total++] = t;
}

static void app_load_task_add_ani(app_load_s *l, i32 aniID, const void *name)
{
    app_load_task_s t = {0};
    t.type            = APP_LOAD_TASK_ANI;
    t.hash            = hash_str(name);
    t.assetID         = aniID;
    DEBUG_CODE(str_cpys(t.name, sizeof(t.name), name));
    l->tasklist[l->n_task_total++] = t;
}

static void app_load_task_add_func(app_load_s *l, void (*func)())
{
    app_load_task_s t              = {0};
    t.type                         = APP_LOAD_TASK_FUNCTION;
    t.func                         = func;
    l->tasklist[l->n_task_total++] = t;
}

static err32 app_load_tex(app_load_s *l, i32 ID, u32 wad_hash)
{
    tex_s *t     = &g_ASSETS.tex[ID];
    err32  err_t = tex_from_wadh(l->f, 0, wad_hash, app_allocator(), t);
    if (err_t) {
        err32 err = err_t | ASSETS_ERR_TEX;
        l->err |= err;
        return err;
    }
    return 0;
}

typedef struct {
    u16 n_kerning;
    u8  tracking;
    u8  grid_w;
    u8  grid_h;
} fnt_header_s;

static err32 app_load_fnt(app_load_s *l, i32 ID, u32 wad_hash)
{
    fnt_s    *f = &g_ASSETS.fnt[ID];
    wad_el_s *e = wad_seek(l->f, 0, wad_hash);
    if (!e) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_EL;
        l->err |= err;
        return err;
    }

    fnt_header_s header = {0};
    if (!pltf_file_r_checked(l->f, &header, sizeof(fnt_header_s))) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        l->err |= err;
        return err;
    }

    usize fsize = 256 + sizeof(fnt_kerning_s) * header.n_kerning;
    byte *fmem  = (byte *)app_alloc_aligned(fsize, 1);

    if (!fmem) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_ALLOC;
        l->err |= err;
        return err;
    }

    if (!pltf_file_r_checked(l->f, fmem, fsize)) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        l->err |= err;
        return err;
    }

    f->widths    = (u8 *)&fmem[0];
    f->kerning   = (fnt_kerning_s *)&fmem[256];
    f->grid_w    = header.grid_w;
    f->grid_h    = header.grid_h;
    f->tracking  = -header.tracking;
    f->n_kerning = header.n_kerning;

    tex_header_s h = {0};
    if (!pltf_file_r_checked(l->f, &h, sizeof(tex_header_s))) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        l->err |= err;
        return err;
    }

    err32 err_t = 0;
    f->t        = tex_create(h.w, h.h, 1, app_allocator(), &err_t);
    if (err_t) {
        err32 err = err_t | ASSETS_ERR_FNT | ASSETS_ERR_TEX;
        l->err |= err;
        return err;
    }

    usize size     = sizeof(u32) * f->t.wword * f->t.h;
    usize size_dec = lzss_decode_file(l->f, f->t.px);
    if (size != size_dec) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        l->err |= err;
        return err;
    }
    return 0;
}

static err32 app_load_sfx(app_load_s *l, i32 ID, u32 wad_hash)
{
    sfx_s *s     = &g_ASSETS.sfx[ID];
    err32  err_s = sfx_from_wadh(l->f, 0, wad_hash, app_allocator(), s);
    if (err_s) {
        err32 err = err_s | ASSETS_ERR_SFX;
        l->err |= err;
        return err;
    }
    return 0;
}

static err32 app_load_ani(app_load_s *l, i32 ID, u32 wad_hash)
{
    ani_s *a     = &g_ASSETS.ani[ID];
    err32  err_s = ani_from_wadh(l->f, 0, wad_hash, app_allocator(), a);
    if (err_s) {
        err32 err = err_s | ASSETS_ERR_ANI;
        l->err |= err;
        return err;
    }
    return 0;
}
