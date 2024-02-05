// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN        500
#define HERO_ROPE_LEN_SHORT      2000
#define HERO_ROPE_LEN_LONG       4000
#define HERO_GRAVITY             60
#define HERO_DRAG_Y              256
#define HERO_WHIP_TICKS          14
#define HERO_GLIDE_VY            200
#define HERO_SPRINT_TICKS        60 // ticks walking until sprinting
#define HERO_VX_WALK             576
#define HERO_VX_SPRINT           900
#define HERO_VY_BOOST_SPRINT_ABS 150 // absolute vy added to a jump sprinted
#define HERO_VX_BOOST_SPRINT_Q8  300 // vx multiplier when jumping sprinted
#define HERO_VX_BOOST_SLIDE_Q8   370 // vx multiplier when jumping slided
#define HERO_DRAG_SLIDING        252

#define HEROHOOK_N_HIST 4
typedef struct {
    int    n_ang;
    v2_f32 anghist[HEROHOOK_N_HIST];
} herohook_s;

typedef struct {
    int vy;
    int ticks; // ticks of variable jump (decreases faster if jump button is not held)
    int v0;    // "jetpack" velocity, goes to v1 over ticks or less
    int v1;
} hero_jumpvar_s;

static const hero_jumpvar_s g_herovar[5] = {
    {725, 25, 60, 30},  // lo ground
    {875, 25, 60, 30},  // hi ground
    {200, 20, 150, 30}, // airj 1
    {150, 25, 180, 0},  // airj 2
    {100, 25, 130, 0},  // airj 3
};

static void   herodata_set_cur_item(herodata_s *h, int item);
static int    hero_max_rope_len_q4(herodata_s *h);
static void   hero_start_jump(game_s *g, obj_s *o, hero_s *h, int ID);
static void   hero_item_use(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_ladder(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_swimming(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_ground(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_air(game_s *g, obj_s *o, hero_s *h);
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout);
static int    hero_can_walljump(game_s *g, obj_s *o);
static int    hero_can_walljump(game_s *g, obj_s *o)
{
#define WALLJUMP_MAX_DST 5
    int s = hero_determine_state(g, o, (hero_s *)o->mem);
    if (s != HERO_STATE_AIR) return 0;

    int dst_l = 0xFF;
    int dst_r = 0xFF;
    for (int x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rl = {o->pos.x - 1 - x, o->pos.y, 1, o->h};
        if (!game_traversable(g, rl)) {
            dst_l = x;
            break;
        }
    }
    for (int x = 0; x < WALLJUMP_MAX_DST; x++) {
        rec_i32 rr = {o->pos.x + o->w + x, o->pos.y, 1, o->h};
        if (!game_traversable(g, rr)) {
            dst_r = x;
            break;
        }
    }
    if (dst_l != 0xFF && dst_l < dst_r) return +1;
    if (dst_r != 0xFF && dst_r < dst_l) return -1;
    return (dst_l == dst_r ? +1 : 0);
}

//
static obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8);
void          hook_update(game_s *g, obj_s *h, obj_s *hook);

obj_s *hero_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->render_priority = 1000;

    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_TILE_COLLISION |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_CLAMP_TO_ROOM |
               OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_SLOPES |
                    OBJ_MOVER_SLOPES_HI |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_ONE_WAY_PLAT;

    o->health_max      = g->herodata.health;
    o->health          = o->health_max;
    o->render_priority = RENDER_PRIO_HERO;
    o->n_sprites       = 2;
    o->drag_q8.x       = 256;
    o->drag_q8.y       = HERO_DRAG_Y;
    o->gravity_q8.y    = HERO_GRAVITY;
    o->vel_cap_q8.x    = 3000;
    o->vel_cap_q8.y    = 3000;
    o->w               = 10;
    o->h               = 20;
    o->facing          = 1;
    return o;
}

bool32 hero_has_upgrade(herodata_s *h, int upgrade)
{
    return (h->aquired_upgrades & (1U << upgrade));
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

static void herodata_set_cur_item(herodata_s *h, int item)
{
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

#if 0
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
#endif
}

void hero_on_squish(game_s *g, obj_s *o)
{
}

void hero_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *sprite = &o->sprites[0];
    o->n_sprites            = 1;
    sprite->trec.t          = asset_tex(TEXID_HERO);
    sprite->offs.x          = 0;
    sprite->offs.y          = 0;
    sprite->flip            = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x          = o->w / 2 - 48;
    sprite->offs.y          = o->h - 96 + 16;
    herodata_s *hero        = &g->herodata;
    hero_s     *h           = (hero_s *)o->mem;
    int         animID      = 0;
    int         frameID     = 0;
    int         state       = hero_determine_state(g, o, h);

    if (h->sliding) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        animID  = 3;
        frameID = 0;
    }

    switch (state) {
    case HERO_STATE_GROUND: {
        if (0 < h->landed_ticks) {
            animID  = 2;
            frameID = 4;
            break;
        }

        sprite->offs.y -= 16;

        if (o->vel_q8.x != 0) {
            animID           = 0; // walking
            int frameID_prev = (o->animation / 4000) % 4;
            o->animation += abs_i(o->vel_q8.x);
            frameID = (o->animation / 4000) % 4;
            if (frameID_prev != frameID && (frameID % 2) == 0) {
                // snd_play_ext(SNDID_STEP, 0.5f, 1.f);
            }
            break;
        }
        animID = 1; // idle
        if (o->vel_prev_q8.x != 0) {
            o->animation = 0; // just got idle
        } else {
            o->animation += 100;
            frameID = (o->animation / 4000) % 2;
        }
        break;
    }

    case HERO_STATE_LADDER: {
        animID       = 4;
        frameID      = 0;
        sprite->flip = ((o->animation >> 3) & 1) ? SPR_FLIP_X : 0;
        break;
    }
    case HERO_STATE_SWIMMING: { // repurpose jumping animation for swimming
        animID   = 2;
        int swim = ((o->animation >> 4) & 7);
        if (swim <= 1) {
            sprite->offs.y += swim == 0 ? 0 : -4;
            frameID = 1;
        } else {
            sprite->offs.y += swim - 2;
            frameID = 2 + (swim & 1);
        }
        break;
    }
    case HERO_STATE_AIR: {
        if (h->walljumpticks) {
            animID       = 7;
            sprite->flip = 0 < o->vel_q8.x ? 0 : SPR_FLIP_X;
            break;
        }
        animID = 2;
        sprite->offs.y += 4;
        if (0 < h->jumped_ticks) {
            frameID = 0;
        } else if (h->gliding) {
            frameID = 2 + (8 <= (o->animation & 63));
        } else if (+500 <= o->vel_q8.y) {
            frameID = 3;
        } else if (-100 <= o->vel_q8.y) {
            frameID = 2;
        } else {
            frameID = 1;
        }
        break;
    }
    }

    rec_i32 frame  = {frameID * 96, animID * 96, 96, 96};
    sprite->trec.r = frame;

    if (h->whip_ticks) {
        o->n_sprites             = 2;
        sprite_simple_s *spritew = &o->sprites[1];
        spritew->trec            = asset_texrec(TEXID_HERO, 0, 768 + h->whip_count * 96, 128, 96);
        int whipframe            = (4 * h->whip_ticks) / HERO_WHIP_TICKS;
        spritew->trec.r.x        = whipframe * 128;
        spritew->flip            = sprite->flip;
        spritew->offs.x          = -64 + o->w / 2 + o->facing * 12;
        spritew->offs.y          = -60;
    }
}

void hero_hurt(game_s *g, obj_s *o, herodata_s *h, int damage)
{
    if (0 < o->invincible_tick) return;

    int health = obj_health_change(o, -damage);
    if (0 < health) {
        o->invincible_tick = ticks_from_ms(1000);
    } else {
        g->die_ticks = 1;
        obj_delete(g, o);
        // kill
    }
}

int hero_determine_state(game_s *g, obj_s *o, hero_s *h)
{
    int water_depth = water_depth_rec(g, obj_aabb(o));
    if (5 < water_depth) return HERO_STATE_SWIMMING;

    bool32 grounded = obj_grounded(g, o) && 0 <= o->vel_q8.y;
    if (grounded) return HERO_STATE_GROUND;

    if (h->onladder) {
        rec_i32 r_ladder;
        if (hero_rec_ladder(g, o, &r_ladder) && h->ladderx == r_ladder.x) {
            return HERO_STATE_LADDER;
        } else {
            h->onladder = 0;
        }
    }

    return HERO_STATE_AIR;
}

void hero_on_update(game_s *g, obj_s *o)
{
    herodata_s *herodata = &g->herodata;
    hero_s     *hero     = (hero_s *)o->mem;

    int state = hero_determine_state(g, o, hero);
    switch (state) {
    case HERO_STATE_GROUND: hero->last_time_ground = time_now(); break;
    case HERO_STATE_AIR: hero->last_time_air = time_now(); break;
    case HERO_STATE_SWIMMING: hero->last_time_swim = time_now(); break;
    case HERO_STATE_LADDER: hero->last_time_ladder = time_now(); break;
    }

    int    dpad_x  = inp_dpad_x();
    int    dpad_y  = inp_dpad_y();
    bool32 sliding = (0 < dpad_y &&
                      hero->sliding &&
                      (state == HERO_STATE_AIR || state == HERO_STATE_GROUND));

    if (hero->walljumpticks)
        hero->walljumpticks--;
    o->drag_q8.x = 250;
    if (sliding) {
        o->drag_q8.x = HERO_DRAG_SLIDING;
    } else {
        hero->sliding = 0;
    }

    o->drag_q8.y = HERO_DRAG_Y;
    o->flags |= OBJ_FLAG_MOVER;
    o->gravity_q8.y = HERO_GRAVITY;
    hero->gliding   = 0;
    hero->jump_btn_buffer--;
    hero->jumped_ticks--;
    hero->landed_ticks--;

    if (state != HERO_STATE_AIR && state != HERO_STATE_GROUND) {
        hero->whip_ticks   = 0;
        hero->sprint_ticks = 0;
        hero->edgeticks    = 0;
    }

    o->facing_locked = 0;
    if (hero->whip_ticks) {
        o->facing_locked = 1;
    }

    if (hero->whip_ticks) {
        hero->whip_ticks++;
        if (HERO_WHIP_TICKS <= hero->whip_ticks) {
            hero->whip_ticks = 0;
        }
    }

    if (!o->facing_locked) {
        if (hero->sliding) {
            o->facing = o->vel_q8.x ? sgn_i(o->vel_q8.x) : o->facing;
        } else if (dpad_x) {
            o->facing = dpad_x;
        }
    }
    if (!dpad_x) {
        hero->sprinting = 0;
    }

    if (o->bumpflags & OBJ_BUMPED_Y) {
        if (1000 <= o->vel_q8.y) {
            f32 vol = (.7f * (f32)o->vel_q8.y) / 2000.f;
            snd_play_ext(SNDID_STEP, min_f(vol, 0.7f), 1.f);
        }
        if (500 <= o->vel_q8.y) {
            hero->landed_ticks = 4;
        }

        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        hero->sprint_ticks = 0;
        if (hero->sliding) {
            o->vel_q8.x = -(o->vel_q8.x >> 1);
        } else {
            o->vel_q8.x = 0;
        }
    }
    o->bumpflags = 0;

    if (state == HERO_STATE_AIR) {
        hero->inair_ticks++;
    } else {
        hero->inair_ticks = 0;
    }

    if (state == HERO_STATE_GROUND) {
        hero->ground_ticks++;
    } else {
        hero->ground_ticks = 0;
    }

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_AIR: {
        if (!inp_just_pressed(INP_DPAD_U)) break;
        rec_i32 rladder;
        if (!hero_rec_ladder(g, o, &rladder)) break;
        o->pos.x       = rladder.x;
        o->pos.y       = rladder.y;
        o->vel_q8.x    = 0;
        o->vel_q8.y    = 0;
        o->animation   = 0;
        hero->onladder = 1;
        hero->ladderx  = rladder.x;
        state          = HERO_STATE_LADDER;
        break;
    }
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        hero->sprinting = 0;
        break;
    }

    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    if (state == HERO_STATE_GROUND && 0 < dpad_y && inp_pressed(INP_A)) {
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
    }

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_LADDER: {
        bool32 can_swim     = hero_has_upgrade(&g->herodata, HERO_UPGRADE_SWIM);
        hero->swimticks     = can_swim ? 2000 : 10000;
        hero->airjumps_left = g->herodata.n_airjumps;
        break;
    }
    }

    switch (state) {
    case HERO_STATE_GROUND:
        o->drag_q8.x = 250;
        hero_update_ground(g, o, hero);
        break;
    case HERO_STATE_AIR:
        o->drag_q8.x = 255;
        hero_update_air(g, o, hero);
        break;
    case HERO_STATE_SWIMMING:
        o->drag_q8.x = 240;
        hero_update_swimming(g, o, hero);
        break;
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        hero_update_ladder(g, o, hero);
        break;
    }

    hero->state_prev = state;
    obj_s *ohook     = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook) {
        hook_update(g, o, ohook);
    }
}

// tries to calculate a snapped ladder position
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout)
{
    rec_i32 aabb = obj_aabb(o);
    int     tx1  = (aabb.x >> 4);
    int     tx2  = ((aabb.x + aabb.w - 1) >> 4);
    int     ty   = ((aabb.y + (aabb.h >> 1)) >> 4);

    for (int tx = tx1; tx <= tx2; tx++) {
        if (g->tiles[tx + ty * g->tiles_x].collision != TILE_LADDER) continue;

        int     lc_x = (tx << 4) + 8;
        rec_i32 r    = {lc_x - (o->w >> 1), o->pos.y, o->w, o->h}; // aabb if on ladder
        if (!game_traversable(g, r)) continue;                     // if ladder position is valid
        if (rout) {
            *rout = r;
        }
        return 1;
    }
    return 0;
}

static void hero_start_jump(game_s *g, obj_s *o, hero_s *h, int ID)
{
    int i = ID;
    if (ID == 0 && hero_has_upgrade(&g->herodata, HERO_UPGRADE_HIGH_JUMP)) {
        i = 1;
    }
    snd_play_ext(SNDID_SPEAK, 0.5f, 0.5f);
    hero_jumpvar_s jv  = g_herovar[i];
    h->jump_index      = i;
    h->edgeticks       = 0;
    h->jump_btn_buffer = 0;
    o->vel_q8.y        = -jv.vy;
    h->jumpticks       = jv.ticks;
}

static void hero_use_weapon(game_s *g, obj_s *o, hero_s *h)
{
    if (0 < h->whip_ticks && h->whip_ticks < 10) return;
    snd_play_ext(SNDID_BASIC_ATTACK, 1.f, rngr_f32(0.9f, 1.1f));
    h->whip_ticks = 1;
    h->whip_count = 1 - h->whip_count;
    rec_i32 r     = {0};
    if (o->facing == +1) {
        r = obj_rec_right(o);
        r.w += 60;
    }
    if (o->facing == -1) {
        r = obj_rec_left(o);
        r.x -= 60;
        r.w += 60;
    }

    hitbox_s hb   = {0};
    hb.damage     = 1;
    hb.r          = r;
    hb.force_q8.x = o->facing * 500;
    hb.force_q8.y = -400;
    obj_game_player_attackbox(g, hb);
}

static void hero_use_hook(game_s *g, obj_s *o, hero_s *h)
{
    // throw new hook
    int dpad_x = inp_dpad_x();
    int dpad_y = inp_dpad_y();

    int dirx = dpad_x;
    int diry = dpad_y;

    if (dirx == 0 && diry == 0) diry = -1;

    v2_i32 center  = obj_pos_center(o);
    v2_i32 vlaunch = {dirx, diry};

    vlaunch = v2_shl(vlaunch, 11);
    if (vlaunch.y < 0) {
        vlaunch.y = vlaunch.y * 5 / 4;
    }
    if (vlaunch.y == 0) {
        vlaunch.y = -500;
        vlaunch.x = vlaunch.x * 5 / 4;
    }

    g->herodata.rope_active = 1;
    rope_s *rope            = &g->herodata.rope;
    obj_s  *hook            = hook_create(g, rope, center, vlaunch);
    o->obj_handles[0]       = obj_handle_from_obj(hook);
    hook->obj_handles[0]    = obj_handle_from_obj(o);
    o->rope                 = rope;
    o->ropenode             = rope->head;
}

static void hero_item_use(game_s *g, obj_s *o, hero_s *h)
{
#if 1
    hero_use_weapon(g, o, h);
    h->jumpticks     = 0;
    h->airjumps_left = 0;
    h->sprint_ticks  = 0;
#else
    if (obj_handle_valid(o->obj_handles[0])) {
        hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
    } else {
        hero_use_hook(g, o, h);
    }
#endif
}

static bool32 hero_is_sprinting(hero_s *h)
{
    return (HERO_SPRINT_TICKS <= abs_i(h->sprint_ticks));
}

static void hero_update_ladder(game_s *g, obj_s *o, hero_s *h)
{
    if (o->pos.x != h->ladderx) {
        h->onladder = 0;
        return;
    }

    if (inp_just_pressed(INP_B)) {
        h->onladder = 0;
        return;
    }

    if (inp_just_pressed(INP_A)) {
        h->onladder = 0;
        hero_start_jump(g, o, h, 0);
        int dpad_x  = inp_dpad_x();
        o->vel_q8.x = dpad_x * 200;
        return;
    }

    int dpad_y  = inp_dpad_y();
    o->tomove.y = dpad_y << 1;
    if (dpad_y) {
        o->animation++;
    }
}

static void hero_update_swimming(game_s *g, obj_s *o, hero_s *h)
{
    int water_depth = water_depth_rec(g, obj_aabb(o));
    o->drag_q8.y    = 230;
    h->swimticks--; // swim ticks are reset when grounded later on
    h->airjumps_left = 0;
    if (0 < h->swimticks || 1) {
        int i0 = pow_i32(min_i(water_depth, 70), 2);
        int i1 = pow_i32(70, 2);
        int k0 = min_i(water_depth, 30);
        int k1 = 30;

        o->vel_q8.y -= lerp_i32(15, 55, k0, k1) + lerp_i32(0, 90, i0, i1);
        o->animation++;
    } else {
        o->vel_q8.y -= 25 + water_depth;
    }

    // o->tomove.y -= clamp_i((water_depth - 10) >> 5, 0, 2);

    int dpad_x = inp_dpad_x();

    if (dpad_x != sgn_i(o->vel_q8.x)) {
        o->vel_q8.x /= 2;
    }
    if (dpad_x) {
        int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        int ax = (max_i(512 - i0, 0) * 32) >> 8;
        o->vel_q8.x += ax * dpad_x;
    }

    if (5 < water_depth && water_depth < 30 && inp_just_pressed(INP_A)) {
        o->tomove.y  = -water_depth / 2;
        o->drag_q8.y = 256;
        hero_start_jump(g, o, h, 0);
    }
}

static void hero_update_ground(game_s *g, obj_s *o, hero_s *h)
{
    bool32 can_swim = hero_has_upgrade(&g->herodata, HERO_UPGRADE_SWIM);
    h->edgeticks    = 6;

    v2_i32 posc              = obj_pos_center(o);
    obj_s *interactable      = obj_closest_interactable(g, posc);
    g->herodata.interactable = obj_handle_from_obj(interactable);
    if (inp_just_pressed(INP_DPAD_U) && interactable) {
        obj_game_interact(g, interactable);
    }

    if (inp_just_pressed(INP_B)) {
        hero_item_use(g, o, h);
    }

    if (inp_pressed(INP_DPAD_D)) { // sliding
        h->sprint_ticks = 0;
        int accx        = 0;
        if (0) {
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+1, 0})) {
            accx = +80;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-1, 0})) {
            accx = -80;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+2, 0})) {
            accx = +10;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-2, 0})) {
            accx = -10;
        }

        if (accx) {
            h->sliding   = 1;
            o->drag_q8.x = HERO_DRAG_SLIDING;
        }

        if (h->sliding) {
            o->vel_q8.x += accx;
        }

        if (h->sliding && rngr_i32(0, 1)) {
            particle_desc_s prt = {0};
            v2_i32          bc  = obj_pos_bottom_center(o);
            bc.y -= 2;
            prt.p.p_q8      = v2_shl(bc, 8);
            prt.p.v_q8.x    = 0;
            prt.p.v_q8.y    = -rngr_i32(100, 200);
            prt.p.a_q8.y    = 20;
            prt.p.size      = 2;
            prt.p.ticks_max = 70;
            prt.ticksr      = 20;
            prt.pr_q8.x     = 2000;
            prt.pr_q8.y     = 2000;
            prt.vr_q8.x     = 100;
            prt.vr_q8.y     = 100;
            prt.ar_q8.y     = 4;
            prt.sizer       = 1;
            prt.p.gfx       = PARTICLE_GFX_CIR;
            particles_spawn(g, &g->particles, prt, 1);
        }
    }

    if (inp_just_pressed(INP_A) || 0 < h->jump_btn_buffer) {
        hero_start_jump(g, o, h, 0);
        if (h->sliding) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SLIDE_Q8) >> 8;
        }
        if (hero_is_sprinting(h)) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SPRINT_Q8) >> 8;
            o->vel_q8.y -= HERO_VY_BOOST_SPRINT_ABS;
        }
        h->jumped_ticks = 6;
    }

    if (!h->sliding) {
        int dpad_x = inp_dpad_x();
        if (dpad_x != sgn_i(o->vel_q8.x)) {
            o->vel_q8.x /= 2;
        }

        if (dpad_x) {
            h->sprint_ticks += dpad_x;
            bool32 sprinting = hero_is_sprinting(h);
            if (dpad_x != sgn_i(h->sprint_ticks)) {
                h->sprint_ticks = sprinting ? -h->sprint_ticks : 0;
                sprinting       = hero_is_sprinting(h);
            }

            int vt      = HERO_VX_WALK;
            int sprintt = abs_i(h->sprint_ticks) - HERO_SPRINT_TICKS;
            if (0 <= sprintt) {
                vt = lerp_i32(vt, HERO_VX_SPRINT, min_i(sprintt, 15), 15);
            }
            int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
            int ax = (max_i(vt - i0, 0) * 256) >> 8;
            o->vel_q8.x += ax * dpad_x;
        } else {
            h->sprint_ticks = 0;
        }

#if 0
        if (inp_just_pressed(INP_DPAD_R) || inp_just_pressed(INP_DPAD_L)) {
            if (h->sprint_dir == dpad_x && h->sprint_ticks < 18) {
                h->sprinting = 1;
            }
            h->sprint_ticks = 0;
            h->sprint_dir   = dpad_x;
        }

        if (h->sprinting && h->dash_ticks == 0 && inp_just_pressed(INP_B)) {
            h->dash_ticks   = 1;
            o->gravity_q8.y = 0;
            o->vel_q8.y     = 0;
            h->jumpticks    = 0;
            h->edgeticks    = 0;
            h->sprint_ticks = 0;
            h->sprinting    = 0;
            o->vel_q8.x     = sgn_i(o->vel_q8.x) * 1500;
            snd_play_ext(SNDID_ATTACK_DASH, 0.5f, 1.f);
        }
#endif
    }
}

static void hero_update_air(game_s *g, obj_s *o, hero_s *h)
{
    o->animation++;
    h->edgeticks--;

    // dynamic jump height
    if (0 < h->jumpticks && !inp_pressed(INP_A)) {
        h->jumpticks = 0;
    }

    if (0 < h->jumpticks) {
        hero_jumpvar_s jv = g_herovar[h->jump_index];
        i32            t0 = pow2_i32(jv.ticks);
        i32            ti = pow2_i32(h->jumpticks) - t0;
        o->vel_q8.y -= jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
    }

    if (inp_just_pressed(INP_A)) {
        h->jump_btn_buffer = 6;
    }

    if (hero_has_upgrade(&g->herodata, HERO_UPGRADE_GLIDE) && inp_pressed(INP_DPAD_U)) {
        h->gliding      = 1;
        h->sprint_ticks = 0;
        if (HERO_GLIDE_VY < o->vel_q8.y) {
            o->vel_q8.y -= HERO_GRAVITY * 2;
            o->vel_q8.y = max_i(o->vel_q8.y, HERO_GLIDE_VY);
        }
    }

#if 0
    bool32 wallr = game_traversable(g, obj_rec_right(o));
    bool32 walll = game_traversable(g, obj_rec_left(o));
    if (!h->runup_wall && ((wallr && inp_dpad_x() == 1) || (wallr && inp_dpad_x() == 1))) {
        h->runup_wall = 1;
        h->runup_wall_ticks++;
    }
#endif

    h->jumpticks--;
    bool32 usehook = 0; // TODO

    if (0 < h->jump_btn_buffer) {
        int    jump_wall   = hero_can_walljump(g, o);
        bool32 jump_ground = 0 < h->edgeticks;
        bool32 jump_midair = !usehook &&             // not hooked
                             h->edgeticks <= 0 &&    // jump in air?
                             0 < h->airjumps_left && // air jumps left?
                             h->jumpticks <= -20;    // wait some ticks after last jump

        if (jump_ground) {
            hero_start_jump(g, o, h, 0);
        } else if (jump_wall && inp_dpad_x() == jump_wall) {
            o->vel_q8.y        = -1300;
            o->vel_q8.x        = jump_wall * 520;
            h->edgeticks       = 0;
            h->jumpticks       = 0;
            h->jump_btn_buffer = 0;
            h->walljumpticks   = 12;
        } else if (jump_midair) {
            int jumpindex = 2 + g->herodata.n_airjumps - h->airjumps_left;
            h->airjumps_left--;
            h->sprint_ticks = 0;
            hero_start_jump(g, o, h, jumpindex);
        }
    }

    int dpad_x = inp_dpad_x();
    if (dpad_x) {
        int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        int ax = (max_i(256 - i0, 0) * 64) >> 8;
        o->vel_q8.x += ax * dpad_x;
    }

    if (dpad_x != sgn_i(h->sprint_ticks)) {
        h->sprint_ticks = 0;
    }

    if (inp_just_pressed(INP_B)) {
        hero_item_use(g, o, h);
    }
}

// HOOK OBJ ====================================================================

static obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8)
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

void hook_update(game_s *g, obj_s *h, obj_s *hook)
{
    rope_s *r = hook->rope;

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

    rope_update(g, &g->herodata.rope);
    if (hook->attached) {
        h->vel_q8 = obj_constrain_to_rope(g, h);
    } else {
        // hook->vel_q8 = rope_adjust_connected_vel(g, r, r->tail, hook->subpos_q8, hook->vel_q8);
        hook->vel_q8 = obj_constrain_to_rope(g, hook);
    }
}

void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
    obj_delete(g, ohook);
    ohero->ropenode         = NULL;
    ohero->rope             = NULL;
    ohook->rope             = NULL;
    ohook->ropenode         = NULL;
    g->herodata.rope_active = 0;
}