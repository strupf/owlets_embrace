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
#include "render.h"
#include "rope.h"
#include "textbox.h"

struct cam_s {
        rec_i32 r;
};

struct rtile_s {
        u16  ID;
        uint tx;
        uint ty;
        u8   flags;
};

enum {
        TRANSITION_TYPE_SIMPLE,
};

enum {
        TRANSITION_TICKS = 10,
};

enum {
        TRANSITION_NONE,
        TRANSITION_FADE_OUT,
        TRANSITION_FADE_IN,
};

struct game_s {
        i32   tick;
        cam_s cam;

        hero_s         hero;
        ropecollider_s coll;
        objset_s       obj_active;           // active objects
        objset_s       obj_scheduled_delete; // objects scheduled for removal
        obj_s          objs[NUM_OBJS];
        obj_s         *objfreestack[NUM_OBJS];
        obj_s         *obj_tag[NUM_OBJ_TAGS];
        int            n_objfree;

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
void       obj_apply_movement(obj_s *o);
bool32     game_area_blocked(game_s *g, rec_i32 r);

#endif