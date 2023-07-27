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

#endif