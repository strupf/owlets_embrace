// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app_load.h"
#include "app.h"

static err32 app_load_tex_new(app_load_s *l, i32 ID, i32 w, i32 h, i32 fmt);
static err32 app_load_tex(app_load_s *l, i32 ID, void *name);
static err32 app_load_fnt(app_load_s *l, i32 ID, void *name);
static err32 app_load_sfx(app_load_s *l, i32 ID, void *name);
static err32 app_load_ani(app_load_s *l, i32 ID, void *name);
static err32 app_load_texh(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_fnth(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_sfxh(app_load_s *l, i32 ID, u32 wad_hash);
static err32 app_load_anih(app_load_s *l, i32 ID, u32 wad_hash);

// subdivide loading into a switch statement
// enables smoother loading
i32 app_load_task(app_load_s *l, i32 state)
{
    switch (state) {
    case 0: return __LINE__ + 1; // return "first" task if state = 0
    case __LINE__: app_load_fnt(l, FNTID_SMALL, "F_SMALL"); break;
    case __LINE__: app_load_fnt(l, FNTID_MEDIUM, "F_MEDIUM"); break;
    case __LINE__: app_load_fnt(l, FNTID_LARGE, "F_LARGE"); break;
    case __LINE__: app_load_fnt(l, FNTID_VLARGE, "F_VLARGE"); break;
    case __LINE__: app_load_fnt(l, FNTID_VVLARGE, "F_VVLARGE"); break;

    case __LINE__: app_load_sfx(l, SFXID_JUMP, "S_JUMP01"); break;
    case __LINE__: app_load_sfx(l, SFXID_WING, "S_DOUBLEJUMP5"); break;
    case __LINE__: app_load_sfx(l, SFXID_KLONG, "S_HOOK5"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAR_ATTACK, "S_SPEAR_ATTACK"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAK0, "S_SPEAK0"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAK1, "S_SPEAK1"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAK2, "S_SPEAK2"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAK3, "S_SPEAK3"); break;
    case __LINE__: app_load_sfx(l, SFXID_SPEAK4, "S_SPEAK4"); break;
    case __LINE__: app_load_sfx(l, SFXID_STOMP_LAND, "S_STOMP"); break;
    case __LINE__: app_load_sfx(l, SFXID_STOMP_START, "S_STOMP_START"); break;
    case __LINE__: app_load_sfx(l, SFXID_ENEMY_EXPLO, "S_ENEMY_EXPLO"); break;
    case __LINE__: app_load_sfx(l, SFXID_STOPSPRINT, "S_STOPSPRINT"); break;
    case __LINE__: app_load_sfx(l, SFXID_LANDING, "S_LANDING"); break;
    case __LINE__: app_load_sfx(l, SFXID_COIN, "S_COINX"); break;
    case __LINE__: app_load_sfx(l, SFXID_MENU1, "S_MENU1"); break;
    case __LINE__: app_load_sfx(l, SFXID_MENU2, "S_MENU2"); break;
    case __LINE__: app_load_sfx(l, SFXID_MENU3, "S_MENU3"); break;
    case __LINE__: app_load_sfx(l, SFXID_HOOK_THROW, "S_THROWHOOK"); break;
    case __LINE__: app_load_sfx(l, SFXID_PLANTPULSE, "S_PLANTPULSE"); break;
    case __LINE__: app_load_sfx(l, SFXID_RUMBLE, "S_RUMBLE"); break;
    case __LINE__: app_load_sfx(l, SFXID_EXPLO1, "S_EXPLO1"); break;
    case __LINE__: app_load_sfx(l, SFXID_BPLANT_SWOOSH, "S_BPLANT_SWOOSH"); break;
    case __LINE__: app_load_sfx(l, SFXID_BPLANT_SHOW, "S_BPLANT_SHOW"); break;
    case __LINE__: app_load_sfx(l, SFXID_BPLANT_HIDE, "S_BPLANT_HIDE"); break;
    case __LINE__: app_load_sfx(l, SFXID_EXPLOPOOF, "S_EXPLOPOOF"); break;
    case __LINE__: app_load_sfx(l, SFXID_WATER_SPLASH_BIG, "S_SPLASH_BIG"); break;
    case __LINE__: app_load_sfx(l, SFXID_WATER_OUT_OF, "S_SPLASH_OUT"); break;
    case __LINE__: app_load_sfx(l, SFXID_WATER_SWIM_1, "S_SWIM_1"); break;
    case __LINE__: app_load_sfx(l, SFXID_WATER_SWIM_2, "S_SWIM_2"); break;
    case __LINE__: app_load_sfx(l, SFXID_JUMPON, "S_JUMPON"); break;
    case __LINE__: app_load_sfx(l, SFXID_ENEMY_DIE, "S_ENEMY_DIE"); break;
    case __LINE__: app_load_sfx(l, SFXID_HURT, "S_HURT"); break;
    case __LINE__: app_load_sfx(l, SFXID_BOSSWIN, "S_BOSSWIN"); break;
    case __LINE__: app_load_sfx(l, SFXID_WINGATTACK, "S_WINGATTACK"); break;

    case __LINE__: app_load_ani(l, ANIID_OWL_ATTACK, "A_OWL_ATTACK"); break;
    case __LINE__: app_load_ani(l, ANIID_OWL_ATTACK_UP, "A_OWL_ATTACK_UP"); break;
    case __LINE__: app_load_ani(l, ANIID_OWL_ATTACK_DOWN, "A_OWL_ATTACK_DOWN"); break;
    case __LINE__: app_load_ani(l, ANIID_COMPANION_ATTACK, "A_COMP_ATTACK"); break;
    case __LINE__: app_load_ani(l, ANIID_COMPANION_FLY, "A_COMP_FLY"); break;
    case __LINE__: app_load_ani(l, ANIID_COMPANION_BUMP, "A_COMP_BUMP"); break;
    case __LINE__: app_load_ani(l, ANIID_COMPANION_HUH, "A_COMP_HUH"); break;
    case __LINE__: app_load_ani(l, ANIID_GEMS, "A_GEMS"); break;
    case __LINE__: app_load_ani(l, ANIID_BUTTON, "A_BUTTON"); break;
    case __LINE__: app_load_ani(l, ANIID_UPGRADE, "A_UPGRADE"); break;
    case __LINE__: app_load_ani(l, ANIID_CURSOR, "A_CURSOR"); break;
    case __LINE__: app_load_ani(l, ANIID_HEALTHDROP, "A_HEALTHDROP"); break;
    case __LINE__: app_load_ani(l, ANIID_BPLANT_HOP, "A_BPLANT_HOP"); break;
    case __LINE__: app_load_ani(l, ANIID_PREPARE_SWAP, "A_PREPARE_SWAP"); break;
    case __LINE__: app_load_ani(l, ANIID_FBLOB_ATTACK, "A_FBLOB_ATTACK"); break;
    case __LINE__: app_load_ani(l, ANIID_FBLOB_LAND_GROUND, "A_FBLOB_LAND_GROUND"); break;
    case __LINE__: app_load_ani(l, ANIID_FBLOB_POP, "A_FBLOB_POP"); break;
    case __LINE__: app_load_ani(l, ANIID_FBLOB_REGROW, "A_FBLOB_REGROW"); break;
    case __LINE__: app_load_ani(l, ANIID_FBLOB_GROUND_IDLE, "A_FBLOB_GROUND_IDLE"); break;
    case __LINE__: app_load_ani(l, ANIID_MOLE_DIG_OUT, "A_MOLE_DIG_OUT"); break;
    case __LINE__: app_load_ani(l, ANIID_MOLE_DIG_IN, "A_MOLE_DIG_IN"); break;
    case __LINE__: app_load_ani(l, ANIID_LOOKAHEAD, "A_LOOKAHEAD"); break;
    case __LINE__: app_load_ani(l, ANIID_FALLASLEEP, "A_FALLASLEEP"); break;
    case __LINE__: app_load_ani(l, ANIID_WAKEUP, "A_WAKEUP"); break;
    case __LINE__: app_load_ani(l, ANIID_FROG_WALK, "A_FROG_WALK"); break;
    case __LINE__: app_load_ani(l, ANIID_FROG_PREPARE, "A_FROG_PREPARE"); break;
    case __LINE__: app_load_ani(l, ANIID_CRAB_ATTACK, "A_CRAB_ATTACK"); break;
    case __LINE__: app_load_ani(l, ANIID_EXPLOSION_1, "A_EXPLOSION_1"); break;
    case __LINE__: app_load_ani(l, ANIID_EXPLOSION_2, "A_EXPLOSION_2"); break;
    case __LINE__: app_load_ani(l, ANIID_EXPLOSION_3, "A_EXPLOSION_3"); break;
    case __LINE__: app_load_ani(l, ANIID_EXPLOSION_4, "A_EXPLOSION_4"); break;
    case __LINE__: app_load_ani(l, ANIID_EXPLOSION_5, "A_EXPLOSION_5"); break;
    case __LINE__: app_load_ani(l, ANIID_STOMP_PARTICLE, "A_STOMP_PARTICLE"); break;
    case __LINE__: app_load_ani(l, ANIID_ENEMY_SPAWN_1, "A_ENEMY_SPAWN"); break;

    case __LINE__: app_load_tex_new(l, TEXID_DISPLAY_TMP, 400, 240, 0); break;
    case __LINE__: app_load_tex_new(l, TEXID_DISPLAY_TMP_MASK, 400, 240, 0); break;
    case __LINE__: app_load_tex_new(l, TEXID_DISPLAY_WHITE_OUTLINED, 400, 240, 1); break;
    case __LINE__: app_load_tex(l, TEXID_HERO, "T_HERO"); break;
    case __LINE__: app_load_tex(l, TEXID_ROTOR, "T_ROTOR"); break;
    case __LINE__: app_load_tex(l, TEXID_FLSURF, "T_FLSURF"); break;
    case __LINE__: app_load_tex(l, TEXID_MUSHROOM, "T_MUSHROOM"); break;
    case __LINE__: app_load_tex(l, TEXID_FLYBLOB, "T_FBLOB"); break;
    case __LINE__: app_load_tex(l, TEXID_SOLIDLEVER, "T_SOLIDLEVER"); break;
    case __LINE__: app_load_tex(l, TEXID_JUMPER, "T_JUMPER"); break;
    case __LINE__: app_load_tex(l, TEXID_MISCOBJ, "T_MISC"); break;
    case __LINE__: app_load_tex(l, TEXID_PARTICLES, "T_PARTICLES"); break;
    case __LINE__: app_load_tex(l, TEXID_TRAMPOLINE, "T_TRAMPOLINE"); break;
    case __LINE__: app_load_tex(l, TEXID_CRAWLER, "T_CRAWLER"); break;
    case __LINE__: app_load_tex(l, TEXID_BUDPLANT, "T_BUDPLANT"); break;
    case __LINE__: app_load_tex(l, TEXID_CHEST, "T_CHEST"); break;
    case __LINE__: app_load_tex(l, TEXID_STAMINARESTORE, "T_STAMINA"); break;
    case __LINE__: app_load_tex(l, TEXID_HOOK, "T_HOOK"); break;
    case __LINE__: app_load_tex(l, TEXID_EXPLO1, "T_EXPLO1"); break;
    case __LINE__: app_load_tex(l, TEXID_UI, "T_UI_EL"); break;
    case __LINE__: app_load_tex(l, TEXID_COMPANION, "T_COMPANION"); break;
    case __LINE__: app_load_tex(l, TEXID_GEMS, "T_GEMS"); break;
    case __LINE__: app_load_tex(l, TEXID_FLUIDS, "T_FLUIDS"); break;
    case __LINE__: app_load_tex(l, TEXID_BOULDER, "T_BOULDER"); break;
    case __LINE__: app_load_tex(l, TEXID_HEARTDROP, "T_HEARTDROP"); break;
    case __LINE__: app_load_tex(l, TEXID_PLANTS, "T_PLANTS"); break;
    case __LINE__: app_load_tex(l, TEXID_FLYING_BUG, "T_FLYBUG"); break;
    case __LINE__: app_load_tex(l, TEXID_WIND, "T_WIND"); break;
    case __LINE__: app_load_tex(l, TEXID_FOREGROUND, "T_FOREGROUND"); break;
    case __LINE__: app_load_tex(l, TEXID_FEATHERUPGR, "T_FEATHERUPGR"); break;
    case __LINE__: app_load_tex(l, TEXID_DRILLER, "T_DRILLER"); break;
    case __LINE__: app_load_tex(l, TEXID_ROOTS, "T_ROOT"); break;
    case __LINE__: app_load_tex(l, TEXID_MOLE, "T_MOLE"); break;
    case __LINE__: app_load_tex(l, TEXID_LOOKAHEAD, "T_LOOKAHEAD"); break;
    case __LINE__: app_load_tex(l, TEXID_FROG, "T_FROG"); break;
    case __LINE__: app_load_tex(l, TEXID_USECRANK, "T_USECRANK"); break;
    case __LINE__: app_load_tex(l, TEXID_SAVEROOM, "T_SAVEROOM"); break;
    case __LINE__: app_load_tex(l, TEXID_CRAB, "T_CRAB"); break;
    case __LINE__: app_load_tex(l, TEXID_VINES, "T_VINES"); break;
    case __LINE__: app_load_tex(l, TEXID_EXPLOSIONS, "T_EXPLOSIONS"); break;
    case __LINE__: app_load_tex(l, TEXID_ANIM_MISC, "T_ANIM_MISC"); break;
    case __LINE__: app_load_tex(l, TEXID_BOMBPLANT, "T_BOMBPLANT"); break;
    case __LINE__: app_load_tex(l, TEXID_BUTTONS, "T_BUTTONS"); break;
    case __LINE__: app_load_tex(l, TEXID_TILESET_TERRAIN, "T_TSTERR"); break;
    case __LINE__: app_load_tex(l, TEXID_TILESET_BG_AUTO, "T_TSBGA"); break;
    case __LINE__: app_load_tex(l, TEXID_TILESET_PROPS, "T_TSPROP"); break;
    case __LINE__: app_load_tex(l, TEXID_TILESET_DECO, "T_TSDECO"); break;
    case __LINE__: return 0;      // returns 0 = finished
    case -1: return __LINE__ - 1; // returns last task if argument = -1 = total number
    }
    return state + 1;
}

i32 app_load_taskID_beg(app_load_s *l)
{
    return app_load_task(l, 0);
}

i32 app_load_taskID_end(app_load_s *l)
{
    return app_load_task(l, -1);
}

i32 app_load_scale(app_load_s *l, i32 v)
{
    if (l->state_cur == 0) return v;

    i32 task_beg = app_load_taskID_beg(l);
    i32 task_end = app_load_taskID_end(l);
    return (((l->state_cur - task_beg) * v) / (task_end - task_beg));
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
        g_ASSETS.tex[TEXID_PAUSE_TEX] = tex_create(400, 240, 0, app_allocator(), 0);

        app_load_texh(l, TEXID_COVER, hash_str("T_COVER"));
        l->state_cur = app_load_task(l, 0);
        return 1;
    } else {
        l->err |= ASSETS_ERR_WAD_OPEN;
        return 0;
    }
}

err32 app_load_get_err(app_load_s *l)
{
    return l->err;
}

static err32 app_load_tex_new(app_load_s *l, i32 ID, i32 w, i32 h, i32 fmt)
{
    err32 err        = 0;
    g_ASSETS.tex[ID] = tex_create(w, h, fmt, app_allocator(), &err);
    return err;
}

static err32 app_load_tex(app_load_s *l, i32 ID, void *name)
{
    pltf_log("Load tex: %s\n", (char *)name);
    return app_load_texh(l, ID, hash_str(name));
}

static err32 app_load_fnt(app_load_s *l, i32 ID, void *name)
{
    pltf_log("Load fnt: %s\n", (char *)name);
    return app_load_fnth(l, ID, hash_str(name));
}

static err32 app_load_sfx(app_load_s *l, i32 ID, void *name)
{
    pltf_log("Load sfx: %s\n", (char *)name);
    return app_load_sfxh(l, ID, hash_str(name));
}

static err32 app_load_ani(app_load_s *l, i32 ID, void *name)
{
    pltf_log("Load ani: %s\n", (char *)name);
    return app_load_anih(l, ID, hash_str(name));
}

static err32 app_load_texh(app_load_s *l, i32 ID, u32 wad_hash)
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

static err32 app_load_fnth(app_load_s *l, i32 ID, u32 wad_hash)
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
    usize size_dec = lz_decode_file(l->f, f->t.px);
    if (size != size_dec) {
        err32 err = ASSETS_ERR_FNT | ASSETS_ERR_WAD_READ;
        l->err |= err;
        return err;
    }
    return 0;
}

static err32 app_load_sfxh(app_load_s *l, i32 ID, u32 wad_hash)
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

static err32 app_load_anih(app_load_s *l, i32 ID, u32 wad_hash)
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
