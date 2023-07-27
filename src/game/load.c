#include "game.h"

void game_load_map(game_s *g, const char *filename)
{
        NOT_IMPLEMENTED

        int w = 1;
        int h = 1;

        g->tiles_x = w;
        g->tiles_y = h;
        g->pixel_x = w << 4;
        g->pixel_y = h << 4;
}