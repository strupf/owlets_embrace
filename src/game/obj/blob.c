// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void blob_think(game_s *g, obj_s *o);
static void blob_think_2(game_s *g, obj_s *o);

obj_s *blob_create(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_apply_flags(g, o, OBJ_FLAG_HURTABLE | OBJ_FLAG_ACTOR | OBJ_FLAG_MOVABLE | OBJ_FLAG_THINK_1 | OBJ_FLAG_THINK_2 | OBJ_FLAG_HURTS_PLAYER | OBJ_FLAG_ENEMY);
        o->pos.x        = 200;
        o->pos.y        = 100;
        o->w            = 20;
        o->h            = 20;
        o->think_1      = blob_think;
        o->think_2      = blob_think_2;
        o->gravity_q8.y = 30;
        o->drag_q8.x    = 256;
        o->drag_q8.y    = 256; // no drag
        o->ID           = 13;
        return o;
}

static void blob_think(game_s *g, obj_s *o)
{
        static int groundticks = 0;
        bool32     grounded    = room_area_blocked(g, obj_rec_bottom(o));

        if (grounded) {
                o->vel_q8.x = 0;
                groundticks++;
                if (groundticks == 60) {
                        groundticks = 0;
                        o->vel_q8.y = -700;
                        o->vel_q8.x = rng_range(-400, +400);
                }
        }
}

static void blob_think_2(game_s *g, obj_s *o)
{
        for (int n = 0; n < o->n_colliders; n++) {
                obj_s *c = o->colliders[n];
                if (c->ID == 2) {
                        obj_delete(g, o);

                        for (int i = 0; i < 60; i++) {
                                /*
                                particle_s *particle = particle_spawn(g);
                                particle->ticks      = rng_range(15, 25);
                                particle->p_q8       = (v2_i32){o->pos.x + o->w / 2,
                                                                o->pos.y + o->h - 4};
                                particle->p_q8       = v2_shl(particle->p_q8, 8);

                                particle->p_q8.x += rng_range(-800, 800);
                                particle->p_q8.y += rng_range(-800, 800);

                                particle->v_q8.x = rng_range(-300, 300);
                                particle->v_q8.y = rng_range(-300, 300);
                                particle->a_q8.y = 30;
                                */
                        }

                        return;
                }
        }
}