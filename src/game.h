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
#include "dia.h"
#include "fluid_area.h"
#include "gamedef.h"
#include "hitbox.h"
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

enum {
    AREANAME_ST_INACTIVE,
    AREANAME_ST_DELAY,
    AREANAME_ST_FADE_IN,
    AREANAME_ST_SHOW,
    AREANAME_ST_FADE_OUT
};

enum {
    GAME_FLAG_TITLE_PREVIEW      = 1 << 0,
    GAME_FLAG_BLOCK_UPDATE       = 1 << 1,
    GAME_FLAG_BLOCK_PLAYER_INPUT = 1 << 2,
    GAME_FLAG_SPEEDRUN           = 1 << 3,
};

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_DEATH      = 1 << 2,
    EVENT_HERO_HOOK_PAUSE = 1 << 3,
};

#if 1 // prefetch the next one?
static inline obj_s *obj_each_valid(obj_s *o)
{
    // already prefetch the next one because of iteration
    // -> likely to access the next one shortly after
    if (o) {
        PREFETCH(o->next);
    }
    return o;
}
#else
#define obj_each_valid(O) O
#endif

// for (obj_each(g, o)) {}
#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    obj_each_valid(IT);           \
    IT = IT->next

#define obj_each_objID(G, IT, OID)      \
    obj_s *IT = obj_find_ID(G, OID, 0); \
    obj_each_valid(IT);                 \
    IT = obj_find_ID(G, OID, IT)

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

typedef struct map_room_wad_s {
    ALIGNAS(4)
    u8  map_name[MAP_WAD_NAME_LEN]; // 12
    i16 x;                          // 2
    i16 y;                          // 2
    u16 w;                          // 2
    u16 h;                          // 2
    u8  musID;                      // 1
    u8  flags;                      // 1
    u8  unused[2];                  // 2
} map_room_wad_s;

enum {
    MAP_ROOM_FLAG_NOT_MAPPED = 1 << 0,
};

typedef struct map_room_s {
    tex_s t;
    u8    map_name[MAP_WAD_NAME_LEN]; // 12
    i16   x;                          // 2
    i16   y;                          // 2
    u16   w;                          // 2
    u16   h;                          // 2
    u8    musID;                      // 1
    u8    flags;                      // 1
    u8    unused[2];                  // 2
} map_room_s;

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

struct g_s {
    savefile_s *savefile;
    inp_s       inp;            // current input state
    i32         tick;           // total playtime of this save
    i32         tick_animation; // incremented every frame, reset between rooms
    i32         tick_gameplay;  // incremented every logical gameplay frame
    u32         flags;
    u8          save_slot;
    u8          freeze_tick;
    u8          health_ui_fade;
    bool8       health_ui_show;
    bool8       dark;
    u8          owl_hitID;

    b8          map_is_mapped; // appears on minimap
    i16         n_map_rooms;
    map_room_s *map_room_cur;
    map_room_s *map_room_cur_mod;
    map_room_s  map_rooms[256];
    u8          map_name[MAP_WAD_NAME_LEN];     // technical name of the room in editor (untransformed)
    u8          map_name_mod[MAP_WAD_NAME_LEN]; // technical name of the room in editor

    v2_i32          cam_center; // set in the beginning of the animation tick before drawing
    grapplinghook_s ghook;
    coins_s         coins;
    minimap_s       minimap;
    owl_s           owl;
    cam_s           cam;
    cs_s            cs;
    dia_s           dia;
    background_s    background;
    u32             events_frame; // flags
    u16             hurt_lp_tick;
    u8              music_ID;
    u8              area_ID;
    u8              vfx_ID;

    ALIGNAS(32)
    u8  areaname[32];
    u8  area_anim_tick;
    u8  area_anim_st;
    u32 enemies_killed;
    u32 enemy_killed[NUM_ENEMYID];

    ALIGNAS(8)
    u16 tiles_x;
    u16 tiles_y;
    u16 pixel_x;
    u16 pixel_y;

    ALIGNAS(32)
    tile_s tiles[NUM_TILES];
    ALIGNAS(32)
    u16 rtiles[NUM_TILELAYER][NUM_TILES];
    ALIGNAS(32)
    u8 fluid_streams[NUM_TILES];

    ALIGNAS(16)
    obj_s *obj_head_busy; // linked list
    obj_s *obj_head_free; // linked list
    obj_s *obj_tag[NUM_OBJ_TAGS];
    u32    obj_ndelete;
    obj_s *obj_to_delete[NUM_OBJ];
    bool16 objrender_dirty; // resort render list?
    u16    n_objrender;
    obj_s *obj_render[NUM_OBJ]; // sorted render array
    obj_s  obj_raw[NUM_OBJ];
    void  *vfx_area_mem;

    ALIGNAS(8)
    void *map_objs;
    void *map_objs_end;

    u32             hitboxUID; // incremented per hitbox; always bigger than 0
    i32             n_hitboxes;
    hitbox_s        hitboxes[HITBOX_NUM];
    boss_s          boss;
    battleroom_s    battleroom;
    i16             n_fg;
    foreground_el_s fg_el[256];
    u32             n_grass;
    grass_s         grass[NUM_GRASS];
    u32             n_deco_verlet;
    deco_verlet_s   deco_verlet[NUM_DECO_VERLET];
    i32             n_fluid_areas;
    fluid_area_s    fluid_areas[16];
    i32             n_ropes;
    wire_s         *ropes[4];
    particle_sys_s  particle_sys;
    i32             n_save_points;
    v2_i32          save_points[8];
    u32             saveIDs[NUM_SAVEID_WORDS];

    marena_s memarena;
    ALIGNAS(32)
    byte mem[MKILOBYTE(2048)];
};

static inline map_obj_s *map_obj_next(map_obj_s *mo)
{
    return (map_obj_s *)((byte *)mo + ((i32)mo->size_words << 2));
}

static inline bool32 map_obj_each_has_next(g_s *g, map_obj_s *mo_it)
{
    if ((byte *)mo_it < (byte *)g->map_objs_end) {
        PREFETCH(map_obj_next(mo_it));
        return 1;
    }
    return 0;
}

#define map_obj_each(G, IT)                   \
    map_obj_s *IT = (map_obj_s *)G->map_objs; \
    map_obj_each_has_next(G, IT);             \
    IT = map_obj_next(IT)

void        game_init(g_s *g);
void        game_tick(g_s *g, inp_state_s inpstate);
void        game_anim(g_s *g);
void        game_draw(g_s *g);
void        game_resume(g_s *g);
void        game_paused(g_s *g);
void       *game_alloc_room(g_s *g, usize s, usize alignment);
allocator_s game_allocator_room(g_s *g);
#define game_alloc_roomt(G, T)     (T *)game_alloc_room(G, sizeof(T), ALIGNOF(T))
#define game_alloc_roomtn(G, T, N) (T *)game_alloc_room(G, (N) * sizeof(T), ALIGNOF(T))

i32         gameplay_time(g_s *g);
i32         gameplay_time_since(g_s *g, i32 t);
void        game_load_savefile(g_s *g);
void        game_update_savefile(g_s *g);
bool32      game_save_savefile(g_s *g, v2_i32 pos);
void        game_on_trigger(g_s *g, i32 trigger);
void        game_on_solid_appear(g_s *g);
void        objs_animate(g_s *g);
obj_s      *obj_find_ID(g_s *g, i32 objID, obj_s *o);
void        game_on_solid_appear_ext(g_s *g, obj_s *s);
void        game_unlock_map(g_s *g); // play cool cutscene and stuff later, too
i32         game_owl_hitID_next(g_s *g);
void        game_cue_area_music(g_s *g);
map_room_s *map_room_find(g_s *g, b8 transformed, const void *name); // returns the transformed room if any
u8         *map_loader_room_mod(g_s *g, u8 *map_name);               // conditionally load a different room variant

static inline i32 gfx_spr_flip_rng(bool32 x, bool32 y)
{
    i32 res = (rngr_i32(0, x != 0) ? SPR_FLIP_X : 0) |
              (rngr_i32(0, y != 0) ? SPR_FLIP_Y : 0);
    return res;
}

#endif