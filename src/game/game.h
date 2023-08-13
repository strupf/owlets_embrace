// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "cam.h"
#include "collision.h"
#include "gamedef.h"
#include "hero.h"
#include "obj.h"
#include "objset.h"
#include "os/os.h"
#include "rope.h"
#include "textbox.h"

// Object bucket is an automatically filtered set of
// active objects. The filter works using object flags (bitset)
struct objbucket_s {
        objset_s      set;
        objflags_s    op_flag[2];
        objflag_op_e  op_func[2];
        objflags_s    cmp_flag;
        objflag_cmp_e cmp_func;
};

struct rtile_s {
        u16 ID;
        u16 ID2;
};

#define TILEID_NULL U16_MAX

static inline u32 tileID_encode_ts(int tilesetID, int tx, int ty)
{
        u32 ID = ((u32)(ty + tilesetID * 32) * 32) | ((u32)tx);
        return ID;
}

static inline u32 tileID_encode(int tx, int ty)
{
        u32 ID = ((u32)ty * 32) | ((u32)tx);
        return ID;
}

static inline void tileID_decode(u32 ID, int *tx, int *ty)
{
        *tx = ID % 32;
        *ty = ID / 32;
}

typedef struct {
        v2_i32 p_q8;
        v2_i32 v_q8;
        v2_i32 a_q8;
        i32    ticks;
} particle_s;

typedef struct {
        int ID;
        int frames;
        int cur;
        int ticks;
        u16 IDs[4];
} tile_animation_s;

extern tile_animation_s g_tileanimations[NUM_TILEANIMATIONS];

enum {
        TRANSITION_TICKS = 10,
};

enum transition_type {
        TRANSITION_TYPE_SIMPLE,
};

typedef enum {
        TRANSITION_NONE,
        TRANSITION_FADE_OUT,
        TRANSITION_FADE_IN,
} transition_phase_e;

typedef struct {
        int    cloudtype;
        v2_i32 pos;
        i32    velx;
} cloudbg_s;

typedef struct {
        v2_i32 pos;
        v2_i32 vel;
} particlebg_s;

typedef struct {
        cloudbg_s    clouds[NUM_BACKGROUND_CLOUDS];
        int          nclouds;
        particlebg_s particles[NUM_BACKGROUND_PARTICLES];
        int          nparticles;
} background_s;

typedef struct {
        char    filename[64];
        rec_i32 r;
} roomdesc_s;

typedef struct {
        roomdesc_s  rooms[64];
        roomdesc_s *curr;
        int         n_rooms;
} roomlayout_s;

typedef enum {
        DIRECTION_NONE,
        DIRECTION_W,
        DIRECTION_N,
        DIRECTION_E,
        DIRECTION_S,
} direction_e;

typedef struct {
        transition_phase_e phase;
        int                ticks;
        char               map[64]; // next map to load
        rec_i32            heroprev;
        cam_s              camprev;
        direction_e        enterfrom;
} transition_s;

struct game_s {
        i32    tick;
        u32    rng;
        cam_s  cam;
        hero_s hero;

        objset_s    obj_scheduled_delete; // objects scheduled for removal
        obj_s       objs[NUM_OBJS];
        obj_s      *objfreestack[NUM_OBJS];
        obj_s      *obj_tag[NUM_OBJ_TAGS];
        int         n_objfree;
        objbucket_s objbuckets[NUM_OBJ_BUCKETS];

        int        n_particles;
        particle_s particles[NUM_PARTICLES];
        textbox_s  textbox;

        int     tiles_x;
        int     tiles_y;
        int     pixel_x;
        int     pixel_y;
        u8      tiles[NUM_TILES];
        rtile_s rtiles[NUM_TILES];

        background_s background;
        transition_s transition;

        roomlayout_s roomlayout;
};

// loads a Tiled .world file
void        roomlayout_load(roomlayout_s *rl, const char *filename);
roomdesc_s *roomlayout_get(roomlayout_s *rl, rec_i32 rec);
void        game_init(game_s *g);
void        game_update(game_s *g);
void        game_draw(game_s *g);
void        game_close(game_s *g);
void        game_trigger(game_s *g, int triggerID);
//
tilegrid_s  game_tilegrid(game_s *g);
void        game_map_transition_start(game_s *g, const char *filename);
void        game_load_map(game_s *g, const char *filename);
bool32      game_area_blocked(game_s *g, rec_i32 r);
//
obj_listc_s objbucket_list(game_s *g, int bucketID);
void        game_tile_bounds_minmax(game_s *g, v2_i32 pmin, v2_i32 pmax,
                                    i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void        game_tile_bounds_tri(game_s *g, tri_i32 t, i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void        game_tile_bounds_rec(game_s *g, rec_i32 r, i32 *x1, i32 *y1, i32 *x2, i32 *y2);
bool32      solid_occupies(obj_s *solid, rec_i32 r);
particle_s *particle_spawn(game_s *g);
void        solid_think(game_s *g, obj_s *o);

#endif