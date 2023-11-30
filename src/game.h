// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "cam.h"
#include "gamedef.h"
#include "map_loader.h"
#include "obj/hero.h"
#include "obj/obj.h"
#include "rope.h"
#include "textbox.h"
#include "title.h"
#include "transition.h"
#include "water.h"

#define NUM_TILES         POW2(256)
#define NUM_OBJ           256
#define INTERACTABLE_DIST 32
#define AREA_NAME_TICKS   300

enum {
    BACKGROUND_WIND_CIRCLE_R = 1900,
};

enum {
    BG_NUM_CLOUDS      = 64,
    BG_NUM_PARTICLES   = 512,
    BG_NUM_CLOUD_TYPES = 3,
    BG_WIND_PARTICLE_N = 8,
};

typedef struct {
    v2_i32 pos;
    int    type;
    int    x_q8;
    int    v_q8;
} grass_s;

typedef struct {
    int    n;
    v2_i32 p_q8; // q8
    v2_i32 v_q8;
    v2_i32 pos_q8[BG_WIND_PARTICLE_N];
    v2_i32 circc;
    i32    ticks;
    i32    circticks;
    i32    circcooldown;
} windparticle_s;

typedef struct {
    // values for Tiled's layer config
    f32 x;
    f32 y;
    f32 offx;
    f32 offy;
} parallax_img_s;

typedef struct {
    u16 layer[2];
} rtile_s;

typedef struct {
    u8 type;
    u8 collision;
} tile_s;

struct game_s {
    int           tick;
    mainmenu_s    mainmenu;
    int           state;
    map_world_s   map_world; // layout of all map files globally
    //
    int           savefile_slotID;
    transition_s  transition;
    cam_s         cam;
    tile_s        tiles[NUM_TILES];
    rtile_s       rtiles[NUM_TILES];
    int           tiles_x;
    int           tiles_y;
    int           pixel_x;
    int           pixel_y;
    int           obj_nfree;
    int           obj_nbusy;
    int           obj_ndelete;
    obj_s        *obj_tag[NUM_OBJ_TAGS];
    obj_s        *obj_free_stack[NUM_OBJ];
    obj_s        *obj_busy[NUM_OBJ];
    obj_s        *obj_to_delete[NUM_OBJ];
    obj_generic_s obj_raw[NUM_OBJ];

    hero_s         herodata;
    parallax_img_s parallax;
    rope_s         rope; // hero rope, singleton
    textbox_s      textbox;

    windparticle_s windparticles[BG_NUM_PARTICLES];
    int            n_windparticles;

    rope_s *ropes[16];
    int     n_ropes;

    char area_filename[LEN_AREA_FILENAME];
    char area_name[64];
    int  area_name_ticks;

    grass_s grass[256];
    int     n_grass;

    ocean_s ocean;

    marena_s arena;
    alignas(4) char mem[MKILOBYTE(1024)];
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
int              tick_now(game_s *g);
void             game_new_savefile(game_s *g, int slotID);
void             game_write_savefile(game_s *g);
void             game_load_savefile(game_s *g, savefile_s sf, int slotID);
void             game_trigger(game_s *g, int triggerID);
bool32           tiles_solid(game_s *g, rec_i32 r);
bool32           game_traversable(game_s *g, rec_i32 r);
solid_rec_list_s game_solid_recs(game_s *g);
int              rtile_pack(int tx, int ty);
void             rtile_unpack(int ID, int *tx, int *ty);
void             game_apply_hitboxes(game_s *g, hitbox_s *boxes, int n_boxes);

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