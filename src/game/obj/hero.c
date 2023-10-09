// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game/game.h"
#include "game/obj.h"
#include "game/rope.h"

static const bgpartice_desc_s g_heroparticles   = {0, 0, 4 << 8, 2 << 8, // pos, spread
                                                   0, -100, 100, 50,     // vel, spread
                                                   0, 10, 0, 4,          // acc, spread
                                                   30, 10, 2, 24};       // tick, spread, size, n
static const bgpartice_desc_s g_heroparticles_1 = {0, 0, 4 << 8, 4 << 8, // pos, spread
                                                   0, 0, 100, 100,       // vel, spread
                                                   0, 0, 0, 0,           // acc, spread
                                                   30, 10, 2, 4};        // tick, spread, size, n

static void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);
static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8, i32 len_max);
static void   hook_squeeze(game_s *g, obj_s *o);
static void   hero_check_hurtables(game_s *g, obj_s *ohero);
static void   hero_jump_particles(game_s *g, obj_s *o);
static void   hero_land_particles(game_s *g, obj_s *o);
static void   hero_squeeze(game_s *g, obj_s *o);
static void   hero_animate(game_s *g, obj_s *o);

static void hero_animate(game_s *g, obj_s *o)
{
        hero_s *h           = (hero_s *)o;
        bool32  grounded    = room_area_blocked(g, obj_rec_bottom(o));
        bool32  onladder    = h->onladder;
        i32     vx          = o->vel_q8.x;
        bool32  touch_right = room_area_blocked(g, obj_rec_right(o));
        bool32  touch_left  = room_area_blocked(g, obj_rec_left(o));

        if (grounded) {
                if (vx == 0) {
                        if (h->anim != HERO_ANIM_IDLE) {
                                h->anim      = HERO_ANIM_IDLE;
                                h->animstate = 0;
                        }
                } else {
                        if ((vx < 0 && touch_left) ||
                            (vx > 0 && touch_right)) {
                                if (h->anim != HERO_ANIM_IDLE) {
                                        h->anim      = HERO_ANIM_IDLE;
                                        h->animstate = 0;
                                }
                        } else {
                                if (h->anim != HERO_ANIM_WALKING) {
                                        h->anim      = HERO_ANIM_WALKING;
                                        h->animstate = 0;
                                }
                        }
                }
        }

        h->animstate++;
}

static void hero_use_whip(game_s *g, obj_s *o)
{
        hero_s *h     = (hero_s *)o;
        h->whip_ticks = HERO_C_WHIP_TICKS;
}

static void hero_use_hook(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)o;

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
        rope_s *rope    = &g->rope;
        obj_s  *hook    = hook_create(g, rope, center, vlaunch, h->rope_len_q16);
        h->hook         = objhandle_from_obj(hook);
        h->rope_len_q16 = HERO_ROPE_MAX;
        rope_set_len_max_q16(rope, h->rope_len_q16);
        o->ropenode = rope->head;
        o->rope     = rope;
}

static void hero_unset_ladder(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)o;
        obj_set_flags(g, o, OBJ_FLAG_MOVABLE);
        h->onladder = 0;
}

// returns 1 if hero snapped to a ladder, otherwise 0
static bool32 hero_try_catch_ladder(game_s *g, obj_s *o)
{
        hero_s *h       = (hero_s *)o;
        v2_i32  lp      = obj_aabb_center(o);
        rec_i32 ladderr = obj_aabb(o);
        ladderr.x       = ((lp.x >> 4) << 4) + 8 - ladderr.w / 2;
        ladderr.y       = ((lp.y >> 4) << 4) + 16 - ladderr.h - 1;
        if (!room_is_ladder(g, lp) || room_area_blocked(g, ladderr)) {
                return 0;
        }

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
        h->ladderx       = o->pos.x;
        obj_unset_flags(g, o, OBJ_FLAG_MOVABLE);
        return 1;
}

static void hero_use_item(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)o;

        if (hero_using_hook(o)) {
                hook_destroy(g, o, h->hook.o); // destroy hook if present
                return;
        }

        bool32 canuseitems = h->hashook &&
                             !hero_using_hook(o) &&
                             !hero_using_whip(o);
        if (!canuseitems) return;

        if (h->rope_len_q16 == HERO_ROPE_MIN) {
                hero_use_whip(g, o);
        } else {
                hero_use_hook(g, o);
        }
}

static void hero_logic_ladder(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)o;

        bool32 grounded = room_area_blocked(g, obj_rec_bottom(o));

        if (grounded) {
                hero_unset_ladder(g, o);
                return;
        }

        if (o->pos.x != h->ladderx) {
                hero_unset_ladder(g, o);
                return;
        }

        if (os_inp_just_pressed(INP_A)) {
                h->edgeticks = 1;
                o->vel_q8.x  = os_inp_dpad_x() * 300;
                hero_unset_ladder(g, o);
                return;
        }

        if (os_inp_dpad_y() < 0) {
                v2_i32 lp = obj_aabb_center(o);
                lp.y -= 2;
                if (room_is_ladder(g, lp)) {
                        o->tomove.y -= 2;
                }
        } else if (os_inp_dpad_y() > 0) {
                o->tomove.y += 2;
        }
}

static void hero_logic(game_s *g, obj_s *o)
{
        hero_s *h = (hero_s *)o;

        if (h->onladder) {
                hero_logic_ladder(g, o);
                return;
        }

        if (os_inp_just_pressed(INP_UP) || os_inp_just_pressed(INP_DOWN)) {
                if (hero_try_catch_ladder(g, o))
                        return;
        }

        bool32 grounded = room_area_blocked(g, obj_rec_bottom(o));

        int dpad_x = os_inp_dpad_x();

        if (!h->locked_facing && dpad_x == -o->facing) {
                o->facing = dpad_x;
        }

        static int loadedonce = 0;
        static int jumpinit   = 0;
        static int jumpticks  = 0;
        static int jumpmax    = 0;
        static int jumpmin    = 0;
        static int gravity    = 0;
        static int djumpticks = 0;
        static int djumpmax   = 0;
        static int djumpmin   = 0;

        if (debug_inp_space() || !loadedonce) {
                loadedonce = 1;
                os_spmem_push();
                char *txt = txt_read_file_alloc("assets/playervars.json", os_spmem_alloc);
                jsn_s jroot;
                jsn_root(txt, &jroot);
                jumpinit        = jsn_intk(jroot, "jumpinit");
                jumpticks       = jsn_intk(jroot, "jumpticks");
                jumpmax         = jsn_intk(jroot, "jumpmax");
                jumpmin         = jsn_intk(jroot, "jumpmin");
                gravity         = jsn_intk(jroot, "gravity");
                djumpticks      = jsn_intk(jroot, "djumpticks");
                djumpmax        = jsn_intk(jroot, "djumpmax");
                djumpmin        = jsn_intk(jroot, "djumpmin");
                o->gravity_q8.y = gravity;
                os_spmem_pop();
        }

        if (h->jumpticks > 0) {
                if (os_inp_pressed(INP_A)) {
                        h->jumpticks--;
                } else {
                        h->jumpticks >>= 1;
                }

                int j0 = pow2_i32(jumpticks - h->jumpticks);
                int j1 = pow2_i32(jumpticks);
                int vy = jumpmax + ((jumpmin - jumpmax) * j0) / j1;
                o->vel_q8.y -= vy;
        }

        if (grounded) {
                h->edgeticks = HERO_C_EDGETICKS;
        } else {
                h->edgeticks--;
        }

        // jumping
        if (os_inp_just_pressed(INP_A)) {
                if (h->edgeticks > 0) {
                        h->edgeticks = 0;
                        h->jumpticks = jumpticks;
                        o->vel_q8.y  = jumpinit;

                        bgpartice_desc_s desc = g_heroparticles;
                        desc.p_q8.x           = (o->pos.x + o->w / 2) << 8;
                        desc.p_q8.y           = (o->pos.y + o->h - 4) << 8;
                        backforeground_spawn_particles(&g->backforeground, desc);
                } else if (hero_using_hook(o) && !grounded) {
                        if (room_area_blocked(g, obj_rec_right(o))) {
                                o->vel_q8.x = -600;
                        }
                        if (room_area_blocked(g, obj_rec_left(o))) {
                                o->vel_q8.x = +600;
                        }
                }
        }

        if (hero_using_hook(o) && h->hook.o->attached) {
                h->rope_len_q16 += os_inp_dpad_y() * HERO_ROPE_REEL_RATE;
                h->rope_len_q16 = clamp_i(h->rope_len_q16, HERO_ROPE_MIN, HERO_ROPE_MAX);
                rope_set_len_max_q16(&g->rope, h->rope_len_q16);
        }

        // just pressed item button
        if (os_inp_just_pressed(INP_B)) {
                hero_use_item(g, o);
        }

        h->wasgrounded = grounded;
        h->gliding     = (!grounded &&
                      !hero_using_hook(o) &&
                      !h->onladder &&
                      os_inp_pressed(INP_UP));
        if (h->gliding) {
                // todo
        }

        int velsgn = sgn_i(o->vel_q8.x);
        int velabs = abs_i(o->vel_q8.x);
        int xacc   = 0;

        if (dpad_x == 0) {
                o->drag_q8.x = (grounded ? 130 : 245);
        }

        if (dpad_x != 0 && grounded) {
                if (velsgn == -dpad_x) { // reverse direction
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

        bool32 ropestretched = hero_using_hook(o) && rope_stretched(g, &g->rope);

        if (dpad_x != 0 && !grounded) {
                o->drag_q8.x = 245;
                if (ropestretched) {
                        v2_i32 dtrope = v2_sub(g->rope.head->p, g->rope.head->next->p);
                        xacc          = (sgn_i(dtrope.x) == dpad_x ? 15 : 30);
                } else if (velsgn == -dpad_x) {
                        xacc = 75;
                } else if (velabs < 1000) {
                        xacc = 35;
                }
        }

        o->vel_q8.x += xacc * dpad_x;

        if (!grounded && ropestretched) {
                o->drag_q8.x = 254;
        }

        if (os_inp_just_pressed(INP_UP) && grounded && o->vel_q8.x == 0 && o->vel_q8.y >= 0) {
                obj_s *interactable = obj_closest_interactable(g, obj_aabb_center(o));
                if (interactable && interactable->oninteract) {
                        interactable->oninteract(g, interactable);
                }
        }
}

static void hook_update(game_s *g, obj_s *o, obj_s *hook)
{
        hero_s *h = (hero_s *)o;
        rope_s *r = &g->rope;

        if (!rope_intact(g, r)) {
                hook_destroy(g, o, hook);
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
                        h->rope_len_q16 = rope_len_q16(g, r);
                        rope_set_len_max_q16(r, h->rope_len_q16);
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

        rope_update(g, &g->rope);
        if (hook->attached) {
                o->vel_q8 = rope_adjust_connected_vel(g, r, r->head,
                                                      o->subpos_q8, o->vel_q8);
        } else {
                hook->vel_q8 = rope_adjust_connected_vel(g, r, r->tail,
                                                         hook->subpos_q8, hook->vel_q8);
        }
        TIMING_END();
}

static void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
        hero_s *h = (hero_s *)ohero;
        obj_delete(g, ohook);
        h->hook.o       = NULL;
        ohero->ropenode = NULL;
        ohero->rope     = NULL;
        ohook->rope     = NULL;
        ohook->ropenode = NULL;
}

static void hook_squeeze(game_s *g, obj_s *o)
{
        /*
        obj_s *ohero;
        if (try_obj_from_handle(hero->obj, &ohero)) {
                hook_destroy(g, hero, ohero, o);
        } else {
                ASSERT(0);
        }
        */
        NOT_IMPLEMENTED
}

static void hero_squeeze(game_s *g, obj_s *o)
{
        NOT_IMPLEMENTED
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
        o->ID           = OBJ_ID_HOOK;
        rope_init(rope);
        rope_set_len_max_q16(rope, len_max_q16);
        rope->head->p = p;
        rope->tail->p = p;
        o->ropenode   = rope->tail;
        o->rope       = rope;

        return o;
}

obj_s *hero_create(game_s *g)
{
        obj_s  *hero  = obj_create(g);
        flags64 flags = OBJ_FLAG_ACTOR |
                        OBJ_FLAG_MOVABLE |
                        OBJ_FLAG_THINK_1 |
                        OBJ_FLAG_ANIMATE;
        obj_apply_flags(g, hero, flags);
        obj_tag(g, hero, OBJ_TAG_HERO);
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
        hero->ID           = OBJ_ID_HERO;

        hero_s *h       = (hero_s *)hero;
        h->hashook      = 1;
        h->rope_len_q16 = HERO_ROPE_MIN + 1;
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

static void hero_pickup_logic(game_s *g, obj_s *o)
{
        hero_s     *h       = (hero_s *)o;
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
        hero_s *h = (hero_s *)ohero;
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
        hero_s *h = (hero_s *)o;
        h->ppos   = o->pos;

        hero_logic(g, o);

        if (hero_using_hook(o)) {
                TIMING_BEGIN(TIMING_HERO_HOOK);
                obj_s *hook = obj_from_handle(h->hook);
                hook_update(g, o, hook);
                TIMING_END();
        } else if (hero_using_whip(o)) {
                h->whip_ticks--;
        }

        hero_check_level_transition(g, o);
        hero_pickup_logic(g, o);
        hero_check_hurtables(g, o);
}

bool32 hero_using_hook(obj_s *o)
{
        hero_s *h = (hero_s *)o;
        if (objhandle_is_valid(h->hook)) {
                ASSERT(o->ropenode);
                return 1;
        }
        return 0;
}

bool32 hero_using_whip(obj_s *o)
{
        hero_s *h = (hero_s *)o;
        return h->whip_ticks > 0;
}

rec_i32 hero_whip_hitbox(obj_s *o)
{
        hero_s *h       = (hero_s *)o;
        rec_i32 aabb    = obj_aabb(o);
        rec_i32 whipbox = {o->pos.x,
                           o->pos.y,
                           30,
                           20};
        if (o->facing == +1) {
                whipbox.x += o->w;
        } else {
                whipbox.x -= whipbox.w;
        }
        return whipbox;
}
