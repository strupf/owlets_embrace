// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN   500
#define HERO_ROPE_LEN_SHORT 2000
#define HERO_ROPE_LEN_LONG  4000

static void hero_set_cur_item(hero_s *h, int item);
static int  hero_max_rope_len_q4(hero_s *h);

obj_s *hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->flags |= OBJ_FLAG_MOVER;
    o->flags |= OBJ_FLAG_TILE_COLLISION;
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_CLAMP_TO_ROOM;
    o->flags |= OBJ_FLAG_SPRITE;
    o->moverflags |= OBJ_MOVER_SLOPES;
    o->moverflags |= OBJ_MOVER_GLUE_GROUND;
    o->moverflags |= OBJ_MOVER_AVOID_HEADBUMP;
    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;

    o->n_sprites    = 2;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->gravity_q8.y = 60;
    o->vel_cap_q8.x = 3000;
    o->vel_cap_q8.y = 3000;
    o->w            = 10;
    o->h            = 20;
    o->facing       = 1;
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
    case HERO_UPGRADE_AIR_JUMP_1: h->n_airjumps = 1; break;
    case HERO_UPGRADE_AIR_JUMP_2: h->n_airjumps = 2; break;
    case HERO_UPGRADE_AIR_JUMP_3: h->n_airjumps = 3; break;
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

    hero_s *hero = &g->herodata;
    rope_init(r);
    rope_set_len_max_q4(r, hero_max_rope_len_q4(hero));
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

void hero_check_rope_intact(game_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (!ohook) return;

    rope_s *r = o->rope;
    if (!rope_intact(g, r)) {
        hook_destroy(g, o, ohook);
    }
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
            int newlen_q4 = clamp_i(rope_length_q4(g, r), HERO_ROPE_LEN_MIN, hero_max_rope_len_q4(&g->herodata));
            rope_set_len_max_q4(r, newlen_q4);
            snd_play_ext(asset_snd(SNDID_HOOK_ATTACH), 1.f, 1.f);
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
        h->vel_q8 = obj_constrain_to_rope(g, h);
    } else {
        // hook->vel_q8 = rope_adjust_connected_vel(g, r, r->tail, hook->subpos_q8, hook->vel_q8);
        hook->vel_q8 = obj_constrain_to_rope(g, hook);
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
    if (o->attack != HERO_ATTACK_NONE && o->attack_tick > 5)
        return;

    obj_s *ohook_;
    if (obj_try_from_obj_handle(o->obj_handles[0], &ohook_)) {
        hook_destroy(g, o, ohook_);
        return;
    }

    switch (hero->selected_item) {
    case HERO_ITEM_HOOK: {
        hero_use_hook(g, o, hero);
    } break;
    case HERO_ITEM_BOMB: {
    } break;
    case HERO_ITEM_WHIP: {
        o->attack_tick   = 18;
        o->facing_locked = 1;
        o->subattack     = 1 - o->subattack; // alternate

        switch (inp_dpad_dir()) {
        case INP_DPAD_DIR_NONE:
        case INP_DPAD_DIR_E:
        case INP_DPAD_DIR_W:
            o->attack = HERO_ATTACK_SIDE;
            break;
        case INP_DPAD_DIR_N:
            o->attack = HERO_ATTACK_UP;
            break;
        case INP_DPAD_DIR_S:
            o->attack = HERO_ATTACK_DOWN;
            break;
        }
    } break;
    }
}

void hero_on_update(game_s *g, obj_s *o)
{
    hero_s *hero = &g->herodata;

    if (o->attack != HERO_ATTACK_NONE) {
        hitbox_s hitboxes[4] = {0};
        hitboxes[0].damage   = 1;
        v2_i32 hbp           = obj_pos_bottom_center(o);
        switch (o->attack) {
        case HERO_ATTACK_UP:
            break;
        case HERO_ATTACK_DOWN:
            break;
        case HERO_ATTACK_DIA_UP:
            break;
        case HERO_ATTACK_DIA_DOWN:
            break;
        case HERO_ATTACK_SIDE:
            hitbox_s *hbr = &hitboxes[0];
            hbr->r.x      = hbp.x + 40;
            hbr->r.y      = hbp.y - 30;
            hbr->r.w      = 60;
            hbr->r.h      = 30;
            break;
        }

        memcpy(hero->hitbox_def, hitboxes, sizeof(hitboxes));
        hero->n_hitbox = 1;

        game_apply_hitboxes(g, hitboxes, 1);

        if (o->attack_tick == 16) {
            particle_desc_s prt = {0};
            prt.p.p_q8.x        = (hbp.x + o->facing * 60) << 8;
            prt.p.p_q8.y        = (hbp.y - 30) << 8;
            prt.p.v_q8.x        = o->facing * 700;
            prt.p.v_q8.y        = -250;
            prt.p.a_q8.y        = 30;
            prt.p.size          = 1;
            prt.p.ticks_max     = 20;
            prt.ticksr          = 10;
            prt.pr_q8.x         = 2000;
            prt.pr_q8.y         = 4000;
            prt.vr_q8.x         = 300;
            prt.vr_q8.y         = 200;
            prt.ar_q8.y         = 4;
            prt.sizer           = 1;
            prt.p.gfx           = PARTICLE_GFX_CIR;
            particles_spawn(g, prt, 20);
        }

        if (--o->attack_tick <= 0) {
            o->attack        = HERO_ATTACK_NONE;
            o->facing_locked = 0;
        }
    } else {
        hero->n_hitbox = 0;
    }

    int    dpad_x   = inp_dpad_x();
    int    dpad_y   = inp_dpad_y();
    bool32 grounded = obj_grounded(g, o);
    bool32 usehook  = o->ropenode != NULL;
    // sys_printf("%i\n", o->vel_q8.y);

    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    if (0 < dpad_y)
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;

    if (dpad_x != 0 && !o->facing_locked) {
        o->facing = dpad_x;
    }

    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;

    // jump buffering
    // https://twitter.com/MaddyThorson/status/1238338575545978880
    if (inp_pressed(INP_A)) {
        if (inp_just_pressed(INP_A)) {
            o->jump_btn_buffer = 8;
        } else {
            o->jump_btn_buffer--;
        }
    } else {
        o->jump_btn_buffer = 0;
    }

    // coyote time -> edgeticks
    // https://twitter.com/MaddyThorson/status/1238338574220546049
    if (grounded) {
        o->edgeticks  = 6;
        o->n_airjumps = hero->n_airjumps;
    } else if (0 < o->edgeticks) {
        o->edgeticks--;
    }

    struct jumpvar_s {
        int v_init; // initial velocity of the jump, absolute
        int ticks;  // ticks of variable jump (decreases faster if jump button is not held)
        int vi;     // "jetpack" velocity, goes to 0 over ticks or less
    };

    static const struct jumpvar_s jump_tab[4] = {
        {800, 20, 60},
        {0, 22, 140},
        {0, 18, 135},
        {0, 15, 130},
    };

    if (0 < o->jumpticks) {
        if (inp_pressed(INP_A))
            o->jumpticks--;
        else
            o->jumpticks >>= 1; // decrease jump ticks faster

        struct jumpvar_s jv = jump_tab[0];
        if (o->n_airjumps != hero->n_airjumps) { // air jump
            jv = jump_tab[hero->n_airjumps - o->n_airjumps];
        }

        int j0 = pow2_i32(jv.ticks - o->jumpticks);
        int j1 = pow2_i32(jv.ticks);
        o->vel_q8.y -= lerp_i32(jv.vi, 0, j0, j1);
    } else {
        o->jumpticks--;
        bool32 can_jump_midair = !usehook &&          // not hooked
                                 o->edgeticks == 0 && // jump in air?
                                 0 < o->n_airjumps && // air jumps left?
                                 o->jumpticks < -10;  // wait some ticks after last jump

        if (0 < o->jump_btn_buffer && (0 < o->edgeticks || can_jump_midair)) {
            struct jumpvar_s jv = jump_tab[0];
            if (o->edgeticks == 0) // air jump
                jv = jump_tab[hero->n_airjumps - --o->n_airjumps];
            int vy = jv.v_init;
            hero->aquired_upgrades |= (1 << HERO_UPGRADE_HIGH_JUMP);
            if (!hero_has_upgrade(hero, HERO_UPGRADE_HIGH_JUMP)) {
                vy = (vy * 3) / 4;
            }

            o->jump_btn_buffer = 0;
            o->edgeticks       = 0;
            o->jumpticks       = jv.ticks;
            o->vel_q8.y        = -vy;
        }
    }

    if (inp_just_pressed(INP_B)) {
        hero_use_item(g, o, hero);
    }

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook && ohook->attached) {
        int rl_q4 = o->rope->len_max_q4;
        rl_q4 += inp_dpad_y() * 30;
        rl_q4 = clamp_i(rl_q4, HERO_ROPE_LEN_MIN, hero_max_rope_len_q4(hero));
        rope_set_len_max_q4(o->rope, rl_q4);
    }

    int velsgn = sgn_i(o->vel_q8.x);
    int velabs = abs_i(o->vel_q8.x);
    int xacc   = 0;

    if (dpad_x == 0) {
        o->drag_q8.x = (grounded ? 130 : 245);
        if (abs_i(o->vel_q8.x) < 50 && grounded) {
            o->vel_q8.x = 0;
        }
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

static void hero_set_cur_item(hero_s *h, int item)
{
    if (h->aquired_items == 0) return;
    if (!(h->aquired_items & (1 << item))) {
        sys_printf("Doesn't have item %i\n", item);
        return;
    }
    h->selected_item = item;
}

static int hero_max_rope_len_q4(hero_s *h)
{
    if (hero_has_upgrade(h, HERO_UPGRADE_LONG_HOOK))
        return HERO_ROPE_LEN_LONG;
    return HERO_ROPE_LEN_SHORT;
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

void hero_on_squish(game_s *g, obj_s *o)
{
}

void hero_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *sprite = &o->sprites[0];
    sprite->trec.t          = asset_tex(TEXID_HERO);
    hero_s *hero            = &g->herodata;
    int     flip            = o->facing == -1 ? SPR_FLIP_X : 0;

    int    animID   = 0;
    int    frameID  = 0;
    bool32 grounded = obj_grounded(g, o);
    if (grounded) {
        if (o->vel_q8.x != 0) {
            animID           = 0; // walking
            int frameID_prev = (o->animation / 4000) % 4;
            o->animation += abs_i(o->vel_q8.x);
            frameID = (o->animation / 4000) % 4;
            if (frameID_prev != frameID && (frameID % 2) == 0) {
                snd_play_ext(asset_snd(SNDID_STEP), 0.5f, 1.f);
            }
        } else {
            animID = 1; // idle
            if (o->vel_prev_q8.x != 0) {
                o->animation = 0; // just got idle
            } else {
                o->animation += 100;
                frameID = (o->animation / 4000) % 2;
            }
        }
    } else { // midair
        if (o->vel_q8.y > 0 || o->ropenode) {
            animID = 3; // falling
        } else {
            animID = 4; // gaining height
        }
    }
    rec_i32 frame  = {frameID * 64, animID * 64, 64, 64};
    sprite->trec.r = frame;

    sprite->flip   = flip;
    sprite->offs.x = o->w / 2 - 32;
    sprite->offs.y = o->h - 64;

    o->n_sprites = 1;
    if (o->attack != HERO_ATTACK_NONE) {
        o->n_sprites             = 2;
        sprite_simple_s *spritew = &o->sprites[1];

        int times[] = {3,
                       7,
                       10,
                       15,
                       18};

        int fr = 0;
        while (o->attack_tick <= times[4 - fr])
            fr++;

        fr = clamp_i(fr, 0, 4);

        spritew->trec.t   = asset_tex(TEXID_HERO_WHIP);
        spritew->trec.r.x = fr * 256;
        spritew->trec.r.y = o->subattack * 128;
        spritew->trec.r.w = 256;
        spritew->trec.r.h = 128;
        spritew->flip     = flip;
        spritew->offs.x   = o->w / 2 - 128 + o->facing * 32;
        spritew->offs.y   = o->h / 2 - 128 + 24;
    }
}