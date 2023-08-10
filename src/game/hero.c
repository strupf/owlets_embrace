// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"
#include "obj.h"
#include "rope.h"

enum hero_const {
        HERO_C_JUMP_INIT = -350,
        HERO_C_ACCX_MAX  = 100,
        HERO_C_JUMP_MAX  = 70,
        HERO_C_JUMP_MIN  = 0,
        HERO_C_JUMPTICKS = 18,
        HERO_C_EDGETICKS = 6,
        HERO_C_GRAVITY   = 45,
};

static void   hero_jump_particles(game_s *g, obj_s *o);
static void   hero_land_particles(game_s *g, obj_s *o);
static void   hook_destroy(game_s *g, hero_s *h, obj_s *ohero, obj_s *ohook);
static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8);
void          hook_squeeze(game_s *g, obj_s *o);
void          hero_squeeze(game_s *g, obj_s *o);

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
                        hero_jump_particles(g, o);
                } else if (h->jumpticks > 0) {
                        int jfrom = pow2_i32(HERO_C_JUMPTICKS - h->jumpticks);
                        int jto   = pow2_i32(HERO_C_JUMPTICKS);
                        int j     = lerp_i32(HERO_C_JUMP_MAX,
                                             HERO_C_JUMP_MIN,
                                             jfrom,
                                             jto);
                        o->vel_q8.y -= j;
                        h->jumpticks--;
                }
        } else {
                h->jumpticks = 0;
        }

        // just pressed item button
        if ((h->inp & HERO_INP_USE_ITEM) && !(h->inpp & HERO_INP_USE_ITEM)) {
                obj_s *hook;

                if (try_obj_from_handle(h->hook, &hook)) {
                        hook_destroy(g, h, o, hook); // destroy hook if present
                } else {
                        // throw new hook
                        int dirx = os_inp_dpad_x();
                        int diry = os_inp_dpad_y();
                        if (dirx == 0 && diry == 0) diry = -1;

                        v2_i32 center  = obj_aabb_center(o);
                        v2_i32 vlaunch = {dirx, diry};

                        vlaunch = v2_shl(vlaunch, 10);
                        if (vlaunch.y < 0) {
                                vlaunch.y = vlaunch.y * 5 / 4;
                        }
                        if (vlaunch.y == 0) {
                                vlaunch.y = -500;
                                vlaunch.x = vlaunch.x * 5 / 4;
                        }
                        obj_s *hook = hook_create(g, &h->rope, center, vlaunch);
                        h->hook     = objhandle_from_obj(hook);
                        o->ropenode = h->rope.head;
                        o->rope     = &h->rope;
                }
        }

        int xs     = os_inp_dpad_x();
        int velsgn = SGN(o->vel_q8.x);

        bool32 ropestretched = objhandle_is_valid(h->hook) && rope_stretched(g, &h->rope);
        if (xs != 0) {
                int acc    = 0;
                int velabs = ABS(o->vel_q8.x);
                if (grounded) {
                        if (velsgn == -xs) { // reverse direction
                                acc          = 200;
                                o->drag_q8.x = 130;
                        } else {
                                o->drag_q8.x = 250;
                                int accfrom  = pow_i32(velabs, 2);
                                int accto    = pow_i32(1000, 2);
                                acc          = lerp_i32(HERO_C_ACCX_MAX,
                                                        0, // min acc
                                                        accfrom,
                                                        accto);
                        }
                } else {
                        o->drag_q8.x = 245;
                        if (ropestretched) {
                                v2_i32 dtrope = v2_sub(h->rope.head->p, h->rope.head->next->p);
                                acc           = (SGN(dtrope.x) == xs ? 10 : 25);
                        } else if (velsgn == -xs) {
                                acc = 50;
                        } else if (velabs < 500) {
                                acc = 25;
                        } else if (velabs < 1000) {
                                acc = 25;
                        }
                }
                o->vel_q8.x += xs * acc;
        } else {
                o->drag_q8.x = (grounded ? 130 : 245);
        }

        if (!grounded && ropestretched) {
                o->drag_q8.x = 254;
        }

        // PRINTF("%i\n", o->drag_q8.x);
        if (os_inp_just_pressed(INP_UP) && grounded &&
            o->vel_q8.x == 0 && o->vel_q8.y >= 0) {
                hero_interact_logic(g, h, o);
        }

        if (grounded && !h->wasgrounded && h->vel_q8_prev > 800) {
                hero_land_particles(g, o);
        }

        h->vel_q8_prev = o->vel_q8.y;
        h->wasgrounded = grounded;
}

static void hero_hook_update(game_s *g, obj_s *o, hero_s *h, obj_s *hook)
{
        rope_s *r           = &h->rope;
        int     crankchange = os_inp_crank_change();
        if (os_inp_dpad_y()) {
                crankchange = os_inp_dpad_y() * 1000;
        }
        if (crankchange) {
                r->len_max_q16 += crankchange * 100;
                r->len_max_q16 = CLAMP(r->len_max_q16, 0x40000, 400 << 16);
                r->len_max     = r->len_max_q16 >> 16;
        }

        if (!rope_intact(g, &h->rope)) {
                hook_destroy(g, h, o, hook);
                return;
        }
        float timei = os_time();
        if (!hook->attached) {
                v2_i32 hookp = hook->pos;
                obj_apply_movement(hook);
                v2_i32 dthook = v2_sub(hook->pos_new, hook->pos);
                obj_move_x(g, hook, dthook.x);
                obj_move_y(g, hook, dthook.y);
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
                if (game_area_blocked(g, hookrec)) {
                        hook->attached     = 1;
                        hook->gravity_q8   = (v2_i32){0};
                        hook->vel_q8       = (v2_i32){0};
                        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
                        for (int n = 0; n < solids.n; n++) {
                                obj_s *solid = solids.o[n];
                                if (solid_occupies(solid, hookrec)) {
                                        hook->linkedsolid = objhandle_from_obj(solid);
                                        break;
                                }
                        }
                }
        } else {
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
                if (!game_area_blocked(g, hookrec)) {
                        hook->attached      = 0;
                        hook->linkedsolid.o = NULL;
                }
                if (hook->linkedsolid.o) {
                        if (!solid_occupies(hook->linkedsolid.o, hookrec)) {
                                hook->linkedsolid.o = NULL;
                        }
                }
        }

        rope_update(g, &h->rope);
        if (hook->attached) {
                o->vel_q8 = rope_adjust_connected_vel(g, r, r->head,
                                                      o->subpos_q8, o->vel_q8);
        } else {
                hook->vel_q8 = rope_adjust_connected_vel(g, r, r->tail,
                                                         hook->subpos_q8, hook->vel_q8);
        }
        os_debug_time(TIMING_ROPE, os_time() - timei);
}

static void hook_destroy(game_s *g, hero_s *h, obj_s *ohero, obj_s *ohook)
{
        obj_delete(g, ohook);
        h->hook.o       = NULL;
        ohero->ropenode = NULL;
        ohero->rope     = NULL;
        ohook->rope     = NULL;
        ohook->ropenode = NULL;
}

void hook_squeeze(game_s *g, obj_s *o)
{
        hero_s *hero = (hero_s *)o->onsqueezearg;
        obj_s  *ohero;
        if (try_obj_from_handle(hero->obj, &ohero)) {
                hook_destroy(g, hero, ohero, o);
        } else {
                ASSERT(0);
        }
}

void hero_squeeze(game_s *g, obj_s *o)
{
        game_map_transition_start(g, "template.tmj");
}

static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8)
{
        obj_s     *o     = obj_create(g);
        objflags_s flags = objflags_create(
            OBJ_FLAG_ACTOR,
            OBJ_FLAG_HOOK);
        obj_set_flags(g, o, flags);
        o->w            = 8;
        o->h            = 8;
        o->pos.x        = p.x - o->w / 2;
        o->pos.y        = p.y - o->h / 2;
        o->vel_q8       = v_q8;
        o->drag_q8.x    = 256;
        o->drag_q8.y    = 256;
        o->gravity_q8.y = 34;
        o->onsqueeze    = hook_squeeze;
        o->onsqueezearg = &g->hero;
        rope_init(rope);
        rope->head->p = p;
        rope->tail->p = p;
        o->ropenode   = rope->tail;
        o->rope       = rope;
        return o;
}

obj_s *hero_create(game_s *g, hero_s *h)
{
        obj_s     *hero  = obj_create(g);
        objflags_s flags = objflags_create(OBJ_FLAG_ACTOR,
                                           OBJ_FLAG_HERO);
        obj_set_flags(g, hero, flags);
        hero->actorflags = ACTOR_FLAG_CLIMB_SLOPES |
                           ACTOR_FLAG_GLUE_GROUND;
        hero->pos.x        = 200;
        hero->pos.y        = 20;
        hero->w            = 16;
        hero->h            = 24;
        hero->gravity_q8.y = HERO_C_GRAVITY;
        hero->drag_q8.x    = 256;
        hero->drag_q8.y    = 256; // no drag
        hero->onsqueeze    = hero_squeeze;
        *h                 = (const hero_s){0};
        h->obj             = objhandle_from_obj(hero);
        return hero;
}

static void hero_jump_particles(game_s *g, obj_s *o)
{
        for (int i = 0; i < 6; i++) {
                particle_s *particle = particle_spawn(g);
                particle->ticks      = rng_max_u16(&g->rng, 10) + 4;
                particle->p_q8       = (v2_i32){o->pos.x + o->w / 2,
                                                o->pos.y + o->h - 4};
                particle->p_q8       = v2_shl(particle->p_q8, 8);
                particle->p_q8.x += rng_i16(&g->rng) / 50;
                particle->p_q8.y += rng_i16(&g->rng) / 50;
                particle->v_q8 = (v2_i32){rng_i16(&g->rng) / 120,
                                          rng_i16(&g->rng) / 220};
        }
}

static void hero_land_particles(game_s *g, obj_s *o)
{
        for (int i = 0; i < 16; i++) {
                particle_s *particle = particle_spawn(g);

                particle->ticks = rng_max_u16(&g->rng, 8) + 5;
                particle->p_q8  = (v2_i32){o->pos.x + o->w / 2,
                                           o->pos.y + o->h - 3};
                particle->a_q8  = (v2_i32){0, 50};
                particle->p_q8  = v2_shl(particle->p_q8, 8);
                particle->p_q8.x += rng_i16(&g->rng) / 50;
                particle->p_q8.y += rng_i16(&g->rng) / 100;
                particle->v_q8 = (v2_i32){rng_i16(&g->rng) / 80,
                                          rng_i16(&g->rng) / 80 - 200};
        }
}

void hero_check_level_transition(game_s *g, obj_s *hero)
{
        rec_i32     haabb    = obj_aabb(hero);
        obj_listc_s triggers = objbucket_list(g, OBJ_BUCKET_NEW_AREA_COLLIDER);
        for (int n = 0; n < triggers.n; n++) {
                obj_s *coll = triggers.o[n];
                if (overlap_rec_excl(haabb, obj_aabb(coll))) {
                        game_map_transition_start(g, coll->filename);
                        break;
                }
        }
}

void hero_pickup_logic(game_s *g, hero_s *h, obj_s *o)
{
        rec_i32     haabb   = obj_aabb(o);
        obj_listc_s pickups = objbucket_list(g, OBJ_BUCKET_PICKUP);
        for (int n = 0; n < pickups.n; n++) {
                obj_s *p = pickups.o[n];
                if (!overlap_rec_excl(haabb, obj_aabb(p))) continue;

                if (p->pickup.action) {
                        p->pickup.action(g, p->pickup.actionarg);
                }
                h->pickups += p->pickup.x;
                obj_delete(g, p);
        }
}

void hero_interact_logic(game_s *g, hero_s *h, obj_s *o)
{
        const obj_listc_s list = objbucket_list(g, OBJ_BUCKET_INTERACT);
        rec_i32           aabb = obj_aabb(o);
        for (int n = 0; n < list.n; n++) {
                obj_s *oi = list.o[n];
                if (overlap_rec_excl(aabb, obj_aabb(oi)) && oi->oninteract) {
                        oi->oninteract(g, oi);
                        break;
                }
        }
}

void hero_update(game_s *g, obj_s *o, hero_s *h)
{
        hero_logic(g, o, &g->hero);
        float timem = os_time();
        obj_apply_movement(o);
        v2_i32 dt = v2_sub(o->pos_new, o->pos);

        v2_i32 herop = o->pos;
        obj_move_x(g, o, dt.x);
        obj_move_y(g, o, dt.y);
        float timeh = os_time();
        os_debug_time(TIMING_HERO_MOVE, timeh - timem);

        obj_s *hook;
        if (try_obj_from_handle(h->hook, &hook)) {
                hero_hook_update(g, o, h, hook);
        }
        os_debug_time(TIMING_HERO_HOOK, os_time() - timeh);
}