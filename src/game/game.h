/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef GAME_H
#define GAME_H

#include "collision.h"
#include "gamedef.h"
#include "obj.h"
#include "objset.h"
#include "os/os.h"
#include "render.h"

struct cam_s {
        rec_i32 r;
};

struct rtile_s {
        u16 ID;
        u8  flags;
};

struct game_s {
        i32   tick;
        cam_s cam;

        objhandle_s player;

        objset_s obj_active;           // active objects
        objset_s obj_scheduled_delete; // objects scheduled for removal
        obj_s    objs[NUM_OBJS];
        obj_s   *objfreestack[NUM_OBJS];
        int      n_objfree;

        int                tiles_x;
        int                tiles_y;
        int                pixel_x;
        int                pixel_y;
        ALIGNAS(4) u8      tiles[NUM_TILES];
        ALIGNAS(4) rtile_s rtiles[NUM_TILES][NUM_RENDERTILE_LAYERS];

        ALIGNAS(4) char solidmem[SOLIDMEM_SIZE];
};

void game_init(game_s *g);
void game_update(game_s *g);
void game_draw(game_s *g);
void game_close(game_s *g);

tilegrid_s game_tilegrid(game_s *g);
void       game_load_map(game_s *g, const char *filename);

#endif