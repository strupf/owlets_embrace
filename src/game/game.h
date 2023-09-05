// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "os/os.h"
//
#include "backforeground.h"
#include "cam.h"
#include "draw.h"
#include "gamedef.h"
#include "maptransition.h"
#include "obj/hero.h"
#include "obj/obj.h"
#include "obj/objset.h"
#include "rope.h"
#include "savefile.h"
#include "textbox.h"
#include "tilegrid.h"
#include "water.h"

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
        u16 ID[2];
};

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

typedef struct {
        texregion_s tr;
        v2_i32      p;
} decal_s;

extern tile_animation_s g_tileanimations[NUM_TILEANIMATIONS];

typedef struct {
        char    filename[64];
        rec_i32 r;
} roomdesc_s;

typedef struct {
        roomdesc_s  rooms[64];
        roomdesc_s *curr;
        int         n_rooms;
} roomlayout_s;

struct game_s {
        i32 tick;
        int state;

        u32    rng;
        cam_s  cam;
        hero_s hero;

        i32  areaname_display_ticks;
        char areaname[64];

        bool32 itemselection_dirty;
        tex_s  itemselection_cache;

        objset_s    obj_scheduled_delete; // objects scheduled for removal
        obj_s       objs[NUM_OBJS];
        obj_s      *objfreestack[NUM_OBJS];
        obj_s      *obj_tag[NUM_OBJ_TAGS];
        int         n_objfree;
        objbucket_s objbuckets[NUM_OBJ_BUCKETS];

        int        n_particles;
        particle_s particles[NUM_PARTICLES];

        int     tiles_x;
        int     tiles_y;
        int     pixel_x;
        int     pixel_y;
        u8      tiles[NUM_TILES];
        rtile_s rtiles[NUM_TILES];

        backforeground_s backforeground;
        transition_s     transition;

        roomlayout_s roomlayout;
        textbox_s    textbox;

        watersurface_s  water;
        waterparticle_s wparticles[256];

        pathmover_s pathmover;

        memheap_s       heap;
        ALIGNAS(4) char heapmem[GAME_HEAPMEM];
};

extern game_s g_gamestate;

void       *game_heapalloc(size_t size);
void        game_heapfree(void *ptr);
void        game_init(game_s *g);
void        game_update(game_s *g);
void        game_draw(game_s *g);
void        game_close(game_s *g);
void        game_trigger(game_s *g, int triggerID);
void        game_load_map(game_s *g, const char *filename);
bool32      game_area_blocked(game_s *g, rec_i32 r);
bool32      game_is_ladder(game_s *g, v2_i32 p);
void        obj_interact_dialog(game_s *g, obj_s *o, void *arg);
obj_listc_s objbucket_list(game_s *g, int bucketID);
particle_s *particle_spawn(game_s *g);
void        solid_think(game_s *g, obj_s *o, void *arg);
// loads a Tiled .world file
void        roomlayout_load(roomlayout_s *rl, const char *filename);
roomdesc_s *roomlayout_get(roomlayout_s *rl, rec_i32 rec);

#endif