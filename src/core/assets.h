// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

#include "aud.h"
#include "gfx.h"

#define ASSET_FILENAME     "assets/assets/assets.dat"
#define ASSETS_LOG_LOADING 0

#if defined(PLTF_SDL)
#define ASSETS_EXPORT 1
#else
#define ASSETS_EXPORT 0
#endif

enum {
    TEXID_DISPLAY,
    TEXID_KEYBOARD,
    TEXID_HERO,
    TEXID_TILESET_TERRAIN,
    TEXID_TILESET_BG_AUTO,
    TEXID_TILESET_PROPS_BG,
    TEXID_TILESET_PROPS_FG,
    TEXID_PAUSE_TEX,
    TEXID_UI,
    TEXID_PLANTS,
    TEXID_CLOUDS,
    TEXID_TITLE,
    TEXID_SWITCH,
    TEXID_SHROOMY,
    TEXID_CRAWLER,
    TEXID_MISCOBJ,
    TEXID_HOOK,
    TEXID_MAINMENU,
    TEXID_AREALABEL,
    TEXID_UPGRADELABEL,
    TEXID_BG_CAVE,
    TEXID_BG_MOUNTAINS,
    TEXID_BG_DEEP_FOREST,
    TEXID_BG_CAVE_DEEP,
    TEXID_BG_FOREST,
    TEXID_CARRIER,
    TEXID_CRUMBLE,
    TEXID_NPC,
    TEXID_JUGGERNAUT,
    TEXID_WATER_PRERENDER,
    TEXID_TOGGLE,
    TEXID_SKELETON,
    TEXID_CHARGER,
    TEXID_FLYER,
    TEXID_WINDGUSH,
    TEXID_COLLISION_TILES,
    TEXID_FLYING_BUG,
    TEXID_WALLWORM,
    TEXID_TITLE_SCREEN,
    TEXID_BUDPLANT,
    TEXID_FLYBLOB,
    TEXID_EXPLOSIONS,
    TEXID_CRABLER,
    //
    NUM_TEXID_EXPLICIT,
    //
    NUM_TEXID = 96
};

enum {
    FNTID_SMALL,
    FNTID_MEDIUM,
    FNTID_LARGE,
    //
    NUM_FNTID
};

enum {
    SNDID_DEFAULT,
    SNDID_HOOK_ATTACH,
    SNDID_SPEAK,
    SNDID_STEP,
    SNDID_SWITCH,
    SNDID_WHIP,
    SNDID_SWOOSH,
    SNDID_HIT_ENEMY,
    SNDID_SHROOMY_JUMP,
    SNDID_DOOR_SQUEEK,
    SNDID_DOOR_TOGGLE,
    SNDID_ATTACK_DASH,
    SNDID_ATTACK_SLIDE_GROUND,
    SNDID_ATTACK_SLIDE_AIR,
    SNDID_ATTACK_SPIN,
    SNDID_SELECT,
    SNDID_MENU_NEXT_ITEM,
    SNDID_MENU_NONEXT_ITEM,
    SNDID_COIN,
    SNDID_BASIC_ATTACK,
    SNDID_ENEMY_HURT,
    SNDID_ENEMY_DIE,
    SNDID_CRUMBLE_BREAKING,
    SNDID_CRUMBLE_BREAK,
    SNDID_DOOR_KEY_SPAWNED,
    SNDID_DOOR_UNLOCKED,
    SNDID_UPGRADE,
    SNDID_CRUMBLE,
    SNDID_HOOK_THROW,
    SNDID_KB_DENIAL,
    SNDID_KB_KEY,
    SNDID_KB_CLICK,
    SNDID_KB_SELECTION,
    SNDID_KB_SELECTION_REV,
    SNDID_FOOTSTEP_LEAVES,
    SNDID_FOOTSTEP_GRASS,
    SNDID_FOOTSTEP_MUD,
    SNDID_FOOTSTEP_SAND,
    SNDID_FOOTSTEP_DIRT,
    SNDID_OWLET_ATTACK_1,
    SNDID_OWLET_ATTACK_2,
    SNDID_ENEMY_EXPLO,
    SNDID_WING,
    SNDID_WING1,
    SNDID_HOOK_READY,
    //
    NUM_SNDID
};

typedef struct {
    tex_s tex;
} asset_tex_s;

typedef struct {
    snd_s snd;
} asset_snd_s;

typedef struct {
    fnt_s fnt;
} asset_fnt_s;

typedef struct {
    asset_tex_s tex[NUM_TEXID];
    asset_snd_s snd[NUM_SNDID];
    asset_fnt_s fnt[NUM_FNTID];

    marena_s    marena;
    mmegabyte_s mem[6];
} ASSETS_s;

extern ASSETS_s      ASSETS;
extern const alloc_s asset_allocator;

void assets_init();
#if ASSETS_EXPORT
void assets_export();
#endif
void     assets_import();
//
void    *assetmem_alloc(u32 s);
tex_s    asset_tex(i32 ID);
snd_s    asset_snd(i32 ID);
fnt_s    asset_fnt(i32 ID);
i32      asset_tex_load(const char *filename, tex_s *tex);
i32      asset_tex_loadID(i32 ID, const char *filename, tex_s *tex);
i32      asset_snd_loadID(i32 ID, const char *filename, snd_s *snd);
i32      asset_fnt_loadID(i32 ID, const char *filename, fnt_s *fnt);
i32      asset_tex_put(tex_s t);
tex_s    asset_tex_putID(i32 ID, tex_s t);
texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h);
fnt_s    fnt_load(const char *filename, alloc_s ma);

u32    snd_play(i32 ID, f32 vol, f32 pitch);
void   mus_fade_to(const char *filename, i32 ticks_out, i32 ticks_in);
void   mus_stop();
bool32 mus_playing();

#endif