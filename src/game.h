// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "area/area.h"
#include "cam.h"
#include "gamedef.h"
#include "gameover.h"
#include "heroupgrade.h"
#include "map_loader.h"
#include "maptransition.h"
#include "menu_screen.h"
#include "obj/behaviour.h"
#include "obj/hero.h"
#include "obj/obj.h"
#include "particle.h"
#include "rope.h"
#include "save.h"
#include "shop.h"
#include "textbox.h"
#include "title.h"
#include "water.h"

#define NUM_TILES           0x40000
#define INTERACTABLE_DIST   32
#define NUM_DECALS          256
#define NUM_WATER           16
#define ENEMY_DECAL_TICK    20
#define NUM_RESPAWNS        8
#define OCEAN_W_WORDS       SYS_DISPLAY_WWORDS
#define OCEAN_W_PX          SYS_DISPLAY_W
#define OCEAN_H_PX          SYS_DISPLAY_H
#define OCEAN_NUM_SPANS     512
#define SAVE_TICKS          100
#define SAVE_TICKS_FADE_OUT 80

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

typedef struct {
    texrec_s tr;
    rec_i32  r;
    i32      t;
    v2_i32   offs;
    bool32   overlaps;
} wiggle_deco_s;

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

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_HOOK_PAUSE = 1 << 2,
};

#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    IT;                           \
    IT = IT->next

enum {
    SUBSTATE_NONE,
    SUBSTATE_TEXTBOX,
    SUBSTATE_MAPTRANSITION,
    SUBSTATE_GAMEOVER,
    SUBSTATE_HEROUPGRADE,
    SUBSTATE_MENUSCREEN,
    SUBSTATE_FREEZE,
};

struct game_s {
    title_s          title;
    i32              state;
    //
    map_world_s      map_world; // layout of all map files globally
    map_worldroom_s *map_worldroom;
    area_s           area;
    //
    shop_s           shop;
    gameover_s       gameover;
    heroupgrade_s    heroupgrade;
    maptransition_s  maptransition;
    textbox_s        textbox;
    menu_screen_s    menu_screen;
    u16              freeze_tick;
    u16              substate;
    //
    cam_s            cam;
    flags32          events_frame;
    //

    //
    tile_s         tiles[NUM_TILES];
    rtile_s        rtiles[NUM_TILELAYER][NUM_TILES];
    i32            tiles_x;
    i32            tiles_y;
    i32            pixel_x;
    i32            pixel_y;
    u16            obj_ndelete;
    u16            n_objrender;
    bool32         objrender_dirty;
    obj_s         *obj_head_busy; // linked list
    obj_s         *obj_head_free; // linked list
    obj_s         *obj_tag[NUM_OBJ_TAGS];
    obj_s         *obj_to_delete[NUM_OBJ];
    obj_s         *obj_render[NUM_OBJ]; // sorted render array
    obj_s          obj_raw[NUM_OBJ];
    //
    i16            coins_added;
    u16            coins_added_ticks;
    u16            save_ticks;
    u16            n_grass;
    u16            n_enemy_decals;
    u16            n_wiggle_deco;
    u16            n_coinparticles;
    u16            n_respawns;
    grass_s        grass[256];
    enemy_decal_s  enemy_decals[16];
    wiggle_deco_s  wiggle_deco[64];
    coinparticle_s coinparticles[NUM_COINPARTICLE];
    v2_i32         respawns[8];
    save_s         save;
    hero_s         hero_mem;
    particles_s    particles;
    ocean_s        ocean;

    struct {
        int selected;
        int doubletap_timer;
    } item;

    struct {
        char filename[LEN_AREA_FILENAME];
        char label[64];
        i32  fadeticks;
    } areaname;

    marena_s             arena;
    align_CL mkilobyte_s mem[256];
};

extern const u32 g_pxmask_tab[32 * 16];
extern const i32 tilecolliders[GAME_NUM_TILECOLLIDERS * 6]; // triangles

void    game_init(game_s *g);
void    game_tick(game_s *g);
void    game_draw(game_s *g);
void    game_resume(game_s *g);
void    game_paused(game_s *g);
//
bool32  game_load_savefile(game_s *g);
bool32  game_save_savefile(game_s *g);
void    game_on_trigger(game_s *g, i32 trigger);
bool32  tiles_hookable(game_s *g, rec_i32 r);
bool32  tiles_solid(game_s *g, rec_i32 r);
bool32  tiles_solid_pt(game_s *g, int x, int y);
bool32  tile_one_way(game_s *g, rec_i32 r);
bool32  game_traversable(game_s *g, rec_i32 r);
bool32  game_traversable_pt(game_s *g, int x, int y);
void    game_on_solid_appear(game_s *g);
void    game_put_grass(game_s *g, int tx, int ty);
void    obj_game_player_attackbox(game_s *g, hitbox_s box);
bool32  ladder_overlaps_rec(game_s *g, rec_i32 r, v2_i32 *tpos);
//
int     ocean_height(game_s *g, int pixel_x);
int     ocean_render_height(game_s *g, int pixel_x);
int     water_depth_rec(game_s *g, rec_i32 r);
//
alloc_s game_allocator(game_s *g);
void    backforeground_animate_grass(game_s *g);

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