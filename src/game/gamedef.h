/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "os/os.h"

enum {
        NUM_TILES             = 256 * 256,
        NUM_RENDERTILE_LAYERS = 2,
        NUM_OBJS              = 256,
};

typedef struct game_s      game_s;
typedef struct rtile_s     rtile_s;
typedef struct obj_s       obj_s;
typedef struct objhandle_s objhandle_s;
typedef struct tilegrid_s  tilegrid_s;
typedef struct cam_s       cam_s;
typedef struct objset_s    objset_s;

#endif