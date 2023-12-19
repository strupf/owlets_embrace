// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "cam.h"
#include "enveffect.h"
#include "fade.h"
#include "gamedef.h"
#include "mainmenu.h"
#include "map_loader.h"
#include "obj/behaviour.h"
#include "obj/hero.h"
#include "obj/obj.h"
#include "rope.h"
#include "textbox.h"
#include "transition.h"
#include "water.h"

//  25 x 15
//  50 x 30
//  75 x 45
// 100 x 60
//

#define NUM_TILES         0x40000
#define NUM_PARTICLES     256
#define INTERACTABLE_DIST 32
#define NUM_DECALS        256

typedef struct {
    v2_i32 pos;
    int    type;
    int    x_q8;
    int    v_q8;
} grass_s;

enum {
    PARTICLE_GFX_REC,
    PARTICLE_GFX_CIR,
    PARTICLE_GFX_SPR
};

typedef struct {
    v2_i32   p_q8;
    v2_i32   v_q8;
    v2_i32   a_q8;
    i32      ticks;
    i32      ticks_max;
    i32      size;
    i32      gfx;
    texrec_s texrec;
} particle_s;

typedef struct {
    particle_s p;

    v2_i32 pr_q8;
    v2_i32 vr_q8;
    v2_i32 ar_q8;
    int    ticksr;
    int    sizer;
} particle_desc_s;

void particles_spawn(game_s *g, particle_desc_s desc, int n);

enum {
    BG_IMG_POS_TILED,
    BG_IMG_POS_FIT_ROOM,
};

typedef struct {
    // values for Tiled's layer config
    int      img_pos;
    f32      x;
    f32      y;
    f32      offx;
    f32      offy;
    texrec_s tr;
    bool32   loopx;
    bool32   loopy;
} parallax_img_s;

typedef struct {
    u8 type;
    u8 collision;
} tile_s;

typedef union {
    struct {
        u8 tx;
        u8 ty;
    };
    u16 u;
} rtile_s;

typedef struct {
    tex_s tex;
    int   x;
    int   y;
    int   w;
    int   h;
    int   sx;
    int   sy;
} decal_s;

struct game_s {
    int          tick;
    mainmenu_s   mainmenu;
    int          state;
    map_world_s  map_world; // layout of all map files globally
    //
    int          savefile_slotID;
    transition_s transition;
    cam_s        cam;
    tile_s       tiles[NUM_TILES];
    rtile_s      rtiles[NUM_TILELAYER][NUM_TILES];
    int          tiles_x;
    int          tiles_y;
    int          pixel_x;
    int          pixel_y;
    int          obj_nfree;
    int          obj_nbusy;
    int          obj_ndelete;
    obj_s       *obj_tag[NUM_OBJ_TAGS];
    obj_s       *obj_free_stack[NUM_OBJ];
    obj_s       *obj_busy[NUM_OBJ];
    obj_s       *obj_to_delete[NUM_OBJ];
    obj_s        obj_raw[NUM_OBJ];

    hero_s         herodata;
    parallax_img_s parallax;
    rope_s         rope; // hero rope, singleton
    textbox_s      textbox;

    enveffect_wind_s env_wind;
    enveffect_heat_s env_heat;

    rope_s *ropes[4];
    int     n_ropes;

    struct {
        char   filename[LEN_AREA_FILENAME];
        char   label[64];
        fade_s fade;
    } areaname;

    int        n_decal_fg;
    int        n_decal_bg;
    int        n_grass;
    int        n_particle;
    decal_s    decal_fg[NUM_DECALS];
    decal_s    decal_bg[NUM_DECALS];
    grass_s    grass[256];
    particle_s particles[NUM_PARTICLES];

    ocean_s ocean;

    marena_s arena;
    alignas(4) char mem[MKILOBYTE(256)];
};

typedef struct {
    rec_i32 *recs;
    int      n;
} solid_rec_list_s;

extern u16           g_animated_tiles[65536];
extern const int     g_pxmask_tab[32 * 16];
extern const tri_i32 tilecolliders[GAME_NUM_TILECOLLIDERS];

void             game_init(game_s *g);
void             game_tick(game_s *g);
void             game_draw(game_s *g);
void             game_paused(game_s *g);
int              tick_now(game_s *g);
void             game_new_savefile(game_s *g, int slotID);
void             game_write_savefile(game_s *g);
void             game_load_savefile(game_s *g, savefile_s sf, int slotID);
void             game_on_trigger(game_s *g, int trigger);
bool32           tiles_solid(game_s *g, rec_i32 r);
bool32           tile_one_way(game_s *g, rec_i32 r);
bool32           game_traversable(game_s *g, rec_i32 r);
solid_rec_list_s game_solid_recs(game_s *g);
void             game_on_solid_appear(game_s *g);
int              rtile_pack(int tx, int ty);
void             rtile_unpack(int ID, int *tx, int *ty);
void             game_apply_hitboxes(game_s *g, hitbox_s *boxes, int n_boxes);
void             game_put_grass(game_s *g, int tx, int ty);

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

#endif