// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "cam.h"
#include "decoration.h"
#include "draw.h"
#include "game_def.h"
#include "obj.h"
#include "obj/hero.h"
#include "rope.h"
#include "savefile.h"
#include "textbox.h"
#include "transition.h"
#include "water.h"
#include "world.h"

#define TILEID_NULL U16_MAX

static inline u32 tileID_encode_ts(int tilesetID, int tx, int ty)
{
        u32 ID = ((u32)(ty + (tilesetID << 5)) << 5) | (u32)tx;
        return ID;
}

static inline u32 tileID_encode(int tx, int ty)
{
        u32 ID = ((u32)ty << 5) | (u32)tx;
        return ID;
}

static inline void tileID_decode(u32 ID, int *tx, int *ty)
{
        *tx = ID & 31; /* % 32 */
        *ty = ID >> 5; /* / 32 */
}

static inline u32 rtile_set(u32 rtileID, u32 tID, int layerID)
{
        static const u32 mk[2] = {0xFFFF0000U, 0xFFFFU};
        return ((rtileID & mk[layerID]) | (tID << (layerID * 16)));
}

static inline void rtile_decode(u32 rtileID, int *tile0, int *tile1)
{
        *tile0 = (rtileID >> 16);
        *tile1 = (rtileID & 0xFFFFU);
}

typedef struct {
        int ID;
        int frames;
        int cur;
        int ticks;
        u16 IDs[4];
} tile_animation_s;

typedef struct {
        texregion_s tr;
        v2_i32      p;
} decal_s;

typedef struct {
        int    ID;
        v2_i32 pos;
} savepoint_s;

struct game_s {
        i32    tick;
        int    state;
        int    savefile_slotID;
        u32    rng;
        cam_s  cam;
        hero_s hero;
        int    area_name_ticks; // how long the area name is already displayed
        char   area_name[64];   // the ingame area name to display
        char   area_filename[LEN_STR_AREA_FILENAME];

        cloudbg_s    clouds[BG_NUM_CLOUDS];
        int          nclouds;
        particlebg_s particlesbg[BG_NUM_PARTICLES];
        int          nparticles;
        int          clouddirection;

        // values for Tiled's layer config
        f32 parallax_x;
        f32 parallax_y;
        f32 parallax_offx;
        f32 parallax_offy;

        int        n_particles;
        particle_s particles[256];

        // objects
        objset_s    obj_scheduled_delete;        // objects scheduled for removal
        obj_s       objs[NUM_OBJS];              // raw object array
        obj_s      *objfreestack[NUM_OBJS];      // free object pointer stack
        obj_s      *obj_tag[NUM_OBJ_TAGS];       // tagged special objs
        int         n_objfree;                   // number of free objects on the stack
        objbucket_s objbuckets[NUM_OBJ_BUCKETS]; // sorted buckets of objects

        // room dimensions
        int tiles_x; // dimensions in tiles
        int tiles_y;
        int pixel_x; // dimensions in pixels
        int pixel_y;

        u8  tiles[NUM_TILES];  // collision tiles
        u32 rtiles[NUM_TILES]; // render tiles

        // available save points in this area
        savepoint_s savepoints[4];
        int         n_savepoints;

        transition_s transition;

        textbox_s         textbox;
        world_def_s      *curr_world;
        world_area_def_s *curr_world_area;

        memheap_s       heap;
        ALIGNAS(4) char heapmem[GAME_HEAPMEM];
};

extern game_s           g_gamestate;
extern u16              g_tileIDs[GAME_NUM_TILEIDS];
extern tile_animation_s g_tileanimations[GAME_NUM_TILEANIMATIONS];
extern const tri_i32    tilecolliders[GAME_NUM_TILECOLLIDERS];

void   game_init(game_s *g);
void   game_update(game_s *g);
void   game_draw(game_s *g);
void   game_close(game_s *g);
//
void  *game_heapalloc(game_s *g, size_t size);
void   game_heapfree(game_s *g, void *ptr);
//
void   game_trigger(game_s *g, int triggerID);
void   game_load_map(game_s *g, const char *filename);
void   game_obj_group_collisions(game_s *g);
void   game_cull_scheduled(game_s *g);
//
bool32 room_area_blocked(game_s *g, rec_i32 r);
bool32 room_overlaps_tileID(game_s *g, rec_i32 r, int tileID);
bool32 room_is_ladder(game_s *g, v2_i32 p);
void   room_tilebounds_pts(game_s *g, v2_i32 p1, v2_i32 p2, i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void   room_tilebounds_tri(game_s *g, tri_i32 t, i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void   room_tilebounds_rec(game_s *g, rec_i32 r, i32 *x1, i32 *y1, i32 *x2, i32 *y2);

particle_s *particle_spawn(game_s *g);

#endif