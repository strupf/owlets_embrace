// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "assets.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN 500
#define HERO_ROPE_LEN_MAX 4000

static void hero_set_cur_item(hero_s *h, int item);

obj_s *obj_hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->flags |= OBJ_FLAG_MOVER;
    o->flags |= OBJ_FLAG_TILE_COLLISION;
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_CLAMP_TO_ROOM;
    o->moverflags |= OBJ_MOVER_SLOPES;
    o->moverflags |= OBJ_MOVER_GLUE_GROUND;
    o->drag_q8.x          = 256;
    o->drag_q8.y          = 256;
    o->gravity_q8.y       = 60;
    o->w                  = 10;
    o->h                  = 20;
    o->spriteanim[0].data = asset_anim(ANIMID_HERO);
    o->spriteanim[0].mode = SPRITEANIM_MODE_LOOP;
    o->facing             = 1;
    return o;
}

bool32 hero_has_upgrade(hero_s *h, int upgrade)
{
    return (h->aquired_upgrades & (1U << upgrade));
}

void hero_aquire_upgrade(hero_s *h, int upgrade)
{
    if (hero_has_upgrade(h, upgrade)) {
        sys_printf("Already has upgrade: %i\n", upgrade);
        return;
    }
    h->aquired_upgrades |= 1 << upgrade;

    switch (upgrade) {
    case HERO_UPGRADE_HOOK:
        h->aquired_items |= 1 << HERO_ITEM_HOOK;
        hero_set_cur_item(h, HERO_ITEM_HOOK);
        break;
    case HERO_UPGRADE_WHIP:
        h->aquired_items |= 1 << HERO_ITEM_WHIP;
        hero_set_cur_item(h, HERO_ITEM_WHIP);
        break;
    }
}

obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HOOK;
    obj_tag(g, o, OBJ_TAG_HOOK);
    o->flags |= OBJ_FLAG_ACTOR;
    o->w            = 8;
    o->h            = 8;
    o->pos.x        = p.x - o->w / 2;
    o->pos.y        = p.y - o->h / 2;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->gravity_q8.y = 70;
    o->vel_q8       = v_q8;

    rope_init(r);
    rope_set_len_max_q4(r, HERO_ROPE_LEN_MAX);
    r->tail->p  = p;
    r->head->p  = p;
    o->rope     = r;
    o->ropenode = o->rope->tail;
    return o;
}

void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
    obj_delete(g, ohook);
    ohero->ropenode = NULL;
    ohero->rope     = NULL;
    ohook->rope     = NULL;
    ohook->ropenode = NULL;
    g->n_ropes      = 0;
}

void hook_update(game_s *g, obj_s *hook)
{
    rope_s *r = hook->rope;
    obj_s  *h;
    if (!obj_try_from_obj_handle(hook->obj_handles[0], &h))
        return;

    if (!rope_intact(g, r)) {
        hook_destroy(g, h, hook);
        return;
    }

    if (!hook->attached) {
        v2_i32 hookp = hook->pos;
        obj_apply_movement(hook);
        actor_move(g, hook, hook->tomove);
        hook->tomove.x  = 0;
        hook->tomove.y  = 0;
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
        if (!game_traversable(g, hookrec)) {
            g->herodata.ropelen = rope_length_q4(g, r);
            snd_play_ext(asset_snd(SNDID_HOOK_ATTACH), 1.f, 1.f);
            rope_set_len_max_q4(r, g->herodata.ropelen);
            hook->attached   = 1;
            hook->gravity_q8 = (v2_i32){0};
            hook->vel_q8     = (v2_i32){0};

            for (int i = 0; i < g->obj_nbusy; i++) {
                obj_s *solid = g->obj_busy[i];
                if ((solid->flags & OBJ_FLAG_SOLID) &&
                    overlap_rec(hookrec, obj_aabb(solid))) {
                    int kk             = overlap_rec(hookrec, obj_aabb(solid));
                    hook->linked_solid = obj_handle_from_obj(solid);
                    break;
                }
            }
        }
    } else {
        // check if still attached
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};

        obj_s *solid;
        if (obj_try_from_obj_handle(hook->linked_solid, &solid) &&
            !overlap_rec(hookrec, obj_aabb(solid))) {
            hook->linked_solid.o = NULL;
        }

        if (game_traversable(g, hookrec)) {
            hook->attached       = 0;
            hook->linked_solid.o = NULL;
        }
    }

    rope_update(g, &g->rope);
    if (hook->attached) {
        h->vel_q8 = rope_adjust_connected_vel(g, r, r->head, h->subpos_q8, h->vel_q8);
    } else {
        hook->vel_q8 = rope_adjust_connected_vel(g, r, r->tail, hook->subpos_q8, hook->vel_q8);
    }
}

void hero_use_hook(game_s *g, obj_s *h, hero_s *hero)
{
    // throw new hook
    int dirx = inp_dpad_x();
    int diry = inp_dpad_y();
    if (dirx == 0 && diry == 0) diry = -1;

    v2_i32 center  = obj_pos_center(h);
    v2_i32 vlaunch = {dirx, diry};

    vlaunch = v2_shl(vlaunch, 11);
    if (vlaunch.y < 0) {
        vlaunch.y = vlaunch.y * 5 / 4;
    }
    if (vlaunch.y == 0) {
        vlaunch.y = -500;
        vlaunch.x = vlaunch.x * 5 / 4;
    }

    rope_s *rope         = &g->rope;
    obj_s  *hook         = hook_create(g, rope, center, vlaunch);
    h->obj_handles[0]    = obj_handle_from_obj(hook);
    hook->obj_handles[0] = obj_handle_from_obj(h);
    h->ropenode          = rope->head;
    h->rope              = rope;
    g->n_ropes           = 1;
    g->ropes[0]          = rope;
}

static void hero_use_item(game_s *g, obj_s *o, hero_s *hero)
{
    if (hero->attack != HERO_ATTACK_NONE) {
        if (hero->attack_tick > 5) return;
        hero->attack_tick = 0; // animation cancel
    }

    obj_s *ohook_;
    if (obj_try_from_obj_handle(o->obj_handles[0], &ohook_)) {
        hook_destroy(g, o, ohook_);
        return;
    }

    switch (hero->selected_item) {
    case HERO_ITEM_HOOK: {
        sys_printf("1\n");
        hero_use_hook(g, o, hero);
    } break;
    case HERO_ITEM_BOMB: {

    } break;
    case HERO_ITEM_WHIP: {
        int dx              = inp_dpad_x();
        int dy              = inp_dpad_y();
        hero->attack_tick   = 20;
        hero->facing_locked = 1;
        if (dy == -1 && dx == 0)
            hero->attack = HERO_ATTACK_UP;
        if (dy == +1 && dx == 0)
            hero->attack = HERO_ATTACK_DOWN;
        if (dy == -1 && dx != 0)
            hero->attack = HERO_ATTACK_DIA_UP;
        if (dy == +1 && dx != 0)
            hero->attack = HERO_ATTACK_DIA_DOWN;
        if (dy == 0)
            hero->attack = HERO_ATTACK_SIDE;
    } break;
    }
}

void hero_update(game_s *g, obj_s *o)
{
    hero_s *hero = &g->herodata;

    if (hero->attack != HERO_ATTACK_NONE) {
        hitbox_s hitboxes[4] = {0};
        hitboxes[0].damage   = 1;
        hitboxes[0].r        = obj_aabb(o);
        switch (hero->attack) {
        case HERO_ATTACK_UP:
            break;
        case HERO_ATTACK_DOWN:
            break;
        case HERO_ATTACK_DIA_UP:
            break;
        case HERO_ATTACK_DIA_DOWN:
            break;
        case HERO_ATTACK_SIDE:
            break;
        }

        game_apply_hitboxes(g, hitboxes, 1);
        if (--hero->attack_tick <= 0) {
            hero->attack        = HERO_ATTACK_NONE;
            hero->facing_locked = 0;
        }
    }

    int    dpad_x   = inp_dpad_x();
    bool32 grounded = !game_traversable(g, obj_rec_bottom(o));
    if (grounded) {
        o->edgeticks = 6;
    } else {
        o->edgeticks--;
    }

    if (dpad_x != 0 && !hero->facing_locked) {
        o->facing = dpad_x;
    }

    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;

    if (0 < o->jumpticks && o->edgeticks <= 0) {
        if (inp_pressed(INP_A)) {
            o->jumpticks--;
        } else {
            o->jumpticks >>= 1;
        }

        int j0 = pow2_i32(20 - o->jumpticks);
        int j1 = pow2_i32(20);
        int vy = 60 + ((0 - 60) * j0) / j1;
        o->vel_q8.y -= vy;
    } else {
        if (inp_just_pressed(INP_A) && 0 < o->edgeticks) { // jumping
            o->edgeticks = 0;
            o->jumpticks = 20;
            o->vel_q8.y  = -800;
        }
    }

    if (inp_just_pressed(INP_B)) {
        hero_use_item(g, o, hero);
    }

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook && ohook->attached) {
        hero->ropelen += inp_dpad_y() * 30;
        hero->ropelen = clamp_i(hero->ropelen, HERO_ROPE_LEN_MIN, HERO_ROPE_LEN_MAX);
        rope_set_len_max_q4(o->rope, hero->ropelen);
    }

    int velsgn = sgn_i(o->vel_q8.x);
    int velabs = abs_i(o->vel_q8.x);
    int xacc   = 0;

    if (dpad_x == 0) {
        o->drag_q8.x = (grounded ? 130 : 245);
    } else {
        if (grounded) {
            if (velsgn == -dpad_x) { // reverse direction
                xacc         = 200;
                o->drag_q8.x = 130;
            } else {
                o->drag_q8.x = 250;
                int accfrom  = pow_i32(velabs, 2);
                int accto    = pow_i32(600, 2);
                xacc         = lerp_i32(135,
                                        0, // min acc
                                        accfrom,
                                        accto);
            }
        } else {
            o->drag_q8.x = 245;
            if (velsgn == -dpad_x) {
                xacc = 75;
            } else if (velabs < 1000) {
                xacc = 35;
            }
        }
    }

    bool32 ropestretched = (o->rope) && rope_stretched(g, o->rope);

    if (dpad_x != 0 && !grounded) {
        o->drag_q8.x = 245;
        if (ropestretched) {
            v2_i32 dtrope = v2_sub(o->rope->head->p, o->rope->head->next->p);
            xacc          = (sgn_i(dtrope.x) == dpad_x ? 15 : 30);
            xacc          = 100;
        } else if (velsgn == -dpad_x) {
            xacc = 75;
        } else if (velabs < 1000) {
            xacc = 35;
        }
    }

    o->vel_q8.x += xacc * dpad_x;

    if (inp_just_pressed(INP_DPAD_U) && grounded) {
        v2_i32 posc         = obj_pos_center(o);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            obj_interact(g, interactable);
        }
    }

    if (ohook) {
        hook_update(g, ohook);
    }
}

void hero_room_transition(game_s *g, obj_s *o)
{
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
    aabb.x += g->map_world.roomcur->r.x;
    aabb.y += g->map_world.roomcur->r.y;

    map_room_s *nextroom = map_world_overlapped_room(&g->map_world, aabb);

    if (!nextroom) {
        sys_printf("no room\n");
        return;
    }

    transition_s *t  = &g->transition;
    rec_i32       nr = nextroom->r;
    trgaabb.x += g->map_world.roomcur->r.x - nr.x;
    trgaabb.y += g->map_world.roomcur->r.y - nr.y;

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

    t->hero_dir  = touchedbounds;
    t->heroaabb  = trgaabb;
    t->hero_face = o->facing;
    t->hero_v    = o->vel_q8;

    transition_start(t, nextroom->filename);
}

static void hero_set_cur_item(hero_s *h, int item)
{
    if (h->aquired_items == 0) return;
    if (!(h->aquired_items & (1 << item))) {
        sys_printf("Doesn't have item %i\n", item);
        return;
    }
    h->selected_item = item;
}

#define ITEM_CRANK_THRESHOLD 8000

void hero_crank_item_selection(hero_s *h)
{
    if (h->aquired_items == 0) return;
    if (h->itemselection_decoupled) return;
    if (inp_debug_space()) return;

    int p_q16 = inp_prev_crank_q16();
    int c_q16 = inp_crank_q16();
    int dt    = inp_crank_calc_dt_q16(p_q16, c_q16);
    int dtp   = inp_crank_calc_dt_q16(p_q16, h->item_angle);
    int dtc   = inp_crank_calc_dt_q16(h->item_angle, c_q16);

    // turn item barrel, snaps into crank angle for small angles
    int angle_old = h->item_angle;
    if ((dt > 0 && dtp >= 0 && dtc >= 0) || (dt < 0 && dtp <= 0 && dtc <= 0)) {
        h->item_angle = c_q16;
    } else if (abs_i(dtc) < ITEM_CRANK_THRESHOLD) {
        int d = sgn_i(dtc) * (ITEM_CRANK_THRESHOLD - abs_i(dtc)) / 10;
        if (sgn_i(d) == sgn_i(dtc) && abs_i(d) > abs_i(dtc)) {
            d = dtc;
        }

        h->item_angle = (h->item_angle + d + 0x10000) & 0xFFFF;
    }

    // item selection based on new item barrel angle
    // check if the barrel "flipped over" the 180 deg position
    // -> select next/prev item
    int change = inp_crank_calc_dt_q16(angle_old, h->item_angle);
    if (change > 0 && angle_old < 0x8000 && h->item_angle >= 0x8000) {
        do {
            h->selected_item += 1;
            h->selected_item %= NUM_HERO_ITEMS;
        } while (!(h->aquired_items & (1 << h->selected_item)));
    } else if (change < 0 && angle_old >= 0x8000 && h->item_angle < 0x8000) {
        do {
            h->selected_item += NUM_HERO_ITEMS - 1;
            h->selected_item %= NUM_HERO_ITEMS;
        } while (!(h->aquired_items & (1 << h->selected_item)));
    }
}