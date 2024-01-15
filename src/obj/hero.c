// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN   500
#define HERO_ROPE_LEN_SHORT 2000
#define HERO_ROPE_LEN_LONG  4000

#define HEROHOOK_N_HIST 4
typedef struct {
    int    n_ang;
    v2_f32 anghist[HEROHOOK_N_HIST];
} herohook_s;

static void herodata_set_cur_item(herodata_s *h, int item);
static int  hero_max_rope_len_q4(herodata_s *h);

obj_s *hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_TILE_COLLISION |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_CLAMP_TO_ROOM |
               OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_SLOPES |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_ONE_WAY_PLAT;

    o->health_max   = 3;
    o->health       = o->health_max;
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

bool32 hero_has_upgrade(herodata_s *h, int upgrade)
{
    return (h->aquired_upgrades & (1U << upgrade));
}

void hero_aquire_upgrade(herodata_s *h, int upgrade)
{
    if (hero_has_upgrade(h, upgrade)) {
        sys_printf("Already has upgrade: %i\n", upgrade);
        return;
    }
    sys_printf("Collected upgrade %i\n", upgrade);
    h->aquired_upgrades |= 1 << upgrade;

    switch (upgrade) {
    case HERO_UPGRADE_HOOK:
        h->aquired_items |= 1 << HERO_ITEM_HOOK;
        herodata_set_cur_item(h, HERO_ITEM_HOOK);
        break;
    case HERO_UPGRADE_WHIP:
        h->aquired_items |= 1 << HERO_ITEM_WHIP;
        herodata_set_cur_item(h, HERO_ITEM_WHIP);
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
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_HOOK, 0, 0, 32, 32);
    spr->offs.x          = -16 + 4;
    spr->offs.y          = -16 + 4;

    o->w            = 8;
    o->h            = 8;
    o->pos.x        = p.x - o->w / 2;
    o->pos.y        = p.y - o->h / 2;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->gravity_q8.y = 70;
    o->vel_q8       = v_q8;
    o->vel_cap_q8.x = 2500;
    o->vel_cap_q8.y = 2500;

    herodata_s *hero = &g->herodata;
    rope_init(r);
    rope_set_len_max_q4(r, hero_max_rope_len_q4(hero));
    r->tail->p  = p;
    r->head->p  = p;
    o->rope     = r;
    o->ropenode = o->rope->tail;

    herohook_s *h = (herohook_s *)o->mem;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        h->anghist[i] = (v2_f32){(f32)v_q8.x, (f32)v_q8.y};
    }
    return o;
}

void hook_on_animate(game_s *g, obj_s *o)
{
    if (o->attached) return;

    herohook_s *h    = (herohook_s *)o->mem;
    ropenode_s *rn   = o->ropenode->prev ? o->ropenode->prev : o->ropenode->next;
    v2_i32      rndt = v2_sub(o->ropenode->p, rn->p);

    v2_f32 v               = {(f32)rndt.x, (f32)rndt.y};
    h->anghist[h->n_ang++] = v;
    h->n_ang %= HEROHOOK_N_HIST;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        v = v2f_add(v, h->anghist[i]);
    }

    f32 ang  = (atan2f(v.y, v.x) * 16.f) / PI2_FLOAT;
    int imgy = (int)(ang + 16.f + 4.f) & 15;

    o->sprites[0].trec.r.y = imgy * 32;
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
            snd_play_ext(SNDID_HOOK_ATTACH, 1.f, 1.f);
            hook->attached   = 1;
            hook->gravity_q8 = (v2_i32){0};
            hook->vel_q8     = (v2_i32){0};

            for (obj_each(g, solid)) {
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

void hero_use_hook(game_s *g, obj_s *h, herodata_s *hero)
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

static void hero_use_item(game_s *g, obj_s *o, herodata_s *hero)
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
    case HERO_ITEM_WHIP: {

        o->attack_tick = 15;
        o->subattack   = 1 - o->subattack; // alternate

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
        snd_play_ext(SNDID_SWOOSH, 0.7f, rngr_f32(1.2f, 1.5f));
    } break;
    }
}

void hero_on_update(game_s *g, obj_s *o)
{
    herodata_s *herodata = &g->herodata;
    herodata->n_hitbox   = 0;
    hero_s *hero         = (hero_s *)o->mem;

    if (o->attack != HERO_ATTACK_NONE) {
        if (o->attack_tick == 12) {
            hitbox_s hitboxes[4] = {0};
            v2_i32   hbp         = obj_pos_bottom_center(o);
            switch (o->attack) {
            case HERO_ATTACK_UP:
                break;
            case HERO_ATTACK_DOWN:
                break;
            case HERO_ATTACK_DIA_UP:
                break;
            case HERO_ATTACK_DIA_DOWN:
                break;
            case HERO_ATTACK_SIDE: {
                hitbox_s *hbr = &hitboxes[0];
                hbr->flags |= HITBOX_FLAG_HERO;
                hbr->damage     = 1;
                hbr->r.w        = 60;
                hbr->r.h        = 40;
                hbr->r.x        = hbp.x + o->facing * 40 - hbr->r.w / 2;
                hbr->r.y        = hbp.y - 30;
                hbr->force_q8.x = o->facing << 8;
                hbr->force_q8.y = -256;

            } break;
            }

#ifdef SYS_DEBUG
            memcpy(herodata->hitbox_def, hitboxes, sizeof(hitboxes));
            herodata->n_hitbox = 1;
#endif

            game_apply_hitboxes(g, hitboxes, 1);
        }

        if (--o->attack_tick <= 0) {
            o->attack = HERO_ATTACK_NONE;
        }
    }

    int    dpad_x   = inp_dpad_x();
    int    dpad_y   = inp_dpad_y();
    bool32 grounded = obj_grounded(g, o);
    bool32 usehook  = o->ropenode != NULL;

    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    o->facing_locked = 0;
    o->drag_q8.y     = 256;

    if (0 < o->invincible_tick)
        o->invincible_tick--;

    if (0 < dpad_y)
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;

    if (o->attack != HERO_ATTACK_NONE) {
        o->facing_locked = 1;
    }

    if (dpad_x != 0 && !o->facing_locked) {
        o->facing = dpad_x;
    }

    if (o->bumpflags & OBJ_BUMPED_Y) {

        if (1000.f <= o->vel_q8.y) {
            f32 vol = (.5f * (f32)o->vel_q8.y) / 2000.f;
            snd_play_ext(SNDID_STEP, min_f(vol, 0.5f), 1.f);
        }

        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;

    int    water_depth = water_depth_rec(g, obj_aabb(o));
    bool32 swimming    = 0 < water_depth && !grounded;

    if (water_depth) { // upwards swimming force
        o->drag_q8.y = 240;
        o->vel_q8.y -= 25;
        hero->swimticks--; // swim ticks are reset when grounded later on
        if (0 < hero->swimticks) {
            int j0 = min_i(water_depth, 60);
            int j1 = 60;
            o->vel_q8.y -= (j0 * 80) / j1;
        } else {
            sys_printf("no swim ticks left\n");
            o->vel_q8.y -= water_depth;
        }
    }

    // jump buffering
    // https://twitter.com/MaddyThorson/status/1238338575545978880
    if (0 < hero->jump_btn_buffer)
        hero->jump_btn_buffer--;
    if (inp_just_pressed(INP_A))
        hero->jump_btn_buffer = 8;

    // coyote time -> edgeticks
    // https://twitter.com/MaddyThorson/status/1238338574220546049
    if (grounded) {
        bool32 can_swim     = hero_has_upgrade(herodata, HERO_UPGRADE_SWIM);
        hero->edgeticks     = 6;
        hero->swimticks     = can_swim ? 2000 : 10000;
        hero->airjumps_left = herodata->n_airjumps;
    } else if (0 < hero->edgeticks) {
        hero->edgeticks--;
    }

    struct jumpvar_s {
        int vy;
        int ticks; // ticks of variable jump (decreases faster if jump button is not held)
        int vi;    // "jetpack" velocity, goes to 0 over ticks or less
    };

    static const struct jumpvar_s jump_tab[5] = {
        {700, 20, 90}, // lo ground
        {900, 25, 90}, // hi ground
        {0, 20, 250},  // airj 1
        {0, 18, 135},  // airj 2
        {0, 15, 130},  // airj 3
    };

#define EDIT_JUMP 0

#if EDIT_JUMP
    static int vy1     = 700;
    static int vticks1 = 20;
    static int vi1     = 90;

    int vvv1 = vy1;
    int vvv2 = vticks1;
    int vvv3 = vi1;

    if (sys_key(SYS_KEY_I)) vy1 += 5;
    if (sys_key(SYS_KEY_K)) vy1 -= 5;
    if (sys_key(SYS_KEY_O)) vi1++;
    if (sys_key(SYS_KEY_L)) vi1--;
    if (sys_key(SYS_KEY_U)) vticks1++;
    if (sys_key(SYS_KEY_J)) vticks1--;

    if (vvv1 != vy1 || vvv2 != vticks1 || vvv3 != vi1) {
        sys_printf("init: %i\n", vy1);
        sys_printf("vtic: %i\n", vi1);
        sys_printf("tick: %i\n\n", vticks1);
    }
#endif

    hero->jumpticks--;
    if (0 < hero->jumpticks) {
        if (inp_pressed(INP_A)) {
#if EDIT_JUMP
            int vticks = max_i(vticks1, 1);
            int vi     = vi1;
#else
            struct jumpvar_s jv     = jump_tab[hero->jump_index];
            int              vticks = jv.ticks;
            int              vi     = jv.vi;
#endif

            int j0 = vticks - hero->jumpticks;
            int j1 = vticks;
            o->vel_q8.y -= vi - (vi * j0) / j1;
        } else {
            hero->jumpticks = 0;
        }
    } else {
        bool32 jump_ground = 0 < hero->jump_btn_buffer && 0 < hero->edgeticks;
        bool32 jump_midair = !usehook &&                // not hooked
                             !swimming &&               // not swimming
                             hero->edgeticks == 0 &&    // jump in air?
                             0 < hero->airjumps_left && // air jumps left?
                             hero->jumpticks < -15 &&   // wait some ticks after last jump
                             inp_pressed(INP_A);

        if ((jump_ground || jump_midair)) {
            int jumpindex = 0;
            if (jump_ground) {
                o->vel_q8.y  = 0;
                bool32 hjump = hero_has_upgrade(herodata, HERO_UPGRADE_HIGH_JUMP);
                jumpindex    = hjump ? 1 : 0;
            } else {
                o->vel_q8.y >>= 1;
                jumpindex = 2 + herodata->n_airjumps - hero->airjumps_left;
                hero->airjumps_left--;
            }

#if EDIT_JUMP
            o->vel_q8.y -= vy1;
            int vticks = max_i(vticks1, 1);
#else
            struct jumpvar_s jv     = jump_tab[jumpindex];
            hero->jump_index        = jumpindex;
            o->vel_q8.y -= jv.vy;
            int vticks = jv.ticks;
#endif
            hero->jumpticks       = vticks;
            hero->jump_btn_buffer = 0;
            hero->edgeticks       = 0;
        }
    }

    if (inp_just_pressed(INP_B) && !swimming) {
        hero_use_item(g, o, herodata);
    }

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook && ohook->attached) {
        int rl_q4 = o->rope->len_max_q4;
        rl_q4 += inp_dpad_y() * 30;
        rl_q4 = clamp_i(rl_q4, HERO_ROPE_LEN_MIN, hero_max_rope_len_q4(herodata));
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

static void herodata_set_cur_item(herodata_s *h, int item)
{
    if (h->aquired_items == 0) return;
    if (!(h->aquired_items & (1 << item))) {
        sys_printf("Doesn't have item %i\n", item);
        return;
    }
    h->selected_item = item;
}

static int hero_max_rope_len_q4(herodata_s *h)
{
    if (hero_has_upgrade(h, HERO_UPGRADE_LONG_HOOK))
        return HERO_ROPE_LEN_LONG;
    return HERO_ROPE_LEN_SHORT;
}

#define ITEM_CRANK_THRESHOLD 8000

void hero_crank_item_selection(herodata_s *h)
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

    sprite->trec.t       = asset_tex(TEXID_HERO);
    herodata_s *hero     = &g->herodata;
    int         flip     = o->facing == -1 ? SPR_FLIP_X : 0;
    int         animID   = 0;
    int         frameID  = 0;
    bool32      grounded = obj_grounded(g, o);

    if (grounded) {
        if (o->vel_q8.x != 0) {
            animID           = 0; // walking
            int frameID_prev = (o->animation / 4000) % 4;
            o->animation += abs_i(o->vel_q8.x);
            frameID = (o->animation / 4000) % 4;
            if (frameID_prev != frameID && (frameID % 2) == 0) {
                snd_play_ext(SNDID_STEP, 0.5f, 1.f);
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
    sprite->mode   = 0;
    if (0 < o->invincible_tick && tick_to_index_freq(g->tick, 2, 8)) {
        sprite->mode = SPR_MODE_INV;
    }

    o->n_sprites = 1;
    if (o->attack != HERO_ATTACK_NONE) {
        o->n_sprites             = 2;
        sprite_simple_s *spritew = &o->sprites[1];

        static const int times[] = {3,
                                    5,
                                    9,
                                    12,
                                    15};

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
        spritew->offs.y   = o->h / 2 - 128 + 28;

        if (o->attack_tick == 10) {
            particle_desc_s prt = {0};
            prt.p.p_q8.x        = (o->pos.x + o->w / 2 + o->facing * 60) << 8;
            prt.p.p_q8.y        = (o->pos.y - 20) << 8;
            prt.p.v_q8.x        = o->facing * 700;
            prt.p.v_q8.y        = -250;
            prt.p.a_q8.y        = 30;
            prt.p.size          = 1;
            prt.p.ticks_max     = 60;
            prt.ticksr          = 10;
            prt.pr_q8.x         = 2000;
            prt.pr_q8.y         = 4000;
            prt.vr_q8.x         = 300;
            prt.vr_q8.y         = 200;
            prt.ar_q8.y         = 4;
            prt.sizer           = 1;
            prt.p.gfx           = PARTICLE_GFX_CIR;
            particles_spawn(g, &g->particles, prt, 20);
        }
    }
}

void hero_hurt(game_s *g, obj_s *o, herodata_s *h, int damage)
{
    if (0 < o->invincible_tick) return;

    int health = obj_health_change(o, -damage);
    if (0 < health) {
        o->invincible_tick = ticks_from_ms(1000);
    } else {
        // kill
    }
}