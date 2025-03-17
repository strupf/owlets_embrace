// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "area/area.h"
#include "boss/battleroom.h"
#include "boss/boss.h"
#include "cam.h"
#include "coins.h"
#include "dialog.h"
#include "fluid_area.h"
#include "gamedef.h"
#include "gameover.h"
#include "grapplinghook.h"
#include "hero.h"
#include "hero_powerup.h"
#include "map_loader.h"
#include "maptransition.h"
#include "minimap.h"
#include "obj.h"
#include "particle.h"
#include "particle_defs.h"
#include "rope.h"
#include "save.h"
#include "settings.h"
#include "steering.h"
#include "tile_map.h"
#include "title.h"
#include "wiggle.h"

#define SAVE_TICKS          100
#define SAVE_TICKS_FADE_OUT 80

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_DEATH      = 1 << 2,
    EVENT_HERO_HOOK_PAUSE = 1 << 3,
};

// for (obj_each(g, o)) {}
#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    IT;                           \
    IT = IT->next

enum {
    SUBSTATE_NONE,
    SUBSTATE_TEXTBOX,
    SUBSTATE_MAPTRANSITION,
    SUBSTATE_GAMEOVER,
    SUBSTATE_POWERUP,
    SUBSTATE_MENUSCREEN,
    SUBSTATE_CAMERA_PAN,
};

enum {
    TRIGGER_BATTLEROOM_ENTER = 10000,
    TRIGGER_BATTLEROOM_LEAVE = 10001,
};

enum {
    RENDER_PRIO_BACKGROUND            = 8,
    RENDER_PRIO_BEHIND_TERRAIN_LAYER  = 16,
    RENDER_PRIO_HERO                  = 24,
    RENDER_PRIO_INFRONT_FLUID_AREA    = 32,
    RENDER_PRIO_INFRONT_TERRAIN_LAYER = 40,
    //
    RENDER_PRIO_DEFAULT_OBJ           = RENDER_PRIO_HERO - 1,
};

typedef struct {
    u32 hash;
    i16 x;
    i16 y;
    u16 w;
    u16 h;
} map_room_wad_s;

typedef struct {
    u32   hash;
    i16   x;
    i16   y;
    u16   w;
    u16   h;
    tex_s t;
} map_room_s;

typedef struct {
    ALIGNAS(4)
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} map_door_s;

typedef struct {
    ALIGNAS(4)
    u16 x;
    u16 w;
} map_pit_s;

enum {
    HITBOX_TMP_CIR,
    HITBOX_TMP_REC,
};

typedef struct {
    i16 type;
    i16 x;
    i16 y;
    i16 cir_r;
    i16 rec_w;
    i16 rec_h;
} hitbox_tmp_s;

struct g_s {
    i32             save_slot;
    i32             tick;
    i32             tick_animation;
    u32             map_hash;
    i16             n_map_rooms;
    u8              n_map_doors;
    u8              n_map_pits;
    map_room_s     *map_rooms; // permanently allocated
    map_room_s     *map_room_cur;
    map_door_s      map_doors[16];
    map_pit_s       map_pits[16];
    v2_i32          cam_prev;
    v2_i32          cam_prev_world;
    area_s          area;
    //
    gameover_s      gameover;
    maptransition_s maptransition;
    hero_powerup_s  powerup;
    dialog_s        dialog;
    grapplinghook_s ghook;
    coins_s         coins;
    minimap_s       minimap;
    cam_s           cam;
    u32             events_frame; // flags
    u32             hero_hurt_lp_tick;
    u8              freeze_tick;
    u8              substate;
    bool8           dark;
    bool8           block_hero_control;
    u8              hero_hitID;
    u8              musicname[8];
    u8              mapname[32];
    u32             enemies_killed;
    i32             tiles_x;
    i32             tiles_y;
    i32             pixel_x;
    i32             pixel_y;
    tile_s          tiles[NUM_TILES];
    u16             rtiles[NUM_TILELAYER][NUM_TILES];
    u8              fluid_streams[NUM_TILES];
    i32             n_hitbox_tmp;
    hitbox_tmp_s    hitbox_tmp[16];
    obj_s          *obj_head_busy; // linked list
    obj_s          *obj_head_free; // linked list
    obj_s          *obj_tag[NUM_OBJ_TAGS];
    u32             obj_ndelete;
    obj_s          *obj_to_delete[NUM_OBJ];
    bool16          objrender_dirty; // resort render list?
    u16             n_objrender;
    obj_s          *obj_render[NUM_OBJ]; // sorted render array
    obj_s           obj_raw[NUM_OBJ];
    boss_s          boss;
    battleroom_s    battleroom;
    u16             save_ticks;
    u32             n_grass;
    grass_s         grass[NUM_GRASS];
    u32             n_deco_verlet;
    deco_verlet_s   deco_verlet[NUM_DECO_VERLET];
    i32             n_fluid_areas;
    fluid_area_s    fluid_areas[16];
    i32             ui_fade_q16;
    hero_s          hero;
    particle_sys_s  particle_sys;
    u32             save_events[NUM_SAVE_EV / 32];
    marena_s        memarena;
    byte            mem[MKILOBYTE(512)];
};

void        game_init(g_s *g);
void        game_tick(g_s *g);
void        game_anim(g_s *g);
void        game_draw(g_s *g);
void        game_resume(g_s *g);
void        game_paused(g_s *g);
void       *game_alloc(g_s *g, usize s, usize alignment);
allocator_s game_allocator(g_s *g);
#define game_alloct(G, T)     (T *)game_alloc(G, sizeof(T), ALIGNOF(T))
#define game_alloctn(G, T, N) (T *)game_alloc(G, (N) * sizeof(T), ALIGNOF(T))

i32    gameplay_time(g_s *g);
i32    gameplay_time_since(g_s *g, i32 t);
void   game_load_savefile(g_s *g);
bool32 game_save_savefile(g_s *g);
void   game_on_trigger(g_s *g, i32 trigger);
void   game_on_solid_appear(g_s *g);
bool32 hero_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32 hero_attackbox(g_s *g, hitbox_s box);
void   objs_animate(g_s *g);
void   game_on_solid_appear_ext(g_s *g, obj_s *s);
void   obj_interact(g_s *g, obj_s *o, obj_s *ohero);
void   game_open_map(void *ctx, i32 opt);
void   game_unlock_map(g_s *g); // play cool cutscene and stuff later, too
void   hitbox_tmp_cir(g_s *g, i32 x, i32 y, i32 r);
i32    game_hero_hitID_next(g_s *g);

// returns a number [0, n_frames-1]
// tick is the time variable
// freqticks is how many ticks are needed for one loop
i32 tick_to_index_freq(i32 tick, i32 n_frames, i32 freqticks);

#define NUM_FRAME_TICKS 64

typedef struct {
    u8 ticks[NUM_FRAME_TICKS];
} frame_ticks_s;

static i32 anim_frame_from_ticks(i32 ticks, frame_ticks_s *f)
{
    i32 a = 0;
    for (i32 i = 0; i < NUM_FRAME_TICKS; i++) {
        i32 t = f->ticks[i];
        if (t == 0) return (i - 1);
        a += t;
        if (ticks <= a) return i;
    }
    return 0;
}

static i32 anim_total_ticks(frame_ticks_s *f)
{
    i32 time = 0;
    for (i32 i = 0; i < NUM_FRAME_TICKS; i++) {
        i32 t = f->ticks[i];
        if (t == 0) break;
        time += t;
    }
    return time;
}

static inline i32 gfx_spr_flip_rng(bool32 x, bool32 y)
{
    i32 res = (rngr_u32(0, x != 0) ? SPR_FLIP_X : 0) |
              (rngr_u32(0, y != 0) ? SPR_FLIP_Y : 0);
    return res;
}

#endif