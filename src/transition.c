// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "transition.h"
#include "game.h"

void cb_transition_load(void *arg)
{
    transition_fade_arg_s *fa = (transition_fade_arg_s *)arg;
    transition_s          *t  = fa->t;
    game_s                *g  = fa->g;

    game_load_map(g, t->to_load);
    obj_s *hero  = hero_create(g);
    hero->pos.x  = t->hero_feet.x - hero->w / 2;
    hero->pos.y  = t->hero_feet.y - hero->h;
    hero->facing = t->hero_face;
    hero->vel_q8 = t->hero_v;
    cam_s *cam   = &g->cam;
    v2_i32 hpos  = obj_pos_center(hero);
    cam_set_pos_px(cam, hpos.x, hpos.y);
}

static void transition_start(transition_s *t, game_s *g, const char *file,
                             int type, v2_i32 hero_feet, v2_i32 hero_v, int facing)
{
    t->fade_arg.t = t;
    t->fade_arg.g = g;

    t->type      = type;
    t->hero_feet = hero_feet;
    t->hero_v    = hero_v;
    t->hero_face = facing;

    str_cpy(t->to_load, file);
    fade_start(&t->fade,
               20, // ticks fading out
               10, // ticks black
               20, // ticks fading in
               cb_transition_load, NULL, &t->fade_arg);
}

void transition_teleport(transition_s *t, game_s *g, const char *mapfile, v2_i32 hero_feet)
{
    transition_start(t, g, mapfile, 0, hero_feet, (v2_i32){0}, 1);
}

void transition_check_hero_slide(transition_s *t, game_s *g)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o) return;

    int touchedbounds = 0;
    if (o->pos.x < 1)
        touchedbounds = DIRECTION_W;
    if (o->pos.x + o->w + 1 > g->pixel_x)
        touchedbounds = DIRECTION_E;
    if (o->pos.y < 1)
        touchedbounds = DIRECTION_N;
    if (o->pos.y + o->h + 1 > g->pixel_y)
        touchedbounds = DIRECTION_S;

    if (touchedbounds == 0) return;

    rec_i32 aabb    = obj_aabb(o);
    rec_i32 trgaabb = aabb;
    v2_i32  vdir    = direction_v2(touchedbounds);
    aabb.x += vdir.x;
    aabb.y += vdir.y;

    if (!g->map_world.roomcur) {
        BAD_PATH
        return;
    }
    aabb.x += g->map_world.roomcur->x;
    aabb.y += g->map_world.roomcur->y;

    map_worldroom_s *nextroom = map_world_overlapped_room(&g->map_world, aabb);

    if (!nextroom) {
        sys_printf("no room\n");
        return;
    }

    rec_i32 nr = (rec_i32){nextroom->x, nextroom->y, nextroom->w, nextroom->h};
    trgaabb.x += g->map_world.roomcur->x - nr.x;
    trgaabb.y += g->map_world.roomcur->y - nr.y;

    switch (touchedbounds) {
    case DIRECTION_E:
        trgaabb.x = 8;
        break;
    case DIRECTION_W:
        trgaabb.x = nr.w - trgaabb.w - 8;
        break;
    case DIRECTION_N:
        trgaabb.y = nr.h - trgaabb.h - 8;
        break;
    case DIRECTION_S:
        trgaabb.y = 8;
        break;
    }

    v2_i32 feet = {trgaabb.x + trgaabb.w / 2, trgaabb.y + trgaabb.h};
    transition_start(t, g, nextroom->filename, 0, feet, o->vel_q8, o->facing);
}

void transition_update(transition_s *t)
{
    fade_update(&t->fade);
}

bool32 transition_finished(transition_s *t)
{
    return (fade_phase(&t->fade) == FADE_PHASE_NONE);
}

void transition_draw(transition_s *t)
{
    if (transition_finished(t)) return;

    int           fade_i = fade_interpolate(&t->fade, 0, 100);
    gfx_pattern_s pat    = gfx_pattern_interpolate(fade_i, 100);

    tex_s display = asset_tex(0);
    u32  *px      = (u32 *)display.px;
    for (int y = 0; y < display.h; y++) {
        u32 p = pat.p[y & 7];
        for (int x = 0; x < display.wword; x++) {
            *px++ &= ~p;
        }
    }
}
