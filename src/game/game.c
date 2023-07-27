#include "game.h"

void game_init(game_s *g)
{

        tex_put(TEXID_FONT_DEFAULT, tex_load("assets/font_mono_8.json"));
        fnt_s font1      = {0};
        font1.gridw      = 16;
        font1.gridh      = 32;
        font1.lineheight = 28;
        font1.tex        = tex_get(TEXID_FONT_DEFAULT);
        for (int n = 0; n < 256; n++) {
                font1.glyph_widths[n] = 14;
        }
        fnt_put(FNTID_DEFAULT, font1);

        for (int n = 1; n < NUM_OBJS; n++) {
                obj_s *o               = &g->objs[n];
                o->index               = n;
                o->gen                 = 1;
                g->objfreestack[n - 1] = o;
        }
        g->n_objfree = NUM_OBJS - 1;
}

void game_update(game_s *g)
{
}

void game_draw(game_s *g)
{
        gfx_rec_fill((rec_i32){50, 70, 30, 40}, 1);
        fnt_s font = fnt_get(FNTID_DEFAULT);
        gfx_text_ascii(&font, "Hello World", 10, 10);
}

void game_close(game_s *g)
{
}

tilegrid_s game_tilegrid(game_s *g)
{
        tilegrid_s tg = {g->tiles,
                         g->tiles_x,
                         g->tiles_y,
                         g->pixel_x,
                         g->pixel_y};
        return tg;
}