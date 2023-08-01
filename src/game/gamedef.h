// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "os/os.h"
#include "util/array.h"

enum {
        NUM_TILES             = 256 * 256,
        NUM_RENDERTILE_LAYERS = 2,
        NUM_OBJS              = 256,
        SOLIDMEM_SIZE         = 0x10000,
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

ARR_DEF_PRIMITIVE(obj, obj_s *)

struct objhandle_s {
        int    gen;
        obj_s *o;
};

enum obj_flag {
        OBJ_FLAG_NONE,
        OBJ_FLAG_DUMMY,
        OBJ_FLAG_ACTOR,
        OBJ_FLAG_SOLID,
        OBJ_FLAG_HERO,
        OBJ_FLAG_NEW_AREA_COLLIDER,
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
        OBJ_BUCKET_ACTOR,
        OBJ_BUCKET_SOLID,
        //
        NUM_OBJ_BUCKETS = 64
};

#endif