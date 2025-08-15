// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "background.h"
#include "boss/battleroom.h"
#include "boss/boss.h"
#include "cam.h"
#include "coins.h"
#include "cs/cs.h"
#include "dialog.h"
#include "fluid_area.h"
#include "game_trigger.h"
#include "gamedef.h"
#include "map_loader.h"
#include "minimap.h"
#include "obj.h"
#include "obj/puppet.h"
#include "owl/grapplinghook.h"
#include "owl/owl.h"
#include "particle.h"
#include "particle_defs.h"
#include "render.h"
#include "save.h"
#include "settings.h"
#include "steering.h"
#include "tile_map.h"
#include "vfx_area.h"
#include "wiggle.h"
#include "wire.h"

#define GAME_N_ROOMS 256
#define NUM_TILES    65536

enum {
    AREANAME_ST_INACTIVE,
    AREANAME_ST_DELAY,
    AREANAME_ST_FADE_IN,
    AREANAME_ST_SHOW,
    AREANAME_ST_FADE_OUT
};

#define AREANAME_TICKS_DELAY 30
#define AREANAME_TICKS_IN    30
#define AREANAME_TICKS_SHOW  255
#define AREANAME_TICKS_OUT   30
#define HEALTH_UI_TICKS      50

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

#define obj_each_objID(G, IT, OID)      \
    obj_s *IT = obj_find_ID(G, OID, 0); \
    IT;                                 \
    IT = obj_find_ID(G, OID, IT)

#define map_obj_each(G, IT)                   \
    map_obj_s *IT = (map_obj_s *)G->map_objs; \
    (byte *)IT < (byte *)G->map_objs_end;     \
    IT = (map_obj_s *)((byte *)IT + IT->bytes)

enum {
    RENDER_PRIO_BACKGROUND            = 8,
    RENDER_PRIO_BEHIND_TERRAIN_LAYER  = 16,
    RENDER_PRIO_OWL                   = 24,
    RENDER_PRIO_INFRONT_FLUID_AREA    = 32,
    RENDER_PRIO_INFRONT_TERRAIN_LAYER = 40,
    RENDER_PRIO_UI_LEVEL              = 240,
    //
    RENDER_PRIO_DEFAULT_OBJ           = RENDER_PRIO_OWL - 1,
};

typedef struct {
    u8  map_name[MAP_WAD_NAME_LEN];
    u8  flags;
    u8  unused[3];
    i16 x;
    i16 y;
    u16 w;
    u16 h;
} map_room_wad_s;

enum {
    MAP_ROOM_FLAG_NOT_MAPPED = 1 << 0,
};

typedef struct {
    tex_s t;
    u8    map_name[MAP_WAD_NAME_LEN];
    u8    flags;
    u8    unused[3];
    i16   x;
    i16   y;
    u16   w;
    u16   h;
} map_room_s;

typedef struct {
    ALIGNAS(8)
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

typedef struct {
    ALIGNAS(16)
    i16 x;
    i16 y;
    u16 tx;
    u16 ty;
    u16 tw;
    u16 th;
    u8  k_q8; // parallax factor
} foreground_el_s;

typedef struct {
    wire_s     *r;
    wirenode_s *rn_head; // non-null if ropenode is "standalone"/not tied to an obj
    wirenode_s *rn_tail; // non-null if ropenode is "standalone"/not tied to an obj
    void       *ctx;
    void (*on_blocked)(void *ctx, wire_s *r);
    void (*on_pushed_ropenode)(void *ctx, wire_s *r, wirenode_s *rn, i32 sx, i32 sy);
} rope_handle_s;

struct g_s {
    savefile_s     *savefile;
    inp_s           inp;            // current input state
    i32             tick;           // total playtime of this save
    i32             tick_animation; // incremented every frame, reset between rooms
    i32             tick_gameplay;  // incremented every logical gameplay frame
    //
    u8              map_name[MAP_WAD_NAME_LEN]; // technical name of the room in editor
    b8              map_is_mapped;              // appears on minimap
    //
    i16             n_map_rooms;
    i16             n_fg;
    u8              n_map_doors;
    u8              n_map_pits;
    u8              save_slot;
    u8              freeze_tick;
    u8              substate;
    u8              health_ui_fade;
    bool8           health_ui_show;
    bool8           dark;
    bool8           previewmode;
    bool8           block_update;
    bool8           block_owl_control;
    bool8           speedrun;
    bool8           render_map_doors;
    u8              owl_hitID;
    map_room_s      map_rooms[GAME_N_ROOMS];
    map_room_s     *map_room_cur;
    map_door_s      map_doors[16];
    map_pit_s       map_pits[16];
    v2_i32          cam_prev;
    v2_i32          cam_center;
    dialog_s        dialog;
    grapplinghook_s ghook;
    coins_s         coins;
    minimap_s       minimap;
    owl_s           owl;
    cam_s           cam;
    cs_s            cs;
    u32             events_frame; // flags
    u16             hurt_lp_tick;
    u8              music_ID;
    u8              area_ID;
    u8              background_ID;
    u8              vfx_ID;
    u8              areaname[32];
    u8              area_anim_tick;
    u8              area_anim_st;
    i32             enemies_killed;
    i32             tiles_x;
    i32             tiles_y;
    i32             pixel_x;
    i32             pixel_y;
    tile_s          tiles[NUM_TILES];
    u16             rtiles[NUM_TILELAYER][NUM_TILES];
    u8              fluid_streams[NUM_TILES];
    i16             bg_offx;
    i16             bg_offy;
    i16             n_hitbox_tmp;
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
    void           *vfx_area_mem;
    void           *map_objs;
    void           *map_objs_end;
    boss_s          boss;
    battleroom_s    battleroom;
    i16             darken_bg_add;
    i16             darken_bg_q12; // fading background to black
    foreground_el_s fg_el[256];
    u32             n_grass;
    grass_s         grass[NUM_GRASS];
    u32             n_deco_verlet;
    deco_verlet_s   deco_verlet[NUM_DECO_VERLET];
    i32             n_fluid_areas;
    fluid_area_s    fluid_areas[16];
    i32             n_ropes;
    wire_s         *ropes[16];
    particle_sys_s  particle_sys;
    i32             n_save_points;
    v2_i32          save_points[8];
    u32             save_events[(NUM_SAVE_EV + 31) / 32];
    marena_s        memarena;
    byte            mem[MKILOBYTE(2048)];
};

void        game_init(g_s *g);
void        game_tick(g_s *g, inp_state_s inpstate);
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
void   game_update_savefile(g_s *g);
bool32 game_save_savefile(g_s *g, v2_i32 pos);
void   game_on_trigger(g_s *g, i32 trigger);
void   game_on_solid_appear(g_s *g);
bool32 hero_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32 hero_attackbox(g_s *g, hitbox_s box);
void   objs_animate(g_s *g);
obj_s *obj_find_ID(g_s *g, i32 objID, obj_s *o);
void   game_on_solid_appear_ext(g_s *g, obj_s *s);
void   game_unlock_map(g_s *g); // play cool cutscene and stuff later, too
void   hitbox_tmp_cir(g_s *g, i32 x, i32 y, i32 r);
i32    game_owl_hitID_next(g_s *g);
void   game_cue_area_music(g_s *g);
bool32 snd_cam_param(g_s *g, f32 vol_max, v2_i32 pos, i32 r, f32 *vol, f32 *pan);

// positive: fade bg to black
// negative: fade black to bg
void game_darken_bg(g_s *g, i32 speed);

static inline i32 gfx_spr_flip_rng(bool32 x, bool32 y)
{
    i32 res = (rngr_i32(0, x != 0) ? SPR_FLIP_X : 0) |
              (rngr_i32(0, y != 0) ? SPR_FLIP_Y : 0);
    return res;
}

#endif