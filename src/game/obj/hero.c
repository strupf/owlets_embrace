// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game/game.h"
#include "game/rope.h"
#include "obj.h"

enum {
        HERO_C_JUMP_INIT = -600,
        HERO_C_ACCX_MAX  = 130,
        HERO_C_JUMP_MAX  = 80,
        HERO_C_JUMP_MIN  = 0,
        HERO_C_JUMPTICKS = 10,
        HERO_C_EDGETICKS = 6,
        HERO_C_GRAVITY   = 55,
};

static void   hero_update_sword(game_s *g, obj_s *o, hero_s *h);
static void   hero_interact_logic(game_s *g, hero_s *h, obj_s *o);
static void   hero_crank_item_selection(hero_s *h);
static void   hero_check_hurtables(game_s *g, obj_s *ohero);
static void   hero_jump_particles(game_s *g, obj_s *o);
static void   hero_land_particles(game_s *g, obj_s *o);
static void   hook_destroy(game_s *g, hero_s *h, obj_s *ohero, obj_s *ohook);
static obj_s *hook_create(game_s *g, rope_s *rope, v2_i32 p, v2_i32 v_q8);
static void   hook_squeeze(game_s *g, obj_s *o, void *arg);
static void   hero_squeeze(game_s *g, obj_s *o, void *arg);

static void obj_squeeze_delete(game_s *g, obj_s *o, void *arg)
{
        obj_delete(g, o);
}

static void arrow_think(game_s *g, obj_s *o, void *arg)
{
        rec_i32 aabb = obj_aabb(o);
        aabb.x--;
        aabb.y--;
        aabb.w += 2;
        aabb.h = 2;
        if (game_area_blocked(g, aabb)) {
                obj_delete(g, o);
        }
}

static obj_s *arrow_create(game_s *g, v2_i32 p, v2_i32 v_q8)
{
        obj_s     *arrow = obj_create(g);
        objflags_s flags = objflags_create(
            OBJ_FLAG_ACTOR,
            OBJ_FLAG_MOVABLE,
            OBJ_FLAG_THINK_1,
            OBJ_FLAG_KILL_OFFSCREEN);
        obj_set_flags(g, arrow, flags);
        arrow->think_1      = arrow_think;
        arrow->ID           = 2;
        arrow->w            = 8;
        arrow->h            = 8;
        arrow->pos.x        = p.x - arrow->w / 2;
        arrow->pos.y        = p.y - arrow->h / 2;
        arrow->vel_q8       = v_q8;
        arrow->gravity_q8.y = 12;
        arrow->drag_q8.x    = 256;
        arrow->drag_q8.y    = 256;
        arrow->onsqueeze    = obj_squeeze_delete;
        return arrow;
}

static void hero_use_bow(game_s *g, obj_s *o, hero_s *h)
{
        int    xs    = o->facing;
        v2_i32 ctr   = obj_aabb_center(o);
        v2_i32 v_q8  = {xs * 1500, -300};
        obj_s *arrow = arrow_create(g, ctr, v_q8);
}

static void hero_use_hook(game_s *g, obj_s *o, hero_s *h)
{
        obj_s *hook;
        if (try_obj_from_handle(h->hook, &hook)) {
                hook_destroy(g, h, o, hook); // destroy hook if present
                snd_play_ext(snd_get(SNDID_JUMP), 0.5f, 1.f);
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

rec_i32 hero_sword_hitbox(obj_s *o, hero_s *h)
{
        rec_i32 hitbox = {o->pos.x + o->w, o->pos.y, 16, 16};
        if (h->sworddir == -1) {
                hitbox.x -= o->w + hitbox.w;
        }
        return hitbox;
}

static void hero_update_sword(game_s *g, obj_s *o, hero_s *h)
{
        h->swordticks--;
        rec_i32 hitbox = hero_sword_hitbox(o, h);

        obj_listc_s hittable = objbucket_list(g, OBJ_BUCKET_HURTABLE);
        for (int n = 0; n < hittable.n; n++) {
                obj_s *ohit = hittable.o[n];
                if (overlap_rec_excl(obj_aabb(ohit), hitbox)) {
                        obj_delete(g, ohit);
                }
        }
}

static void hero_use_sword(game_s *g, obj_s *o, hero_s *h)
{
        h->swordticks = 40;
        h->sworddir   = o->facing;
        hero_update_sword(g, o, h);
}

static void hero_logic(game_s *g, obj_s *o, hero_s *h)
{
        int xs = os_inp_dpad_x();
        if (xs != 0 && xs != o->facing) {
                o->facing      = xs;
                h->facingticks = 0;
        } else {
                h->facingticks += o->facing;
        }

        bool32 canuseitems = 1;
        bool32 grounded    = game_area_blocked(g, obj_rec_bottom(o));
        if (grounded) {
                h->edgeticks = HERO_C_EDGETICKS;
        } else if (h->edgeticks > 0) {
                h->edgeticks--;
        }

        if (o->vel_prev_q8.y > 700 && grounded) {
                snd_play(snd_get(SNDID_HERO_LAND));
        }

        if (os_inp_pressed(INP_A)) {
                if (!os_inp_pressedp(INP_A) && h->edgeticks > 0) {
                        h->edgeticks = 0;
                        h->jumpticks = HERO_C_JUMPTICKS;
                        o->vel_q8.y  = HERO_C_JUMP_INIT;
                        snd_play_ext(snd_get(SNDID_JUMP), 0.15f, 1.f);
                        hero_jump_particles(g, o);
                } else if (h->jumpticks > 0) {
                        int jfrom = pow2_i32(HERO_C_JUMPTICKS - h->jumpticks--);
                        int jto   = pow2_i32(HERO_C_JUMPTICKS);
                        o->vel_q8.y -= lerp_i32(HERO_C_JUMP_MAX,
                                                HERO_C_JUMP_MIN,
                                                jfrom,
                                                jto);
                }
        } else {
                h->jumpticks = 0;
        }

        if (h->swordticks > 0) {
                hero_update_sword(g, o, h);
                canuseitems = 0;
        }

        // just pressed item button
        if (canuseitems && os_inp_just_pressed(INP_B)) {
                switch (h->selected_item) {
                case HERO_ITEM_HOOK:
                        hero_use_hook(g, o, h);
                        break;
                case HERO_ITEM_BOW:
                        hero_use_bow(g, o, h);
                        break;
                case HERO_ITEM_SWORD:
                        hero_use_sword(g, o, h);
                        break;
                }
        }

        int    velsgn        = SGN(o->vel_q8.x);
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
                                int accto    = pow_i32(600, 2);
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

        if (os_inp_just_pressed(INP_UP) && grounded &&
            o->vel_q8.x == 0 && o->vel_q8.y >= 0) {
                hero_interact_logic(g, h, o);
        }

        if (grounded && !h->wasgrounded && o->vel_prev_q8.y > 800) {
                hero_land_particles(g, o);
        }

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

        TIMING_BEGIN(TIMING_ROPE);
        if (!hook->attached) {
                v2_i32 hookp = hook->pos;
                obj_apply_movement(hook);
                actor_move_x(g, hook, hook->tomove.x);
                actor_move_y(g, hook, hook->tomove.y);
                hook->tomove.x  = 0;
                hook->tomove.y  = 0;
                rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
                if (game_area_blocked(g, hookrec)) {
                        snd_play_ext(snd_get(SNDID_HOOK), 0.5f, 0.8f);
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
                if (!game_area_blocked(g, hookrec)) {
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

static void hook_squeeze(game_s *g, obj_s *o, void *arg)
{
        hero_s *hero = (hero_s *)arg;
        obj_s  *ohero;
        if (try_obj_from_handle(hero->obj, &ohero)) {
                hook_destroy(g, hero, ohero, o);
        } else {
                ASSERT(0);
        }
}

static void hero_squeeze(game_s *g, obj_s *o, void *arg)
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
        o->userarg      = &g->hero;
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
                                           OBJ_FLAG_HERO,
                                           OBJ_FLAG_MOVABLE,
                                           OBJ_FLAG_THINK_1);
        hero->userarg    = h;
        obj_set_flags(g, hero, flags);
        hero->think_1    = hero_update;
        hero->onsqueeze  = hero_squeeze;
        hero->facing     = 1;
        hero->actorflags = ACTOR_FLAG_CLIMB_SLOPES |
                           ACTOR_FLAG_GLUE_GROUND;
        hero->pos.x        = 200;
        hero->pos.y        = 20;
        hero->w            = 16;
        hero->h            = 24;
        hero->gravity_q8.y = HERO_C_GRAVITY;
        hero->drag_q8.x    = 256;
        hero->drag_q8.y    = 256; // no drag
        hero->ID           = 3;

        *h     = (const hero_s){0};
        h->obj = objhandle_from_obj(hero);
        hero_aquire_item(h, HERO_ITEM_BOW);
        hero_aquire_item(h, HERO_ITEM_HOOK);
        hero_aquire_item(h, HERO_ITEM_SWORD);
        return hero;
}

static void hero_jump_particles(game_s *g, obj_s *o)
{
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
}

static void hero_land_particles(game_s *g, obj_s *o)
{
        for (int i = 0; i < 12; i++) {
                particle_s *particle = particle_spawn(g);

                particle->ticks = rng_max_u16(&g->rng, 8) + 5;
                particle->p_q8  = (v2_i32){o->pos.x + o->w / 2,
                                           o->pos.y + o->h - 3};
                particle->p_q8  = v2_shl(particle->p_q8, 8);
                particle->a_q8  = (v2_i32){0, 50};

                particle->p_q8.x += rng_range(-800, 800);
                particle->p_q8.y += rng_range(-800, 800);

                particle->v_q8.x = rng_range(-300, 300);
                particle->v_q8.y = rng_range(-300, -100);
        }
}

static void hero_check_level_transition(game_s *g, obj_s *hero)
{
        rec_i32 haabb = obj_aabb(hero);

        direction_e dir = 0;
        if (haabb.x <= 0) dir = DIRECTION_W;
        if (haabb.y <= 0) dir = DIRECTION_N;
        if (haabb.x + haabb.w >= g->pixel_x) dir = DIRECTION_E;
        if (haabb.y + haabb.h >= g->pixel_y) dir = DIRECTION_S;

        if (dir) {
                rec_i32 gaabb = haabb;
                gaabb.x += g->roomlayout.curr->r.x;
                gaabb.y += g->roomlayout.curr->r.y;
                switch (dir) {
                case DIRECTION_NONE: ASSERT(0); break;
                case DIRECTION_W: gaabb.x--; break;
                case DIRECTION_E: gaabb.x++; break;
                case DIRECTION_N: gaabb.y--; break;
                case DIRECTION_S: gaabb.y++; break;
                }
                roomdesc_s *rd = roomlayout_get(&g->roomlayout, gaabb);
                ASSERT(rd);
                g->roomlayout.curr      = rd;
                g->transition.enterfrom = dir;
                g->transition.heroprev  = haabb;
                g->transition.camprev   = g->cam;
                game_map_transition_start(g, rd->filename);
                return;
        }
        obj_listc_s triggers = objbucket_list(g, OBJ_BUCKET_NEW_AREA_COLLIDER);
        for (int n = 0; n < triggers.n; n++) {
                obj_s *coll = triggers.o[n];
                if (overlap_rec_excl(haabb, obj_aabb(coll))) {
                        game_map_transition_start(g, coll->filename);
                        break;
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

                if (p->pickup.action) {
                        p->pickup.action(g, p->pickup.actionarg);
                }
                h->pickups += p->pickup.x;
                obj_delete(g, p);
        }
}

static void hero_interact_logic(game_s *g, hero_s *h, obj_s *o)
{
        v2_i32 pc           = obj_aabb_center(o);
        obj_s *interactable = interactable_closest(g, pc);
        if (interactable)
                interactable->oninteract(g, interactable, NULL);
}

static void hero_crank_item_selection(hero_s *h)
{
        if (h->aquired_items == 0) return;

        // crank item input
        int crankp_q16  = os_inp_crankp();
        int crankc_q16  = os_inp_crank();
        int crankchange = os_inp_crank_change();

        // here we check if the crank "flipped over" the 180 deg position
        if (crankchange > 0 &&
            (crankp_q16 < 0x8000 && crankc_q16 >= 0x8000)) {
                hero_set_cur_item(h, h->selected_item_next);
        } else if (crankchange < 0 &&
                   (crankp_q16 >= 0x8000 && crankc_q16 < 0x8000)) {
                hero_set_cur_item(h, h->selected_item_prev);
        }
}

static void hero_check_hurtables(game_s *g, obj_s *ohero)
{
        obj_listc_s hurtables = objbucket_list(g, OBJ_BUCKET_HURTS_PLAYER);
        rec_i32     haabb     = obj_aabb(ohero);
        for (int n = 0; n < hurtables.n; n++) {
                obj_s *ho = hurtables.o[n];
                if (overlap_rec_excl(obj_aabb(ho), haabb)) {
                        ohero->invincibleticks = 30;
                        v2_i32 c1              = obj_aabb_center(ho);
                        v2_i32 c2              = obj_aabb_center(ohero);
                        ohero->vel_q8.x        = SGN(c2.x - c1.x) * 750;
                        ohero->vel_q8.y        = -500;
                        break;
                }
        }
}

void hero_update(game_s *g, obj_s *o, void *arg)
{
        hero_s *h = (hero_s *)arg;

        hero_crank_item_selection(h);
        hero_logic(g, o, h);

        TIMING_BEGIN(TIMING_HERO_HOOK);
        obj_s *hook;
        if (try_obj_from_handle(h->hook, &hook)) {
                hero_hook_update(g, o, h, hook);
        }

        TIMING_END();

        if (g->transition.phase == TRANSITION_NONE) {
                hero_check_level_transition(g, o);
        }
        hero_pickup_logic(g, h, o);

        if (o->invincibleticks-- <= 0) {
                hero_check_hurtables(g, o);
        }

        o->animation += (int)((float)ABS(o->vel_q8.x) * 0.1f);

        bool32 grounded   = game_area_blocked(g, obj_rec_bottom(o));
        o->animframe_prev = o->animframe;
        if (o->vel_prev_q8.x == 0 && o->vel_q8.x != 0) {
                o->animation = 300;
        }

        if (o->vel_q8.x == 0 && grounded) {
                o->animation += 10;
                o->animframe = 0;
        } else {
                o->animframe = (o->animation / 500) % 4;
                if (grounded && o->animframe % 2 == 1 && o->animframe_prev != o->animframe) {
                        snd_play_ext(snd_get(SNDID_STEP), 0.5f, rngf_range(0.8f, 1.f));
                }
        }
}

static inline int i_hero_itemID_get(hero_s *h, int dir)
{
        ASSERT(h->aquired_items != 0);
        int i = h->selected_item;
        while (1) {
                i = (i + dir + NUM_HERO_ITEMS) % NUM_HERO_ITEMS;
                if (h->aquired_items & (1 << i))
                        return i;
        }
        return i;
}

void hero_set_cur_item(hero_s *h, int itemID)
{
        if (h->aquired_items == 0) return;

        ASSERT(h->aquired_items & (1 << itemID));
        h->selected_item      = itemID;
        h->selected_item_prev = i_hero_itemID_get(h, -1);
        h->selected_item_next = i_hero_itemID_get(h, +1);
}

void hero_aquire_item(hero_s *h, int itemID)
{
        ASSERT(((h->aquired_items >> itemID) & 1) == 0);

        h->aquired_items |= 1 << itemID;
        hero_set_cur_item(h, itemID);
}