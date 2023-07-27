#ifndef GAME_H
#define GAME_H

#include "collision.h"
#include "gamedef.h"
#include "obj.h"
#include "os/os.h"

struct rtile_s {
        u16 ID;
        u8  flags;
};

struct game_s {
        i32 tick;

        obj_s  objs[NUM_OBJS];
        obj_s *objfreestack[NUM_OBJS];
        int    n_objfree;

        int                tiles_x;
        int                tiles_y;
        int                pixel_x;
        int                pixel_y;
        ALIGNAS(4) u8      tiles[NUM_TILES];
        ALIGNAS(4) rtile_s rtiles[NUM_TILES][NUM_RENDERTILE_LAYERS];
};

void game_init(game_s *g);
void game_update(game_s *g);
void game_draw(game_s *g);
void game_close(game_s *g);

tilegrid_s game_tilegrid(game_s *g);
void       game_load_map(game_s *g, const char *filename);

#endif