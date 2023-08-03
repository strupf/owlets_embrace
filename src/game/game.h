// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

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
        objset_s   set;
        objflags_s op_flag[2];
        int        op_func[2];
        objflags_s cmp_flag;
        int        cmp_func;
};

struct cam_s {
        int    w;
        int    h;
        int    wh;
        int    hh;
        v2_i32 target;
        v2_i32 pos;
};

struct rtile_s {
        u16  ID;
        uint tx;
        uint ty;
        u8   flags;
        int  m;
};

enum {
        TRANSITION_TICKS = 10,
};

enum transition_type {
        TRANSITION_TYPE_SIMPLE,
};

enum transition_phase {
        TRANSITION_NONE,
        TRANSITION_FADE_OUT,
        TRANSITION_FADE_IN,
};

struct game_s {
        i32   tick;
        cam_s cam;

        hero_s hero;

        objset_s obj_active;           // active objects
        objset_s obj_scheduled_delete; // objects scheduled for removal
        obj_s    objs[NUM_OBJS];
        obj_s   *objfreestack[NUM_OBJS];
        obj_s   *obj_tag[NUM_OBJ_TAGS];
        int      n_objfree;

        objbucket_s objbuckets[NUM_OBJ_BUCKETS];

        textbox_s textbox;

        int                tiles_x;
        int                tiles_y;
        int                pixel_x;
        int                pixel_y;
        ALIGNAS(4) u8      tiles[NUM_TILES];
        ALIGNAS(4) rtile_s rtiles[NUM_TILES][NUM_RENDERTILE_LAYERS];

        ALIGNAS(4) char solidmem[SOLIDMEM_SIZE];

        int  transitionphase;
        int  transitionticks;
        char transitionmap[64]; // next map to load
};

void       game_init(game_s *g);
void       game_update(game_s *g);
void       game_draw(game_s *g);
void       game_close(game_s *g);
//
tilegrid_s game_tilegrid(game_s *g);
void       game_map_transition_start(game_s *g);
void       game_load_map(game_s *g, const char *filename);
bool32     game_area_blocked(game_s *g, rec_i32 r);

obj_listc_s objbucket_list(game_s *g, int bucketID);

#endif