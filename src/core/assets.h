// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

#include "core/aud.h"
#include "core/gfx.h"
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
    ASSETS_ERR_SFX       = 1 << 8,
    ASSETS_ERR_FNT       = 1 << 9,
    ASSETS_ERR_ANI       = 1 << 10,
};

enum {
    TEXID_DISPLAY,
    TEXID_DISPLAY_TMP,
    TEXID_DISPLAY_TMP_MASK,
    TEXID_DISPLAY_WHITE_OUTLINED,
    TEXID_BUTTONS,
    TEXID_HERO,
    TEXID_COMPANION,
    TEXID_TILESET_TERRAIN,
    TEXID_TILESET_BG_AUTO,
    TEXID_TILESET_PROPS,
    TEXID_TILESET_DECO,
    TEXID_TILESET_FRONT,
    TEXID_BG_PARALLAX,
    TEXID_PAUSE_TEX,
    TEXID_COVER,
    TEXID_UI,
    TEXID_PLANTS,
    TEXID_BOULDER,
    TEXID_FLSURF,
    TEXID_MUSHROOM,
    TEXID_DRILLER,
    TEXID_SOLIDLEVER,
    TEXID_VINES,
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
    TEXID_LOOKAHEAD,
    TEXID_CRUMBLE,
    TEXID_NPC,
    TEXID_TOGGLE,
    TEXID_ROTOR,
    TEXID_FLYER,
    TEXID_WINDGUSH,
    TEXID_FLYING_BUG,
    TEXID_BUDPLANT,
    TEXID_FLYBLOB,
    TEXID_FLUIDS,
    TEXID_JUMPER,
    TEXID_FOREGROUND,
    TEXID_FROG,
    TEXID_CHEST,
    TEXID_HEARTDROP,
    TEXID_TRAMPOLINE,
    TEXID_STALACTITE,
    TEXID_STAMINARESTORE,
    TEXID_BOSS,
    TEXID_USECRANK,
    TEXID_SAVEROOM,
    TEXID_CRAB,
    TEXID_EXPLOSIONS,
    TEXID_ANIM_MISC,
    TEXID_BOMBPLANT,
    //
    NUM_TEXID
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
    SFXID_DEFAULT,
    SFXID_HOOK_ATTACH,
    SFXID_KLONG,
    SFXID_SPEAK0,
    SFXID_SPEAK1,
    SFXID_SPEAK2,
    SFXID_SPEAK3,
    SFXID_SPEAK4,
    SFXID_SWITCH,
    SFXID_SWOOSH,
    SFXID_HIT_ENEMY,
    SFXID_JUMP,
    SFXID_COIN,
    SFXID_ENEMY_HURT,
    SFXID_CRUMBLE_BREAKING,
    SFXID_CRUMBLE_BREAK,
    SFXID_DOOR_KEY_SPAWNED,
    SFXID_DOOR_UNLOCKED,
    SFXID_UPGRADE,
    SFXID_BOSSWIN,
    SFXID_HOOK_THROW,
    SFXID_FOOTSTEP_LEAVES,
    SFXID_FOOTSTEP_GRASS,
    SFXID_FOOTSTEP_MUD,
    SFXID_FOOTSTEP_SAND,
    SFXID_FOOTSTEP_DIRT,
    SFXID_WING,
    SFXID_WING_BIG,
    SFXID_WATER_SPLASH_BIG,
    SFXID_WATER_SPLASH_SMALL,
    SFXID_WATER_SWIM_1,
    SFXID_WATER_SWIM_2,
    SFXID_WATER_OUT_OF,
    SFXID_STOMP,
    SFXID_SKID,
    SFXID_EXPLO1,
    SFXID_PROJECTILE_SPIT,
    SFXID_PROJECTILE_WALL,
    SFXID_SPEAR_ATTACK,
    SFXID_STOMP_START,
    SFXID_STOMP_LAND,
    SFXID_ENEMY_EXPLO,
    SFXID_LANDING,
    SFXID_STOPSPRINT,
    SFXID_BPLANT_SWOOSH,
    SFXID_BPLANT_SHOW,
    SFXID_BPLANT_HIDE,
    SFXID_EXPLOPOOF,
    SFXID_HURT,
    SFXID_MENU1,
    SFXID_MENU2,
    SFXID_MENU3,
    SFXID_PLANTPULSE,
    SFXID_RUMBLE,
    SFXID_JUMPON,
    SFXID_ENEMY_DIE,
    SFXID_WINGATTACK,
    //
    NUM_SFXID
};

enum {
    ANIID_OWL_ATTACK,
    ANIID_OWL_ATTACK_UP,
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
    ANIID_FBLOB_LAND_GROUND,
    ANIID_FBLOB_POP,
    ANIID_FBLOB_REGROW,
    ANIID_FBLOB_GROUND_IDLE,
    ANIID_MOLE_DIG_OUT,
    ANIID_MOLE_DIG_IN,
    ANIID_LOOKAHEAD,
    ANIID_FALLASLEEP,
    ANIID_WAKEUP,
    ANIID_FROG_WALK,
    ANIID_FROG_PREPARE,
    ANIID_CRAB_ATTACK,
    ANIID_EXPLOSION_1,
    ANIID_EXPLOSION_2,
    ANIID_EXPLOSION_3,
    ANIID_EXPLOSION_4,
    ANIID_EXPLOSION_5,
    ANIID_ENEMY_SPAWN_1,
    ANIID_STOMP_PARTICLE,
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
    tex_s tex[NUM_TEXID];
    fnt_s fnt[NUM_FNTID];
    ani_s ani[NUM_ANIID];
    sfx_s sfx[NUM_SFXID];
} assets_s;

extern assets_s g_ASSETS;

tex_s    asset_tex(i32 ID);
sfx_s    asset_sfx(i32 ID);
fnt_s    asset_fnt(i32 ID);
ani_s    asset_ani(i32 ID);
tex_s   *asset_texptr(i32 ID);
texrec_s asset_texrec_from_tex(i32 ID);
texrec_s asset_texrec(i32 ID, i32 x, i32 y, i32 w, i32 h);
//
err32    sfx_from_wad_ID(i32 ID, const void *name, allocator_s a);
err32    tex_from_wad_ID(i32 ID, const void *name, allocator_s a);
err32    tex_from_wad_ID_ext(void *f, i32 ID, const void *name, allocator_s a);
err32    tex_from_wad_ext(const void *name, allocator_s a, tex_s *o_t);
err32    tex_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, tex_s *o_t);
err32    sfx_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, sfx_s *o_s);
err32    ani_from_wad(void *f, wad_el_s *wf, const void *name, allocator_s a, ani_s *o_a);
err32    tex_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, tex_s *o_t);
err32    sfx_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, sfx_s *o_s);
err32    ani_from_wadh(void *f, wad_el_s *wf, u32 hash, allocator_s a, ani_s *o_a);
i32      ani_frame_loop(i32 ID, i32 ticks); // loops through the animation
i32      ani_frame(i32 ID, i32 ticks);      // once; returns -1 if completed
i32      ani_len(i32 ID);

#endif