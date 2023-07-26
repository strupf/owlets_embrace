#ifndef GAME_H
#define GAME_H

#include "os.h"

enum {
        NUM_TILES = 256 * 256,
};

typedef struct {
        i32 tick;

        u8  tiles[NUM_TILES];
        int tiles_x;
        int tiles_y;
} game_s;

void game_init(game_s *g);
void game_update(game_s *g);
void game_draw(game_s *g);
void game_close(game_s *g);

#endif