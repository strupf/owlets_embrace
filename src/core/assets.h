// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

#include "aud.h"
#include "gfx.h"
#include "util/lzss.h"
#include "util/marena.h"

enum {
    ASSETS_ERR_MISC      = 1 << 0,
    ASSETS_ERR_WAD_INIT  = 1 << 1,
    ASSETS_ERR_WAD_OPEN  = 1 << 2,
    ASSETS_ERR_WAD_CLOSE = 1 << 3,
    ASSETS_ERR_WAD_READ  = 1 << 4,
    ASSETS_ERR_WAD_EL    = 1 << 5,
    ASSETS_ERR_ALLOC     = 1 << 6,
    ASSETS_ERR_TEX       = 1 << 7,
    ASSETS_ERR_SND       = 1 << 8,
    ASSETS_ERR_FNT       = 1 << 9,
    ASSETS_ERR_ANI       = 1 << 10,
};

enum {
    TEXID_DISPLAY,
    TEXID_DISPLAY_TMP,
    TEXID_DISPLAY_TMP_MASK,
    TEXID_BUTTONS,
    TEXID_HERO,
    TEXID_COMPANION,
    TEXID_TILESET_TERRAIN,
    TEXID_TILESET_BG_AUTO,
    TEXID_TILESET_PROPS,
    TEXID_TILESET_DECO,
    TEXID_TILESET_FRONT,
    TEXID_BG_PARALLAX,
    TEXID_BG_PARALLAX_PERF,
    TEXID_PAUSE_TEX,
    TEXID_COVER,
    TEXID_UI,
    TEXID_PLANTS,
    TEXID_BOULDER,
    TEXID_FLSURF,
    TEXID_MUSHROOM,
    TEXID_DRILLER,
    TEXID_SOLIDLEVER,
    TEXID_WATERCOL,
    TEXID_EXPLO1,
    TEXID_UPGRADE,
    TEXID_PARTICLES,
    TEXID_SWITCH,
    TEXID_CRAWLER,
    TEXID_ROOTS,
    TEXID_MISCOBJ,
    TEXID_HOOK,
    TEXID_MOLE,
    TEXID_GEMS,
    TEXID_WIND,
    TEXID_FEATHERUPGR,
    TEXID_SAVEPOINT,
    TEXID_CRUMBLE,
    TEXID_NPC,
    TEXID_TOGGLE,
    TEXID_ROTOR,
    TEXID_FLYER,
    TEXID_WINDGUSH,
    TEXID_COLLISION_TILES,
    TEXID_FLYING_BUG,
    TEXID_BUDPLANT,
    TEXID_FLYBLOB,
    TEXID_EXPLOSIONS,
    TEXID_FLUIDS,
    TEXID_JUMPER,
    TEXID_FOREGROUND,
    TEXID_CHEST,
    TEXID_HEARTDROP,
    TEXID_TRAMPOLINE,
    TEXID_STALACTITE,
    TEXID_STAMINARESTORE,
    TEXID_BOSSPLANT,
    //
    NUM_TEXID_EXPLICIT,
    //
    NUM_TEXID = 96
};

enum {
    FNTID_SMALL,
    FNTID_MEDIUM,
    FNTID_LARGE,
    FNTID_VLARGE,
    FNTID_VVLARGE,
    //
    NUM_FNTID
};

enum {
    SNDID_DEFAULT,
    SNDID_HOOK_ATTACH,
    SNDID_KLONG,
    SNDID_SPEAK0,
    SNDID_SPEAK1,
    SNDID_SPEAK2,
    SNDID_SPEAK3,
    SNDID_SPEAK4,
    SNDID_SWITCH,
    SNDID_SWOOSH,
    SNDID_HIT_ENEMY,
    SNDID_JUMP,
    SNDID_COIN,
    SNDID_ENEMY_HURT,
    SNDID_CRUMBLE_BREAKING,
    SNDID_CRUMBLE_BREAK,
    SNDID_DOOR_KEY_SPAWNED,
    SNDID_DOOR_UNLOCKED,
    SNDID_UPGRADE,
    SNDID_BOSSWIN,
    SNDID_HOOK_THROW,
    SNDID_FOOTSTEP_LEAVES,
    SNDID_FOOTSTEP_GRASS,
    SNDID_FOOTSTEP_MUD,
    SNDID_FOOTSTEP_SAND,
    SNDID_FOOTSTEP_DIRT,
    SNDID_WING,
    SNDID_WING_BIG,
    SNDID_WATER_SPLASH_BIG,
    SNDID_WATER_SPLASH_SMALL,
    SNDID_WATER_SWIM_1,
    SNDID_WATER_SWIM_2,
    SNDID_WATER_OUT_OF,
    SNDID_STOMP,
    SNDID_SKID,
    SNDID_EXPLO1,
    SNDID_PROJECTILE_SPIT,
    SNDID_PROJECTILE_WALL,
    SNDID_SPEAR_ATTACK,
    SNDID_STOMP_START,
    SNDID_STOMP_LAND,
    SNDID_ENEMY_EXPLO,
    SNDID_LANDING,
    SNDID_STOPSPRINT,
    SNDID_BPLANT_SWOOSH,
    SNDID_BPLANT_SHOW,
    SNDID_BPLANT_HIDE,
    SNDID_EXPLOPOOF,
    SNDID_HURT,
    SNDID_MENU1,
    SNDID_MENU2,
    SNDID_MENU3,
    SNDID_PLANTPULSE,
    SNDID_RUMBLE,
    SNDID_JUMPON,
    SNDID_ENEMY_DIE,
    SNDID_WINGATTACK,
    //
    NUM_SNDID
};

enum {
    ANIID_HERO_ATTACK,
    ANIID_HERO_ATTACK_AIR,
    ANIID_COMPANION_FLY,
    ANIID_COMPANION_ATTACK,
    ANIID_COMPANION_BUMP,
    ANIID_COMPANION_HUH,
    ANIID_GEMS,
    ANIID_BUTTON,
    ANIID_UPGRADE,
    ANIID_CURSOR,
    ANIID_HEALTHDROP,
    ANIID_BPLANT_HOP,
    ANIID_PREPARE_SWAP,
    ANIID_FBLOB_ATTACK,
    ANIID_MOLE_DIG_OUT,
    ANIID_MOLE_DIG_IN,
    //
    NUM_ANIID
};

typedef struct {
    ALIGNAS(2)
    u8 i; // image index
    u8 t; // number of ticks
} ani_frame_s;

typedef struct {
    ALIGNAS(8)
    ani_frame_s *f;
    u16          ticks; // total length in ticks
    u16          n;     // number of frames
} ani_s;

typedef struct {
    tex_s            tex[NUM_TEXID];
    fnt_s            fnt[NUM_FNTID];
    ani_s            ani[NUM_ANIID];
    ALIGNAS(8) snd_s snd[NUM_SNDID];
} assets_s;

i32      assets_init();
tex_s   *asset_texptr(i32 ID);
tex_s    asset_tex(i32 ID);
snd_s    asset_snd(i32 ID);
fnt_s    asset_fnt(i32 ID);
ani_s    asset_ani(i32 ID);
i32      asset_tex_put(tex_s t);
tex_s    asset_tex_putID(i32 ID, tex_s t);
texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h);
i32      snd_play(i32 ID, f32 vol, f32 pitch);
i32      snd_play_ext(i32 ID, f32 vol, f32 pitch, bool32 loop);
//
err32    snd_from_wad_ID(i32 ID, const void *name, allocator_s a);
err32    tex_from_wad_ID(i32 ID, const void *name, allocator_s a);
err32    tex_from_wad_ID_ext(void *f, i32 ID, const void *name,
                             allocator_s a);
err32    tex_from_wad_ext(const void *name, allocator_s a, tex_s *o_t);

err32 tex_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, tex_s *o_t);
err32 snd_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, snd_s *o_s);
err32 ani_from_wad(void *f, wad_el_s *wf, const void *name,
                   allocator_s a, ani_s *o_a);
i32   ani_frame(i32 ID, i32 ticks);
i32   ani_len(i32 ID);

#endif