// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game/game.h"
#include "game/obj.h"
#include "game/rope.h"

static void   hook_destroy(game_s *g, hero_s *h, obj_s *ohero, obj_s *ohook);
static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8, i32 len_max);
static void   hook_squeeze(game_s *g, obj_s *o);
static void   hero_check_hurtables(game_s *g, obj_s *ohero);
static void   hero_jump_particles(game_s *g, obj_s *o);
static void   hero_land_particles(game_s *g, obj_s *o);
static void   hero_squeeze(game_s *g, obj_s *o);
static void   hero_animate(game_s *g, obj_s *o);

static void hero_animate(game_s *g, obj_s *o)
{
        hero_s *h = &g->hero;
        /*
        o->animation += (int)((float)ABS(o->vel_q8.x) * 0.1f);

        bool32 grounded   = room_area_blocked(g, obj_rec_bottom(o));
        o->animframe_prev = o->animframe;
        if (o->vel_prev_q8.x == 0 && o->vel_q8.x != 0) {
                o->animation = 300;
        }

        if (h->ppos.x == o->pos.x && grounded) {
                o->animation += 10;
                o->animframe = 0;
        } else {
                o->animframe = (o->animation / 500) % 4;
                if (grounded && o->animframe % 2 == 1 && o->animframe_prev != o->animframe) {
                        // snd_play_ext(snd_get(SNDID_STEP), 0.5f, rngf_range(0.8f, 1.f));
                }
        }
        */
}

static void hero_use_whip(game_s *g, obj_s *o, hero_s *h)
{
        h->whip_ticks = HERO_C_WHIP_TICKS;
}

static void hero_use_hook(game_s *g, obj_s *o, hero_s *h)
{
        // throw new hook
        int dirx = os_inp_dpad_x();
        int diry = os_inp_dpad_y();
        if (dirx == 0 && diry == 0) diry = -1;

        v2_i32 center  = obj_aabb_center(o);
        v2_i32 vlaunch = {dirx, diry};

        vlaunch = v2_shl(vlaunch, 11);
        if (vlaunch.y < 0) {
                vlaunch.y = vlaunch.y * 5 / 4;
        }
        if (vlaunch.y == 0) {
                vlaunch.y = -500;
                vlaunch.x = vlaunch.x * 5 / 4;
        }
        obj_s *hook = hook_create(g, &h->rope, center, vlaunch, h->rope_len_q16);
        h->hook     = objhandle_from_obj(hook);
        o->ropenode = h->rope.head;
        o->rope     = &h->rope;
}

static void hero_unset_ladder(game_s *g, obj_s *o, hero_s *h)
{
        obj_set_flags(g, o, OBJ_FLAG_MOVABLE);
        h->onladder = 0;
}

static void hero_logic(game_s *g, obj_s *o, hero_s *h)
{

        bool32 grounded = room_area_blocked(g, obj_rec_bottom(o));
        if (grounded) {
                hero_unset_ladder(g, o, h);
                h->edgeticks = HERO_C_EDGETICKS;
                h->state     = HERO_STATE_GROUND;
        } else if (h->edgeticks > 0) {
                h->edgeticks--;
        }

        if (h->onladder) {
                v2_i32 lp = obj_aabb_center(o);
                h->state  = HERO_STATE_LADDER;
                if (os_inp_just_pressed(INP_A) || !room_is_ladder(g, lp)) {
                        hero_unset_ladder(g, o, h);
                }
                if (os_inp_just_pressed(INP_A)) {
                        h->edgeticks = 1;
                        h->state     = HERO_STATE_AIR;
                        o->vel_q8.x  = os_inp_dpad_x() * 300;
                }

                if (h->onladder) {
                        if (os_inp_pressed(INP_UP)) {
                                o->tomove.y = -2;
                        }
                        if (os_inp_pressed(INP_DOWN)) {
                                o->tomove.y = +2;
                        }
                        return;
                }
        }

        int xs = os_inp_dpad_x();
        if (!hero_using_whip(h) && xs != 0 && xs != o->facing) {
                o->facing = xs;
        }

        if (!h->onladder && os_inp_just_pressed(INP_UP)) {
                v2_i32  lp      = obj_aabb_center(o);
                rec_i32 ladderr = obj_aabb(o);
                ladderr.x       = ((lp.x >> 4) << 4) + 8 - ladderr.w / 2;
                ladderr.y       = ((lp.y >> 4) << 4) + 16 - ladderr.h - 1;
                if (room_is_ladder(g, lp) && !room_area_blocked(g, ladderr)) {
                        h->onladder      = 1;
                        o->pos.x         = ladderr.x;
                        o->pos.y         = ladderr.y;
                        o->subpos_q8.x   = 0;
                        o->subpos_q8.y   = 0;
                        o->vel_prev_q8.y = 0;
                        o->vel_q8.y      = 0;
                        o->vel_prev_q8.x = 0;
                        o->vel_q8.x      = 0;
                        o->tomove.x      = 0;
                        o->tomove.y      = 0;
                        obj_unset_flags(g, o, OBJ_FLAG_MOVABLE);
                        return;
                }
        }

        if (o->vel_prev_q8.y > 700 && grounded) {
                snd_play(snd_get(SNDID_HERO_LAND));
        }

        // jumping
        if (os_inp_just_pressed(INP_A) && h->edgeticks > 0) {
                h->edgeticks = 0;
                h->jumpticks = HERO_C_JUMPTICKS;
                o->vel_q8.y  = -HERO_C_JUMP_INIT;
                snd_play_ext(snd_get(SNDID_JUMP), 0.15f, 1.f);
                hero_jump_particles(g, o);
        } else if (os_inp_pressed(INP_A) && h->jumpticks > 0) {
                int jfrom = pow2_i32(HERO_C_JUMPTICKS - h->jumpticks--);
                int jto   = pow2_i32(HERO_C_JUMPTICKS);
                o->vel_q8.y -= lerp_i32(HERO_C_JUMP_MAX,
                                        HERO_C_JUMP_MIN,
                                        jfrom,
                                        jto);
        } else if (!os_inp_pressed(INP_A)) {
                h->jumpticks = 0;
        }

        int crankchange_q16 = os_inp_crank_change();
        if (crankchange_q16) {
                h->rope_len_q16 += crankchange_q16 * HERO_ROPE_REEL_RATE;
                h->rope_len_q16 = clamp_i(h->rope_len_q16, HERO_ROPE_MIN, HERO_ROPE_MAX);
                if (hero_using_hook(h)) {
                        rope_set_len_max_q16(&h->rope, h->rope_len_q16);
                }
        }

        bool32 canuseitems = h->hashook &&
                             !hero_using_hook(h) &&
                             !hero_using_whip(h);

        // just pressed item button
        if (os_inp_just_pressed(INP_B)) {
                if (hero_using_hook(h)) {
                        hook_destroy(g, h, o, h->hook.o); // destroy hook if present
                        snd_play_ext(snd_get(SNDID_JUMP), 0.5f, 1.f);
                } else if (canuseitems) {
                        if (h->rope_len_q16 == HERO_ROPE_MIN) {
                                hero_use_whip(g, o, h);
                        } else {
                                hero_use_hook(g, o, h);
                        }
                }
        }

        int velsgn = sgn_i(o->vel_q8.x);
        int velabs = abs_i(o->vel_q8.x);
        int xacc   = 0;

        if (xs == 0) {
                o->drag_q8.x = (grounded ? 130 : 245);
        }

        if (xs != 0 && grounded) {
                if (velsgn == -xs) { // reverse direction
                        xacc         = 200;
                        o->drag_q8.x = 130;
                } else {
                        o->drag_q8.x = 250;
                        int accfrom  = pow_i32(velabs, 2);
                        int accto    = pow_i32(600, 2);
                        xacc         = lerp_i32(HERO_C_ACCX_MAX,
                                                0, // min acc
                                                accfrom,
                                                accto);
                }
        }

        bool32 ropestretched = hero_using_hook(h) && rope_stretched(g, &h->rope);

        if (ropestretched && !grounded) {
                // try to "jump up" on ledges when roped
                ropenode_s *rprev = o->ropenode->prev ? o->ropenode->prev : o->ropenode->next;
                v2_i32      vv    = v2_sub(rprev->p, o->ropenode->p);
                if (abs_i(vv.y) < 4 && ((vv.x < 0 && room_area_blocked(g, obj_rec_left(o))) ||
                                        (vv.x > 0 && room_area_blocked(g, obj_rec_right(o))))) {
                        PRINTF("JUMP\n");
                        o->vel_q8.y -= 250;
                        o->vel_q8.x -= sgn_i(vv.x) * 250;
                }
        }

        if (xs != 0 && !grounded) {
                o->drag_q8.x = 245;
                if (ropestretched) {
                        v2_i32 dtrope = v2_sub(h->rope.head->p, h->rope.head->next->p);
                        xacc          = (sgn_i(dtrope.x) == xs ? 15 : 30);
                } else if (velsgn == -xs) {
                        xacc = 75;
                } else if (velabs < 1000) {
                        xacc = 35;
                }
        }

        o->vel_q8.x += xacc * xs;

        if (!grounded && ropestretched) {
                o->drag_q8.x = 254;
        }

        if (os_inp_just_pressed(INP_UP) && grounded && o->vel_q8.x == 0 && o->vel_q8.y >= 0) {
                obj_s *interactable = obj_closest_interactable(g, obj_aabb_center(o));
                if (interactable && interactable->oninteract) {
                        interactable->oninteract(g, interactable);
                }
        }

        if (grounded && !h->wasgrounded && o->vel_prev_q8.y > 800) {
                hero_land_particles(g, o);
        }

        h->wasgrounded = grounded;
}

static void hook_update(game_s *g, obj_s *o, hero_s *h, obj_s *hook)
{
        rope_s *r = &h->rope;

        if (!rope_intact(g, &h->rope)) {
                hook_destroy(g, h, o, hook);
                return;
        }

        TIMING_BEGIN(TIMING_ROPE);
        if (!hook->attached) {
                v2_i32 hookp = hook->pos;
                obj_apply_movement(hook);
                actor_move(g, hook, hook->tomove.x, hook->tomove.y);
                hook->tomove.x  = 0;
                hook->tomove.y  = 0;
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
                if (room_area_blocked(g, hookrec)) {
                        snd_play_ext(snd_get(SNDID_HOOKATTACH), 0.5f, 0.8f);
                        hook->attached   = 1;
                        hook->gravity_q8 = (v2_i32){0};
                        hook->vel_q8     = (v2_i32){0};
                        obj_listc_s sld  = objbucket_list(g, OBJ_BUCKET_SOLID);
                        for (int n = 0; n < sld.n; n++) {
                                obj_s *solid = sld.o[n];
                                if (!solid_occupies(solid, hookrec)) continue;
                                hook->linkedsolid = objhandle_from_obj(solid);
                                break;
                        }
                }
        } else {
                // check if still attached
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
                if (hook->linkedsolid.o && !solid_occupies(hook->linkedsolid.o, hookrec)) {
                        hook->linkedsolid.o = NULL;
                }
                if (!room_area_blocked(g, hookrec)) {
                        hook->attached      = 0;
                        hook->linkedsolid.o = NULL;
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
        TIMING_END();
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

static void hook_squeeze(game_s *g, obj_s *o)
{
        hero_s *hero = (hero_s *)&g->hero;
        obj_s  *ohero;
        if (try_obj_from_handle(hero->obj, &ohero)) {
                hook_destroy(g, hero, ohero, o);
        } else {
                ASSERT(0);
        }
}

static void hero_squeeze(game_s *g, obj_s *o)
{
        // transition_start(g, "template.tmj");
}

static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8, i32 len_max_q16)
{
        obj_s *o = obj_create(g);
        obj_apply_flags(g, o, OBJ_FLAG_ACTOR);
        o->w            = 8;
        o->h            = 8;
        o->pos.x        = p.x - o->w / 2;
        o->pos.y        = p.y - o->h / 2;
        o->vel_q8       = v_q8;
        o->drag_q8.x    = 256;
        o->drag_q8.y    = 256;
        o->gravity_q8.y = 34;
        o->onsqueeze    = hook_squeeze;
        rope_init(rope);
        rope_set_len_max_q16(rope, len_max_q16);
        rope->head->p = p;
        rope->tail->p = p;
        o->ropenode   = rope->tail;
        o->rope       = rope;

        return o;
}

obj_s *hero_create(game_s *g, hero_s *h)
{
        obj_s  *hero  = obj_create(g);
        flags64 flags = OBJ_FLAG_ACTOR |
                        OBJ_FLAG_MOVABLE |
                        OBJ_FLAG_THINK_1 |
                        OBJ_FLAG_ANIMATE;
        obj_apply_flags(g, hero, flags);
        hero->animate_func = hero_animate;
        hero->think_1      = hero_update;
        hero->onsqueeze    = hero_squeeze;
        hero->facing       = 1;
        hero->actorflags   = ACTOR_FLAG_CLIMB_SLOPES |
                           ACTOR_FLAG_GLUE_GROUND;
        hero->pos.x        = 200;
        hero->pos.y        = 20;
        hero->w            = HERO_WIDTH;
        hero->h            = HERO_HEIGHT;
        hero->gravity_q8.y = HERO_C_GRAVITY;
        hero->drag_q8.x    = 256;
        hero->drag_q8.y    = 256; // no drag
        hero->ID           = 3;

        *h              = (const hero_s){0};
        h->hashook      = 1;
        h->rope_len_q16 = HERO_ROPE_MIN + 1;
        h->obj          = objhandle_from_obj(hero);
        return hero;
}

static void hero_jump_particles(game_s *g, obj_s *o)
{
        /*
        for (int i = 0; i < 6; i++) {
                particle_s *particle = particle_spawn(g);
                particle->ticks      = rng_range(4, 15);
                particle->p_q8       = (v2_i32){o->pos.x + o->w / 2,
                                                o->pos.y + o->h - 4};
                particle->p_q8       = v2_shl(particle->p_q8, 8);

                particle->p_q8.x += rng_range(-800, 800);
                particle->p_q8.y += rng_range(-800, 800);

                particle->v_q8.x = rng_range(-100, 100);
                particle->v_q8.y = rng_range(-100, 100);
        }
        */
}

static void hero_land_particles(game_s *g, obj_s *o)
{
        /*
        for (int i = 0; i < 12; i++) {
                particle_s *particle = particle_spawn(g);

                particle->ticks = rng_max_u16(&g->rng, 8) + 5;
                particle->p_q8  = (v2_i32){o->pos.x + o->w / 2, o->pos.y + o->h - 3};
                particle->p_q8  = v2_shl(particle->p_q8, 8);
                particle->a_q8  = (v2_i32){0, 50};

                particle->p_q8.x += rng_range(-800, 800);
                particle->p_q8.y += rng_range(-800, 800);

                particle->v_q8.x = rng_range(-300, 300);
                particle->v_q8.y = rng_range(-300, -100);
        }
        */
}

static void hero_check_level_transition(game_s *g, obj_s *hero)
{
        if (transition_active(g)) return;
        rec_i32 haabb = obj_aabb(hero);

        // clamp to current room and determine direction to which to slide
        int dir = 0;
        if (haabb.x <= 0) {
                dir     = DIRECTION_W;
                haabb.x = 0;
        }
        if (haabb.y <= 0) {
                dir     = DIRECTION_N;
                haabb.y = 0;
        }
        if (haabb.x + haabb.w >= g->pixel_x) {
                dir     = DIRECTION_E;
                haabb.x = g->pixel_x - haabb.w;
        }
        if (haabb.y + haabb.h >= g->pixel_y) {
                dir     = DIRECTION_S;
                haabb.y = g->pixel_y - haabb.h;
        }

        if (dir != 0) {
                rec_i32 gaabb = haabb;
                gaabb.x += g->curr_world_area->r.x;
                gaabb.y += g->curr_world_area->r.y;

                // now slide the player just outside the bounds of the current room
                // to enable overlapping of new room bounds
                switch (dir) {
                case DIRECTION_W: gaabb.x--; break;
                case DIRECTION_E: gaabb.x++; break;
                case DIRECTION_N: gaabb.y--; break;
                case DIRECTION_S: gaabb.y++; break;
                }
                world_area_def_s *wd = world_get_area(g->curr_world, g->curr_world_area, gaabb);
                if (wd == NULL) {
                        ASSERT(0);
                        // kill player or something?
                }
                gaabb.x -= wd->r.x;
                gaabb.y -= wd->r.y;

                v2_i32 teleportto = {0};
                switch (dir) {
                case DIRECTION_W:
                        teleportto.x = gaabb.x - HERO_WIDTH;
                        teleportto.y = gaabb.y;
                        break;
                case DIRECTION_E:
                        teleportto.x = gaabb.x + HERO_WIDTH;
                        teleportto.y = gaabb.y;
                        break;
                case DIRECTION_N:
                        teleportto.x = gaabb.x;
                        teleportto.y = gaabb.y - HERO_HEIGHT;
                        break;
                case DIRECTION_S:
                        teleportto.x = gaabb.x;
                        teleportto.y = gaabb.y + HERO_HEIGHT;
                        break;
                }

                g->curr_world_area = wd;
                transition_start(g, wd->filename, teleportto, dir);
        } else {
                // check for portals
                obj_listc_s triggers = objbucket_list(g, OBJ_BUCKET_NEW_AREA_COLLIDER);
                for (int n = 0; n < triggers.n; n++) {
                        obj_s *coll = triggers.o[n];
                        if (overlap_rec_excl(haabb, obj_aabb(coll))) {
                                transition_start(g, coll->filename, (v2_i32){0}, 0);
                                break;
                        }
                }
        }
}

static void hero_pickup_logic(game_s *g, hero_s *h, obj_s *o)
{
        rec_i32     haabb   = obj_aabb(o);
        obj_listc_s pickups = objbucket_list(g, OBJ_BUCKET_PICKUP);
        for (int n = 0; n < pickups.n; n++) {
                obj_s *p = pickups.o[n];
                if (!overlap_rec_excl(haabb, obj_aabb(p))) continue;

                obj_delete(g, p);
        }
}

static void hero_check_hurtables(game_s *g, obj_s *ohero)
{
        if (ohero->invincibleticks-- > 0) return;
        obj_listc_s hurtables = objbucket_list(g, OBJ_BUCKET_HURTS_PLAYER);
        rec_i32     haabb     = obj_aabb(ohero);
        for (int n = 0; n < hurtables.n; n++) {
                obj_s *ho = hurtables.o[n];
                if (overlap_rec_excl(obj_aabb(ho), haabb)) {
                        ohero->invincibleticks = 30;
                        v2_i32 c1              = obj_aabb_center(ho);
                        v2_i32 c2              = obj_aabb_center(ohero);
                        ohero->vel_q8.x        = sgn_i(c2.x - c1.x) * 750;
                        ohero->vel_q8.y        = -500;
                        break;
                }
        }
}

void hero_update(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)&g->hero;
        h->ppos   = o->pos;

        hero_logic(g, o, h);

        if (hero_using_hook(h)) {
                TIMING_BEGIN(TIMING_HERO_HOOK);
                obj_s *hook = obj_from_handle(h->hook);
                hook_update(g, o, h, hook);
                TIMING_END();
        } else if (hero_using_whip(h)) {
                h->whip_ticks--;
        }

        hero_check_level_transition(g, o);
        hero_pickup_logic(g, h, o);
        hero_check_hurtables(g, o);
}

bool32 hero_using_hook(hero_s *h)
{
        if (objhandle_is_valid(h->hook)) {
                ASSERT(h->obj.o->ropenode);
                return 1;
        }
        return 0;
}

bool32 hero_using_whip(hero_s *h)
{
        return h->whip_ticks > 0;
}

rec_i32 hero_whip_hitbox(hero_s *h)
{
        obj_s  *ohero   = h->obj.o;
        rec_i32 aabb    = obj_aabb(ohero);
        rec_i32 whipbox = {ohero->pos.x,
                           ohero->pos.y,
                           30,
                           20};
        if (ohero->facing == +1) {
                whipbox.x += ohero->w;
        } else {
                whipbox.x -= whipbox.w;
        }
        return whipbox;
}
