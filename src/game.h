// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "area/area.h"
#include "cam.h"
#include "coinparticle.h"
#include "gamedef.h"
#include "inventory.h"
#include "lighting.h"
#include "mainmenu.h"
#include "map_loader.h"
#include "obj/behaviour.h"
#include "obj/hero.h"
#include "obj/obj.h"
#include "particle.h"
#include "rope.h"
#include "save.h"
#include "shop.h"
#include "substate.h"
#include "water.h"

#define NUM_TILES         0x40000
#define INTERACTABLE_DIST 32
#define NUM_DECALS        256
#define NUM_WATER         16
#define ENEMY_DECAL_TICK  20
#define NUM_RESPAWNS      8

#define OCEAN_W_WORDS   SYS_DISPLAY_WWORDS
#define OCEAN_W_PX      SYS_DISPLAY_W
#define OCEAN_H_PX      SYS_DISPLAY_H
#define OCEAN_NUM_SPANS 512

typedef struct {
    i16 y;
    i16 w;
} ocean_span_s;

typedef struct {
    bool32       active;
    i32          y;
    i32          y_min; // boundaries of affected wave area
    i32          y_max;
    i32          n_spans;
    ocean_span_s spans[OCEAN_NUM_SPANS];
} ocean_s;

typedef struct {
    tex_s tex;
    u16   sx, sy, sw, sh;
    i32   n_frames;
    u16   t;
    u16   t_max;
} anim_particle_s;

typedef struct {
    v2_i32 pos;
    i32    type;
    i32    x_q8;
    i32    v_q8;
} grass_s;

#define TILE_WATER_MASK 0x80
#define TILE_ICE_MASK   0x40

typedef union {
    struct {
        union {
            struct {
                u8 tx;
                u8 ty;
            };
            u16 u;
        };
        u8 type; // 6 bits for type, 1 bit for water, 1 bit for iced
        u8 collision;
    };
    u32 U;
} tile_s;

typedef union {
    struct {
        u8 tx;
        u8 ty;
    };
    u16 u;
} rtile_s;

typedef struct {
    v2_i32   pos;
    texrec_s t;
    i32      tick;
} enemy_decal_s;

typedef struct {
    v2_i32 pos;
    tex_s  tex;
    i16    x;
    i16    y;
    i16    w;
    i16    h;
    i16    t;
    i16    t_og;
    i32    n_frames;
} sprite_decal_s;

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_HOOK_PAUSE = 1 << 2,
};

#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    IT;                           \
    IT = IT->next

typedef struct item_selector_s {
    bool32 decoupled;
    i32    angle;
    i32    item;
} item_selector_s;

struct game_s {
    mainmenu_s       mainmenu;
    i32              state;
    //
    map_world_s      map_world; // layout of all map files globally
    map_worldroom_s *map_worldroom;
    shop_s           shop;
    area_s           area;
    substate_s       substate;
    //
    cam_s            cam;
    flags32          events_frame;
    //
    i32              n_respawns;
    v2_i32           respawns[8];
    //
    tile_s           tiles[NUM_TILES];
    rtile_s          rtiles[NUM_TILELAYER][NUM_TILES];
    i32              tiles_x;
    i32              tiles_y;
    i32              pixel_x;
    i32              pixel_y;
    i32              obj_ndelete;
    i32              n_objrender;
    bool32           objrender_dirty;
    obj_s           *obj_head_busy; // linked list
    obj_s           *obj_head_free; // linked list
    obj_s           *obj_tag[NUM_OBJ_TAGS];
    obj_s           *obj_to_delete[NUM_OBJ];
    obj_s           *obj_render[NUM_OBJ]; // sorted render array
    obj_s            obj_raw[NUM_OBJ];

    inventory_s   inventory;
    i32           n_enemy_decals;
    enemy_decal_s enemy_decals[16];

    i32            n_sprite_decals;
    sprite_decal_s sprite_decals[256];

    i32            n_coinparticles;
    coinparticle_s coinparticles[NUM_COINPARTICLE];

    hero_s          hero_mem;
    save_s          save;
    item_selector_s item_selector;

    bool32 avoid_flickering;

    struct {
        char filename[LEN_AREA_FILENAME];
        char label[64];
        i32  fadeticks;
    } areaname;

    i32         n_grass;
    grass_s     grass[256];
    particles_s particles;

    ocean_s              ocean;
    marena_s             arena;
    align_CL mkilobyte_s mem[256];
};

extern const int g_pxmask_tab[32 * 16];
extern const i32 tilecolliders[GAME_NUM_TILECOLLIDERS * 6]; // triangles

void            game_init(game_s *g);
void            game_tick(game_s *g);
void            game_draw(game_s *g);
void            game_resume(game_s *g);
void            game_paused(game_s *g);
//
bool32          game_load_savefile(game_s *g);
bool32          game_save_savefile(game_s *g);
void            game_on_trigger(game_s *g, int trigger);
bool32          tiles_hookable(game_s *g, rec_i32 r);
bool32          tiles_solid(game_s *g, rec_i32 r);
bool32          tiles_solid_pt(game_s *g, int x, int y);
bool32          tile_one_way(game_s *g, rec_i32 r);
bool32          game_traversable(game_s *g, rec_i32 r);
bool32          game_traversable_pt(game_s *g, int x, int y);
void            game_on_solid_appear(game_s *g);
sprite_decal_s *sprite_decal_create(game_s *g);
void            game_put_grass(game_s *g, int tx, int ty);
void            obj_game_update(game_s *g, obj_s *o, inp_s inp);
void            obj_game_animate(game_s *g, obj_s *o);
void            obj_game_trigger(game_s *g, obj_s *o, int trigger);
void            obj_game_interact(game_s *g, obj_s *o);
void            obj_game_player_attackbox(game_s *g, hitbox_s box);
void            item_selector_update(item_selector_s *s);
void            item_selector_draw(item_selector_s *s);
bool32          ladder_overlaps_rec(game_s *g, rec_i32 r, v2_i32 *tpos);
//
int             ocean_height(game_s *g, int pixel_x);
int             ocean_render_height(game_s *g, int pixel_x);
int             water_depth_rec(game_s *g, rec_i32 r);
//
alloc_s         game_allocator(game_s *g);
void            backforeground_animate_grass(game_s *g);

// returns a number [0, n_frames-1]
// tick is the time variable
// freqticks is how many ticks are needed for one loop
int    tick_to_index_freq(int tick, int n_frames, int freqticks);
obj_s *obj_closest_interactable(game_s *g, v2_i32 pos);

typedef struct {
    i32 x1, y1, x2, y2;
} bounds_2D_s;

bounds_2D_s game_tilebounds_rec(game_s *g, rec_i32 r);
bounds_2D_s game_tilebounds_pts(game_s *g, v2_i32 p0, v2_i32 p1);
bounds_2D_s game_tilebounds_tri(game_s *g, tri_i32 t);

#define NUM_FRAME_TICKS 64

typedef struct {
    u8 ticks[NUM_FRAME_TICKS];
} frame_ticks_s;

static int anim_frame_from_ticks(int ticks, frame_ticks_s *f)
{
    int a = 0;
    for (int i = 0; i < NUM_FRAME_TICKS; i++) {
        int t = f->ticks[i];
        if (t == 0) return (i - 1);
        a += t;
        if (ticks <= a) return i;
    }
    return 0;
}

static int anim_total_ticks(frame_ticks_s *f)
{
    int time = 0;
    for (int i = 0; i < NUM_FRAME_TICKS; i++) {
        int t = f->ticks[i];
        if (t == 0) break;
        time += t;
    }
    return time;
}

#endif