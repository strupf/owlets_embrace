// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

#include "aud.h"
#include "gfx.h"

enum {
    TEXID_DISPLAY,
    TEXID_OCEAN,
    TEXID_HERO,
    TEXID_HERO_WHIP,
    TEXID_TILESET_TERRAIN,
    TEXID_TILESET_PROPS_BG,
    TEXID_TILESET_PROPS_FG,
    TEXID_UI,
    TEXID_UI_ITEM_CACHE,
    TEXID_UI_ITEMS,
    TEXID_UI_TEXTBOX,
    TEXID_PLANTS,
    TEXID_CLOUDS,
    TEXID_TITLE,
    TEXID_PROPS,
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
    TEXID_CARRIER,
    TEXID_CRUMBLE,
    TEXID_NPC,
    TEXID_WINDGUSH,
    TEXID_JUGGERNAUT,
    TEXID_WATER_PRERENDER,
    TEXID_TOGGLE,
//
#ifdef SYS_DEBUG
    TEXID_COLLISION_TILES,
#endif
    //
    NUM_TEXID,
    //
    NUM_TEXID_MAX = 256
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
    //
    NUM_SNDID = 256
};

typedef struct {
    tex_s tex;
    char  file[32];
} asset_tex_s;

typedef struct {
    snd_s snd;
} asset_snd_s;

typedef struct {
    fnt_s fnt;
} asset_fnt_s;

typedef struct {
    asset_tex_s tex[NUM_TEXID_MAX];
    asset_snd_s snd[NUM_SNDID];
    asset_fnt_s fnt[NUM_FNTID];

    int next_texID;

    marena_s             marena;
    align_CL mkilobyte_s mem[5 * 1024];

    usize size_snd;
    usize size_tex;
} ASSETS_s;

extern ASSETS_s      ASSETS;
extern const alloc_s asset_allocator;

void  assets_init();
//
void *assetmem_alloc(usize s);
tex_s asset_tex(int ID);
snd_s asset_snd(int ID);
fnt_s asset_fnt(int ID);
int   asset_tex_load(const char *filename, tex_s *tex);
int   asset_tex_loadID(int ID, const char *filename, tex_s *tex);
int   asset_snd_loadID(int ID, const char *filename, snd_s *snd);
int   asset_fnt_loadID(int ID, const char *filename, fnt_s *fnt);
int   asset_tex_put(tex_s t);
tex_s asset_tex_putID(int ID, tex_s t);

texrec_s asset_texrec(int ID, int x, int y, int w, int h);

void   snd_play_ext(int ID, f32 vol, f32 pitch);
void   mus_fade_to(const char *filename, int ticks_out, int ticks_in);
void   mus_stop();
bool32 mus_playing();

#endif