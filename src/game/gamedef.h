// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "os/os.h"
#include "util/array.h"

#define ASSET_PATH_MAPS     "assets/map/"
#define ASSET_PATH_DIALOGUE "assets/"

enum {
        NUM_TILES             = 256 * 256,
        NUM_RENDERTILE_LAYERS = 2,
        NUM_OBJS              = 256,
        //
        TILECACHE_W           = 512,
        TILECACHE_H           = 256,
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

ARR_DEF_PRIMITIVE(obj, obj_s *)

struct objhandle_s {
        int    gen;
        obj_s *o;
};

enum obj_flag {
        OBJ_FLAG_NONE,
        OBJ_FLAG_ALIVE,
        OBJ_FLAG_DUMMY,
        OBJ_FLAG_ACTOR,
        OBJ_FLAG_SOLID,
        OBJ_FLAG_HERO,
        OBJ_FLAG_NEW_AREA_COLLIDER,
        OBJ_FLAG_PICKUP,
        OBJ_FLAG_HOOK,
        OBJ_FLAG_INTERACT,
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
        //
        NUM_OBJ_BUCKETS = 64,
};

enum {
        TILE_EMPTY      = 0,
        TILE_BLOCK      = 1,
        //
        TILE_SLOPE_45   = 2,
        TILE_SLOPE_45_1 = 2,
        TILE_SLOPE_45_2,
        TILE_SLOPE_45_3,
        TILE_SLOPE_45_4,
        //
        TILE_SLOPE_LO   = 6,
        TILE_SLOPE_LO_1 = 6,
        TILE_SLOPE_LO_2,
        TILE_SLOPE_LO_3,
        TILE_SLOPE_LO_4,
        TILE_SLOPE_LO_5,
        TILE_SLOPE_LO_6,
        TILE_SLOPE_LO_7,
        TILE_SLOPE_LO_8,
        //
        TILE_SLOPE_HI   = 14,
        TILE_SLOPE_HI_1 = 14,
        TILE_SLOPE_HI_2,
        TILE_SLOPE_HI_3,
        TILE_SLOPE_HI_4,
        TILE_SLOPE_HI_5,
        TILE_SLOPE_HI_6,
        TILE_SLOPE_HI_7,
        TILE_SLOPE_HI_8,
};

#endif