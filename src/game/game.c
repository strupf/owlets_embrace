/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "game.h"

void game_init(game_s *g)
{
        tex_put(TEXID_FONT_DEFAULT, tex_load("assets/font_mono_8.json"));
        tex_put(TEXID_TILESET, tex_load("assets/testtiles.json"));
        fnt_s font1      = {0};
        font1.gridw      = 16;
        font1.gridh      = 32;
        font1.lineheight = 28;
        font1.tex        = tex_get(TEXID_FONT_DEFAULT);
        for (int n = 0; n < 256; n++) {
                font1.glyph_widths[n] = 14;
        }
        fnt_put(FNTID_DEFAULT, font1);

        for (int n = 1; n < NUM_OBJS; n++) { // obj at index 0 is "dead"
                obj_s *o               = &g->objs[n];
                o->index               = n;
                o->gen                 = 1;
                g->objfreestack[n - 1] = o;
        }
        g->n_objfree = NUM_OBJS - 1;

        g->cam.r.w = 400;
        g->cam.r.h = 240;

        obj_s *player        = obj_create(g);
        player->pos.x        = 10;
        player->pos.y        = 20;
        player->w            = 16;
        player->h            = 32;
        player->gravity_q8.y = 20;
        player->drag_q8.x    = 200;
        player->drag_q8.y    = 256; // no drag
        g->hero.obj          = objhandle_from_obj(player);

        game_load_map(g, "assets/samplemap.tmj");
}

void game_update(game_s *g)
{
        obj_s *o;
        g->hero.inpp = g->hero.inp;
        g->hero.inp  = 0;
        if (try_obj_from_handle(g->hero.obj, &o)) {
                if (os_inp_pressed(INP_LEFT)) g->hero.inp |= HERO_INP_LEFT;
                if (os_inp_pressed(INP_RIGHT)) g->hero.inp |= HERO_INP_RIGHT;
                if (os_inp_pressed(INP_DOWN)) g->hero.inp |= HERO_INP_DOWN;
                if (os_inp_pressed(INP_UP)) g->hero.inp |= HERO_INP_UP;
                if (os_inp_pressed(INP_A)) g->hero.inp |= HERO_INP_JUMP;

                hero_update(g, o, &g->hero);
                obj_apply_movement(o);
                v2_i32 dt = v2_sub(o->pos_new, o->pos);
                obj_move_x(g, o, dt.x);
                obj_move_y(g, o, dt.y);
        }
}

void game_draw(game_s *g)
{
        render_draw(g);
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

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o)
{
        o->vel_q8    = v2_add(o->vel_q8, o->gravity_q8);
        o->vel_q8.x  = q_mulr(o->vel_q8.x, o->drag_q8.x, 8);
        o->vel_q8.y  = q_mulr(o->vel_q8.y, o->drag_q8.y, 8);
        o->subpos_q8 = v2_add(o->subpos_q8, o->vel_q8);
        o->pos_new   = v2_add(o->pos, v2_shr(o->subpos_q8, 8));
        o->subpos_q8.x &= 255;
        o->subpos_q8.y &= 255;
}

bool32 game_area_blocked(game_s *g, rec_i32 r)
{
        return tiles_area(game_tilegrid(g), r);
}