// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"
#include "obj.h"
#include "rope.h"

enum hero_const {
        HERO_C_JUMP_INIT = -300,
        HERO_C_ACCX      = 30,
        HERO_C_JUMP_MAX  = 60,
        HERO_C_JUMP_MIN  = 0,
        HERO_C_JUMPTICKS = 20,
        HERO_C_EDGETICKS = 8,
        HERO_C_GRAVITY   = 32,
};

obj_s *hero_create(game_s *g, hero_s *h)
{
        obj_s *hero = obj_create(g);

        hero->flags      = objflags_create(OBJ_FLAG_ACTOR,
                                           OBJ_FLAG_HERO);
        hero->actorflags = ACTOR_FLAG_CLIMB_SLOPES |
                           ACTOR_FLAG_GLUE_GROUND;
        hero->pos.x        = 50;
        hero->pos.y        = 20;
        hero->w            = 16;
        hero->h            = 24;
        hero->gravity_q8.y = HERO_C_GRAVITY;
        hero->drag_q8.x    = 256;
        hero->drag_q8.y    = 256; // no drag
        *h                 = (const hero_s){0};
        h->obj             = objhandle_from_obj(hero);
        return hero;
}

static void hook_destroy(game_s *g, hero_s *h, obj_s *ohero, obj_s *ohook)
{
        obj_delete(g, ohook);
        h->hook.o = NULL;
}

static void hero_logic(game_s *g, obj_s *o, hero_s *h)
{
        if (os_inp_pressed(INP_LEFT)) h->inp |= HERO_INP_LEFT;
        if (os_inp_pressed(INP_RIGHT)) h->inp |= HERO_INP_RIGHT;
        if (os_inp_pressed(INP_DOWN)) h->inp |= HERO_INP_DOWN;
        if (os_inp_pressed(INP_UP)) h->inp |= HERO_INP_UP;
        if (os_inp_pressed(INP_A)) h->inp |= HERO_INP_JUMP;
        if (os_inp_pressed(INP_B)) h->inp |= HERO_INP_USE_ITEM;

        bool32 grounded = game_area_blocked(g, obj_rec_bottom(o));
        if (grounded) {
                h->edgeticks = HERO_C_EDGETICKS;
        } else if (h->edgeticks > 0) {
                h->edgeticks--;
        }

        if (h->inp & HERO_INP_JUMP) {
                if (!(h->inpp & HERO_INP_JUMP) && h->edgeticks > 0) {
                        h->edgeticks = 0;
                        h->jumpticks = HERO_C_JUMPTICKS;
                        o->vel_q8.y  = HERO_C_JUMP_INIT;
                } else if (h->jumpticks > 0) {
                        i32 jt = HERO_C_JUMPTICKS - h->jumpticks;
                        int j  = HERO_C_JUMP_MAX -
                                ease_out_q(HERO_C_JUMP_MIN,
                                           HERO_C_JUMP_MAX,
                                           HERO_C_JUMPTICKS,
                                           jt, 2);
                        o->vel_q8.y -= j;
                        h->jumpticks--;
                }
        } else {
                h->jumpticks = 0;
        }

        if ((h->inp & HERO_INP_USE_ITEM) &&
            !(h->inpp & HERO_INP_USE_ITEM)) {
                obj_s *hook;
                if (try_obj_from_handle(h->hook, &hook)) {
                        hook_destroy(g, h, o, hook);
                } else {
                        hook        = obj_create(g);
                        hook->flags = objflags_create(
                            OBJ_FLAG_ACTOR,
                            OBJ_FLAG_HOOK);
                        hook->w            = 8;
                        hook->h            = 8;
                        hook->pos          = o->pos;
                        hook->vel_q8.x     = 700;
                        hook->vel_q8.y     = -1000;
                        hook->drag_q8.x    = 256;
                        hook->drag_q8.y    = 256;
                        hook->gravity_q8.y = 30;
                        h->hook            = objhandle_from_obj(hook);
                        rope_init(&h->rope);
                        h->rope.head->p = (v2_i32){
                            o->pos.x + o->w / 2,
                            o->pos.y + o->h / 2};
                        h->rope.tail->p = (v2_i32){
                            o->pos.x + hook->w / 2,
                            o->pos.y + hook->h / 2};
                }
        }

        int xs = 0;
        if (h->inp & HERO_INP_LEFT) xs = -1;
        if (h->inp & HERO_INP_RIGHT) xs = +1;

        o->vel_q8.x += xs * HERO_C_ACCX;

        if (grounded && (SGN(o->vel_q8.x) == -xs || xs == 0)) {
                o->drag_q8.x = 180;
        } else {
                o->drag_q8.x = 255;
        }
}

void hero_update(game_s *g, obj_s *o, hero_s *h)
{
        hero_logic(g, o, &g->hero);

        obj_apply_movement(o);
        v2_i32 dt = v2_sub(o->pos_new, o->pos);

        v2_i32 herop = o->pos;
        obj_move_x(g, o, dt.x);
        obj_move_y(g, o, dt.y);

        obj_s *hook;
        if (!try_obj_from_handle(h->hook, &hook)) return;

        int crankchange = os_inp_crank_change();
        if (crankchange) {
                h->rope.len_max_q16 += crankchange * 100;
                h->rope.len_max_q16 = MAX(h->rope.len_max_q16, 0x40000);
                h->rope.len_max     = h->rope.len_max_q16 >> 16;
        }

        v2_i32 dthero = v2_sub(o->pos, herop);
        ropenode_move(&h->rope, &g->coll, h->rope.head, dthero);

        if (!hook->attached) {
                v2_i32 hookp = hook->pos;
                obj_apply_movement(hook);
                v2_i32 dthook = v2_sub(hook->pos_new, hook->pos);
                obj_move_x(g, hook, dthook.x);
                obj_move_y(g, hook, dthook.y);
                v2_i32 hookmove = v2_sub(hook->pos, hookp);
                ropenode_move(&h->rope, &g->coll, h->rope.tail, hookmove);
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1,
                                   hook->w + 2, hook->h + 2};
                if (game_area_blocked(g, hookrec)) {
                        hook->attached   = 1;
                        hook->gravity_q8 = (v2_i32){0};
                        hook->vel_q8     = (v2_i32){0};
                }
        }

        if (rope_blocked(&h->rope, &g->coll)) {
                hook_destroy(g, h, o, hook);
                return;
        }

        rope_update(&h->rope, &g->coll);

        if (hook->attached) {
                o->vel_q8 = rope_adjust_connected_vel(&h->rope,
                                                      h->rope.head,
                                                      o->subpos_q8,
                                                      o->vel_q8);
        } else {
                hook->vel_q8 = rope_adjust_connected_vel(&h->rope,
                                                         h->rope.tail,
                                                         hook->subpos_q8,
                                                         hook->vel_q8);
        }
}