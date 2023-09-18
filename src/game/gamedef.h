// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "os/os.h"
#include "util/array.h"

enum {
        GAMESTATE_GAMEPLAY,
        GAMESTATE_TITLE,
};

#define ASSET_PATH_MAPS     "assets/map/"
#define ASSET_PATH_DIALOGUE "assets/"

enum {
        GAME_HEAPMEM            = 0x40000,
        NUM_TILES               = 1024 * 1024,
        NUM_OBJS                = 256,
        NUM_AUTOTILE_MAIN       = 16,
        NUM_AUTOTILE_BG         = 48,
        NUM_AUTOTILE_TYPES      = NUM_AUTOTILE_BG + NUM_AUTOTILE_MAIN,
        NUM_TILEANIMATIONS      = 16,
        NUM_PARTICLES           = 256,
        NUM_CAMATTRACTORS       = 8,
        INTERACTABLE_DISTANCESQ = 175,
        AREA_NAME_DISPLAY_TICKS = 100,
        AREA_NAME_FADE_TICKS    = (AREA_NAME_DISPLAY_TICKS * 1) / 4,
        TILE_LAYER_MAIN         = 1,
        TILE_LAYER_BG           = 0,
        LENGTH_PLAYERNAME       = 16,
        LENGTH_AREAFILENAME     = 64,
};

typedef struct game_s      game_s;
typedef struct rtile_s     rtile_s;
typedef struct obj_s       obj_s;
typedef struct objhandle_s objhandle_s;
typedef struct objflags_s  objflags_s;
typedef struct objset_s    objset_s;
typedef struct objbucket_s objbucket_s;
typedef struct tilegrid_s  tilegrid_s;
typedef struct cam_s       cam_s;
typedef struct hero_s      hero_s;
typedef struct textbox_s   textbox_s;
typedef struct obj_listc_s obj_listc_s;

#define foreach_tile_in_bounds(X1, Y1, X2, Y2, XIT, YIT) \
        for (int YIT = Y1; YIT <= Y2; YIT++)             \
                for (int XIT = X1; XIT <= X2; XIT++)
extern u16 g_tileIDs[0x10000];

struct objhandle_s {
        int    gen;
        obj_s *o;
};

enum obj_flag {
        OBJ_FLAG_NONE,
        OBJ_FLAG_ALIVE,
        OBJ_FLAG_ACTOR,
        OBJ_FLAG_SOLID,
        OBJ_FLAG_HERO,
        OBJ_FLAG_NEW_AREA_COLLIDER,
        OBJ_FLAG_PICKUP,
        OBJ_FLAG_HOOK,
        OBJ_FLAG_INTERACT,
        OBJ_FLAG_MOVABLE,
        OBJ_FLAG_THINK_1,
        OBJ_FLAG_THINK_2,
        OBJ_FLAG_HURTABLE,
        OBJ_FLAG_ENEMY,
        OBJ_FLAG_KILL_OFFSCREEN,
        OBJ_FLAG_HURTS_PLAYER,
        OBJ_FLAG_CAM_ATTRACTOR,
        OBJ_FLAG_SPRITE_ANIM,
        OBJ_FLAG_RENDERABLE,
        OBJ_FLAG_ANIMATE,
        OBJ_FLAG_HURTS_ENEMIES,
        //
        NUM_OBJ_FLAGS
};

enum obj_tag {
        OBJ_TAG_DUMMY,
        OBJ_TAG_HERO,
        //
        NUM_OBJ_TAGS
};

enum obj_bucket {
        OBJ_BUCKET_ALIVE,
        OBJ_BUCKET_ACTOR,
        OBJ_BUCKET_SOLID,
        OBJ_BUCKET_NEW_AREA_COLLIDER,
        OBJ_BUCKET_PICKUP,
        OBJ_BUCKET_INTERACT,
        OBJ_BUCKET_MOVABLE,
        OBJ_BUCKET_THINK_1,
        OBJ_BUCKET_THINK_2,
        OBJ_BUCKET_HURTABLE,
        OBJ_BUCKET_KILL_OFFSCREEN,
        OBJ_BUCKET_HURTS_PLAYER,
        OBJ_BUCKET_CAM_ATTRACTOR,
        OBJ_BUCKET_SPRITE_ANIM,
        OBJ_BUCKET_RENDERABLE,
        OBJ_BUCKET_ANIMATE,
        OBJ_BUCKET_ENEMY,
        OBJ_BUCKET_HURTS_ENEMIES,
        //
        NUM_OBJ_BUCKETS
};

typedef enum {
        DIRECTION_NONE,
        DIRECTION_W,
        DIRECTION_N,
        DIRECTION_E,
        DIRECTION_S,
} direction_e;

enum {
        TILE_EMPTY      = 0,
        TILE_BLOCK      = 1,
        //
        TILE_SLOPE_45   = 2,
        TILE_SLOPE_45_1 = TILE_SLOPE_45,
        TILE_SLOPE_45_2,
        TILE_SLOPE_45_3,
        TILE_SLOPE_45_4,
        //
        TILE_SLOPE_LO   = 6,
        TILE_SLOPE_LO_1 = TILE_SLOPE_LO,
        TILE_SLOPE_LO_2,
        TILE_SLOPE_LO_3,
        TILE_SLOPE_LO_4,
        TILE_SLOPE_LO_5,
        TILE_SLOPE_LO_6,
        TILE_SLOPE_LO_7,
        TILE_SLOPE_LO_8,
        //
        TILE_SLOPE_HI   = 14,
        TILE_SLOPE_HI_1 = TILE_SLOPE_HI,
        TILE_SLOPE_HI_2,
        TILE_SLOPE_HI_3,
        TILE_SLOPE_HI_4,
        TILE_SLOPE_HI_5,
        TILE_SLOPE_HI_6,
        TILE_SLOPE_HI_7,
        TILE_SLOPE_HI_8,
        //
        NUM_TILE_BLOCKS,
        TILE_LADDER,
};

#endif