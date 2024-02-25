// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN             500
#define HERO_ROPE_LEN_MIN_JUST_HOOKED 1000
#define HERO_ROPE_LEN_SHORT           3000
#define HERO_ROPE_LEN_LONG            5000
#define HERO_GRAVITY                  80
#define HERO_DRAG_Y                   256
#define HERO_WHIP_TICKS               14
#define HERO_GLIDE_VY                 200
#define HERO_SPRINT_TICKS             60 // ticks walking until sprinting
#define HERO_VX_WALK                  576
#define HERO_VX_SPRINT                900
#define HERO_VY_BOOST_SPRINT_ABS      0   // absolute vy added to a jump sprinted
#define HERO_VX_BOOST_SPRINT_Q8       300 // vx multiplier when jumping sprinted
#define HERO_VX_BOOST_SLIDE_Q8        370 // vx multiplier when jumping slided
#define HERO_DRAG_SLIDING             250
#define HERO_REEL_RATE                30
#define HERO_ROPEWALLJUMP_TICKS       30

#define HEROHOOK_N_HIST 4

typedef struct {
    int    n_ang;
    v2_f32 anghist[HEROHOOK_N_HIST];

} herohook_s;

static_assert(sizeof(herohook_s) <= 256, "M");

typedef struct {
    int vy;
    int ticks; // ticks of variable jump (decreases faster if jump button is not held)
    int v0;    // "jetpack" velocity, goes to v1 over ticks or less
    int v1;
} hero_jumpvar_s;

enum {
    HERO_JUMP_WATER,
    HERO_JUMP_GROUND,
    HERO_JUMP_AIR_1,
    HERO_JUMP_AIR_2,
    HERO_JUMP_AIR_3,
    HERO_JUMP_WALL,
};

static const hero_jumpvar_s g_herovar[6] = {
    {825, 25, 70, 30},   // out of water
    {925, 30, 80, 40},   // ground
    {300, 20, 170, 40},  // airj 1
    {150, 25, 180, 0},   // airj 2
    {100, 25, 130, 0},   // airj 3
    {1000, 25, 80, 30}}; // wall jump

static int    hero_max_rope_len_q4(herodata_s *h);
static void   hero_start_jump(game_s *g, obj_s *o, hero_s *h, int ID);
static void   hero_item_use(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_ladder(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_swimming(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_ground(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_air(game_s *g, obj_s *o, hero_s *h);
static void   hero_update_air_hook(game_s *g, obj_s *o, hero_s *h);
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout);
static int    hero_can_walljump(game_s *g, obj_s *o);
static void   hero_restore_grounded_stuff(game_s *g, obj_s *o, hero_s *h);
static void   hero_hook_reel(game_s *g, obj_s *o, int dt);
static bool32 hero_unhook(game_s *g, obj_s *o);
//
static obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8);
void          hook_update(game_s *g, obj_s *h, obj_s *hook);

static bool32 hero_try_snap_to_ladder(game_s *g, obj_s *o, int diry)
{
    rec_i32 aabb    = obj_aabb(o);
    rec_i32 rladder = aabb;
    if (obj_grounded(g, o) && 0 < diry) {
        rladder = obj_rec_bottom(o);
    }

    v2_i32 lpos;
    if (!ladder_overlaps_rec(g, rladder, &lpos)) return 0;

    hero_s *hero = (hero_s *)o->mem;

    int posx = (lpos.x << 4) + 8 - (aabb.w / 2);
    aabb.x   = posx;
    aabb.y += diry;
    if (!game_traversable(g, aabb)) return 0;

    hero_unhook(g, o);
    o->flags &= ~OBJ_FLAG_MOVER;
    hero_restore_grounded_stuff(g, o, hero);
    o->pos.x = posx;
    o->pos.y += diry;
    o->vel_q8.x     = 0;
    o->vel_q8.y     = 0;
    o->animation    = 0;
    hero->onladder  = 1;
    hero->ladderx   = posx;
    hero->sprinting = 0;
    return 1;
}

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
               // OBJ_FLAG_RENDER_AABB |
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
    o->vel_cap_q8.y    = 2500;
    o->w               = 12;
    o->h               = 26;
    o->facing          = 1;
    return o;
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

static int hero_can_walljump(game_s *g, obj_s *o)
{
    if (!(g->herodata.upgrades[HERO_UPGRADE_WALLJUMP])) return 0;

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

    if (dst_r == dst_l) return 0;
    if (dst_l < dst_r) return +1;
    if (dst_r < dst_l) return -1;
    return 0;
}

static int hero_max_rope_len_q4(herodata_s *h)
{

    if (h->upgrades[HERO_UPGRADE_LONG_HOOK])
        return HERO_ROPE_LEN_LONG;
    return HERO_ROPE_LEN_SHORT;
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
    sprite->offs.x          = o->w / 2 - 32;
    sprite->offs.y          = o->h - 64 + 16;
    sprite->mode            = ((o->invincible_tick >> 2) & 1) ? SPR_MODE_INV : 0;
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
        if (0 < h->ground_impact_ticks && !h->carrying) {
            animID  = 2; // "oof"
            frameID = 4;
            break;
        }

        sprite->offs.y -= 16;

        if (o->vel_q8.x != 0) {
            animID           = h->carrying ? 8 : 0; // "carrying/walking"
            int frameID_prev = (o->animation / 4000) & 3;
            o->animation += abs_i(o->vel_q8.x);
            frameID = (o->animation / 4000) & 3;
            if (frameID_prev != frameID && (frameID & 1) == 0) {
                // snd_play_ext(SNDID_STEP, 0.5f, 1.f);
            }
            break;
        }
        animID = h->carrying ? 7 : 1; // "carrying/idle"
        if (o->vel_prev_q8.x != 0) {
            o->animation = 0; // just got idle
        } else {
            o->animation += 100;
            frameID = (o->animation / 4000) & 1;
        }
        break;
    }

    case HERO_STATE_LADDER: {
        animID       = 4; // "ladder"
        frameID      = 0;
        sprite->flip = ((o->animation >> 3) & 1) ? SPR_FLIP_X : 0;
        break;
    }
    case HERO_STATE_SWIMMING: { // repurpose jumping animation for swimming
        animID   = 2;           // "air"
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
    case HERO_STATE_AIR_HOOK:
    case HERO_STATE_AIR: {
        if (h->walljumpticks) {
            animID       = 6; // "walljump"
            sprite->flip = 0 < o->vel_q8.x ? 0 : SPR_FLIP_X;
            break;
        }

        sprite->offs.y += 4;
        if (h->carrying) {
            animID = 9; // "carrying air"
            break;
        }

        animID = 2; // "air"
        if (0 < h->ground_impact_ticks) {
            frameID = 0;
        } else if (h->gliding) {
            frameID = 2 + (8 <= (o->animation & 63));
        } else if (+300 <= o->vel_q8.y) {
            frameID = 3;
        } else if (-300 <= o->vel_q8.y) {
            frameID = 2;
        } else {
            frameID = 1;
        }
        break;
    }
    case HERO_STATE_DEAD: {
        sprite->offs.y -= 16;
        frameID = 0;
        animID  = 0;
    } break;
    }

    rec_i32 frame  = {frameID * 64, animID * 64, 64, 64};
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

void hero_hurt(game_s *g, obj_s *o, int damage)
{
    if (0 < o->invincible_tick) return;
    o->health -= damage;
#if GAME_HERO_CANT_DIE
    o->health = max_i(o->health, 1);
#endif

    if (0 < o->health) {
        o->invincible_tick = ticks_from_ms(1000);
    } else {
        hero_kill(g, o);
    }
}

void hero_kill(game_s *g, obj_s *o)
{
    o->health = 0;
    o->flags &= ~OBJ_FLAG_CLAMP_ROOM_Y; // let hero fall through floor
    hero_unhook(g, o);
}

int hero_determine_state(game_s *g, obj_s *o, hero_s *h)
{
    if (o->health <= 0) return HERO_STATE_DEAD;

    int water_depth = water_depth_rec(g, obj_aabb(o));
    if (15 < water_depth) return HERO_STATE_SWIMMING;

    bool32 grounded = obj_grounded(g, o) && 0 <= o->vel_q8.y;
    if (grounded) return HERO_STATE_GROUND;

    if (h->onladder) {
        if (ladder_overlaps_rec(g, obj_aabb(o), NULL) &&
            h->ladderx == o->pos.x) {
            return HERO_STATE_LADDER;
        } else {
            h->onladder = 0;
        }
    }

    rope_s *r = o->rope;
    if (r && (r->len_max_q4 * 254) <= (rope_length_q4(g, r) << 8)) {
        return HERO_STATE_AIR_HOOK;
    }

    return HERO_STATE_AIR;
}

static void hero_restore_grounded_stuff(game_s *g, obj_s *o, hero_s *h)
{
    bool32 can_swim = (g->herodata.upgrades[HERO_UPGRADE_SWIM]) != 0;
    h->swimticks    = can_swim ? 2000 : 10000;
    if (g->herodata.upgrades[HERO_UPGRADE_AIR_JUMP_3]) {
        g->herodata.n_airjumps = 3;
    } else if (g->herodata.upgrades[HERO_UPGRADE_AIR_JUMP_2]) {
        g->herodata.n_airjumps = 2;
    } else if (g->herodata.upgrades[HERO_UPGRADE_AIR_JUMP_1]) {
        g->herodata.n_airjumps = 1;
    }
    h->airjumps_left = g->herodata.n_airjumps;
}

// tries to calculate a snapped ladder position
static bool32 hero_rec_on_ladder(game_s *g, rec_i32 aabb, rec_i32 *rout)
{
    int tx1 = (aabb.x >> 4);
    int tx2 = ((aabb.x + aabb.w - 1) >> 4);
    int ty  = ((aabb.y + (aabb.h >> 1)) >> 4);

    for (int tx = tx1; tx <= tx2; tx++) {
        if (g->tiles[tx + ty * g->tiles_x].collision != TILE_LADDER) continue;

        int     lc_x = (tx << 4) + 8;
        rec_i32 r    = {lc_x - (aabb.w >> 1), aabb.y, aabb.w, aabb.h}; // aabb if on ladder
        if (!game_traversable(g, r)) continue;                         // if ladder position is valid
        if (rout) {
            *rout = r;
        }
        return 1;
    }
    return 0;
}

// tries to calculate a snapped ladder position
static bool32 hero_rec_ladder(game_s *g, obj_s *o, rec_i32 *rout)
{
    return hero_rec_on_ladder(g, obj_aabb(o), rout);
}

static bool32 hero_unhook(game_s *g, obj_s *o)
{
    if (obj_handle_valid(o->obj_handles[0])) {
        hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
        int vx_abs = abs_i(o->vel_q8.x);
        int vy_abs = abs_i(o->vel_q8.y);

        int mulx = vx_abs < 800 ? 350 : 300;
        int muly = vy_abs < 800 ? 350 : 300;

        o->vel_q8.x = (o->vel_q8.x * mulx) >> 8;
        o->vel_q8.y = (o->vel_q8.y * muly) >> 8;

        return 1;
    }
    return 0;
}

static void hero_start_jump(game_s *g, obj_s *o, hero_s *h, int ID)
{
    snd_play_ext(SNDID_SPEAK, 1.f, 0.5f);
    hero_jumpvar_s jv  = g_herovar[ID];
    h->jump_index      = ID;
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
    snd_play_ext(SNDID_HOOK_THROW, 1.f, 1.f);
    // throw new hook
    int dirx = inp_dpad_x();
    int diry = inp_dpad_y();

    if (dirx == 0 && diry == 0) {
        diry = -1;
        dirx = o->facing;
    }

    v2_i32 center  = obj_pos_center(o);
    v2_i32 vlaunch = {dirx * 2100, diry * 1700};

    if (diry < 0) {
        vlaunch.y = (vlaunch.y * 5) / 4;
    } else if (diry == 0) {
        vlaunch.y = -500;
        vlaunch.x = (vlaunch.x * 5) / 4;
    }

    g->herodata.rope_active = 1;
    rope_s *rope            = &g->herodata.rope;
    obj_s  *hook            = hook_create(g, rope, center, vlaunch);
    o->obj_handles[0]       = obj_handle_from_obj(hook);
    hook->obj_handles[0]    = obj_handle_from_obj(o);
    o->rope                 = rope;
    o->ropenode             = rope->head;

    herodata_s *hd    = &g->herodata;
    v2_i32      pcurr = v2_shl(center, 8);
    // "hint" the direction to the verlet sim
    v2_i32      pprev = {pcurr.x - (dirx << 9), pcurr.y - (diry << 9)};
    for (int n = 0; n < ROPE_VERLET_N; n++) {
        hd->hookpt[n].p  = pcurr;
        hd->hookpt[n].pp = pprev;
    }
}

static void hero_item_use(game_s *g, obj_s *o, hero_s *h)
{
    if (!g->herodata.upgrades[HERO_UPGRADE_HOOK] &&
        !g->herodata.upgrades[HERO_UPGRADE_WHIP]) return;

    int item = g->item_selector.item;

    if (item == 0 && !g->herodata.upgrades[HERO_UPGRADE_HOOK]) {
        item = 1;
    }
    if (item == 1 && !g->herodata.upgrades[HERO_UPGRADE_WHIP]) {
        item = 0;
    }

#if GAME_JUMP_ATTACK
    if (!g->herodata.upgrades[HERO_UPGRADE_HOOK]) return;
    item = 0;
#endif
    switch (item) {
    case 0:
        if (!hero_unhook(g, o)) {
            hero_use_hook(g, o, h);
        }
        break;
    case 1:
        hero_use_weapon(g, o, h);
        h->jumpticks     = 0;
        h->airjumps_left = 0;
        h->sprint_ticks  = 0;
        break;
    }
}

static bool32 hero_is_sprinting(hero_s *h)
{
    return (HERO_SPRINT_TICKS <= abs_i(h->sprint_ticks));
}

void hero_on_update(game_s *g, obj_s *o)
{
    herodata_s *herodata = &g->herodata;
    hero_s     *hero     = (hero_s *)o->mem;

#if !GAME_JUMP_ATTACK
    if (g->item_selector.item != 0 && herodata->rope_active &&
        g->herodata.upgrades[HERO_UPGRADE_WHIP]) {
        hook_destroy(g, o, obj_from_obj_handle(o->obj_handles[0]));
    }
#endif

    int state = hero_determine_state(g, o, hero);

    int dpad_x = inp_dpad_x();
    int dpad_y = inp_dpad_y();
    if (state == HERO_STATE_DEAD) {
        dpad_x = 0;
        dpad_y = 0;
    }

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
    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    o->gravity_q8.y = HERO_GRAVITY;
    hero->gliding   = 0;
    hero->jump_btn_buffer--;
    hero->ground_impact_ticks--;

    if (state == HERO_STATE_SWIMMING) {
        hero_unhook(g, o);
    }

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

    if ((o->bumpflags & OBJ_BUMPED_Y) && !(o->bumpflags & OBJ_BUMPED_Y_BOUNDS)) {
        if (state == HERO_STATE_AIR_HOOK) {
            o->vel_q8.y = -(o->vel_q8.y >> 2);
        } else {
            if (1000 <= o->vel_q8.y) {
                f32 vol = (1.f * (f32)o->vel_q8.y) / 2000.f;
                snd_play_ext(SNDID_STEP, min_f(vol, 1.f) * 1.2f, 1.f);
            }

            if (250 <= o->vel_q8.y) {
                hero->ground_impact_ticks = min_i(o->vel_q8.y >> 8, 6);
            }

            o->vel_q8.y = (o->bumpflags & OBJ_BUMPED_ON_HEAD ? -1000 : 0);
        }
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        hero->sprint_ticks = 0;
        if (hero->sliding) {
            o->vel_q8.x = -(o->vel_q8.x >> 2);
        } else if (state == HERO_STATE_AIR_HOOK) {
            if (dpad_x == sgn_i(o->vel_q8.x) || abs_i(o->vel_q8.x) < 600) {
                o->vel_q8.x = 0;
            } else {
                o->vel_q8.x = -(o->vel_q8.x >> 2);
            }

        } else {
            o->vel_q8.x = 0;
        }
    }
    o->bumpflags = 0;

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_AIR: {
        if (!inp_just_pressed(INP_DPAD_U) && !inp_just_pressed(INP_DPAD_D)) break;
        if (hero_try_snap_to_ladder(g, o, dpad_y)) {
            return; // RETURN
        }
        break;
    }
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        hero->sprinting = 0;
        break;
    }

    switch (state) {
    case HERO_STATE_GROUND:
    case HERO_STATE_LADDER: {
        hero_restore_grounded_stuff(g, o, hero);
        break;
    }
    }

    switch (state) {
    case HERO_STATE_GROUND:
        o->drag_q8.x = 250;
        hero_update_ground(g, o, hero);
        break;
    case HERO_STATE_AIR:
        o->drag_q8.x = hero->sliding ? 253 : 240;
        hero_update_air(g, o, hero);
        break;
    case HERO_STATE_AIR_HOOK:
        o->drag_q8.x = 255;
        hero_update_air_hook(g, o, hero);
        break;
    case HERO_STATE_SWIMMING:
        o->drag_q8.x = 240;
        hero_update_swimming(g, o, hero);
        break;
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        hero_update_ladder(g, o, hero);
        break;
    case HERO_STATE_DEAD:
        o->drag_q8.x = obj_grounded(g, o) ? 210 : 245;
        break;
    }

    obj_s *ohook = obj_from_obj_handle(o->obj_handles[0]);
    if (ohook) {
        hook_update(g, o, ohook);
    }
}

static void hero_update_ladder(game_s *g, obj_s *o, hero_s *h)
{
    if (o->pos.x != h->ladderx) {
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

    if (inp_just_pressed(INP_B)) {
        h->onladder = 0;
        return;
    }

    int dpad_y = inp_dpad_y();
    int dir_y  = dpad_y << 1;

    for (int m = abs_i(dir_y); m; m--) {
        rec_i32 aabb = obj_aabb(o);
        aabb.y += dpad_y;
        if (!game_traversable(g, aabb)) break;
        if (obj_grounded_at_offs(g, o, (v2_i32){0, dpad_y})) {
            o->pos.y += dpad_y;
            h->onladder = 0;
            break;
        }
        if (!ladder_overlaps_rec(g, aabb, NULL)) break;
        o->pos.y += dpad_y;
    }

    if (dir_y) {
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

        o->vel_q8.y -= lerp_i32(20, 70, k0, k1) + lerp_i32(0, 100, i0, i1);
        o->animation++;
    } else {
        o->vel_q8.y -= 25 + water_depth;
    }

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
        hero_start_jump(g, o, h, HERO_JUMP_WATER);
    }
}

static void hero_update_ground(game_s *g, obj_s *o, hero_s *h)
{
    bool32 can_swim = (g->herodata.upgrades[HERO_UPGRADE_SWIM]) != 0;
    h->edgeticks    = 6;

    int dpad_x = inp_dpad_x();
    int dpad_y = inp_dpad_y();

    v2_i32 posc              = obj_pos_center(o);
    obj_s *interactable      = obj_closest_interactable(g, posc);
    g->herodata.interactable = obj_handle_from_obj(interactable);

    if (inp_just_pressed(INP_B)) {
        hero_item_use(g, o, h);
    }

    bool32 usinghook = o->rope != NULL;
    if (dpad_y && usinghook) {
        hero_hook_reel(g, o, dpad_y * HERO_REEL_RATE);
    }

    if (dpad_y == 1 && !usinghook) { // sliding
        h->sprint_ticks = 0;
        int accx        = 0;
        if (0) {
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+1, 0})) {
            accx = +85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-1, 0})) {
            accx = -85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+2, 0})) {
            accx = +40;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-2, 0})) {
            accx = -40;
        }

        if (accx) {
            h->sliding = 1;
        }

        if (h->sliding) {
            o->drag_q8.x = accx ? HERO_DRAG_SLIDING : 245;
            o->vel_q8.x += accx;
        }

        if (h->sliding && rngr_i32(0, 2)) {
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

    if ((inp_just_pressed(INP_A) || 0 < h->jump_btn_buffer) && dpad_y <= 0) {
        hero_start_jump(g, o, h, HERO_JUMP_GROUND);
        if (h->sliding) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SLIDE_Q8) >> 8;
        }
        if (hero_is_sprinting(h)) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SPRINT_Q8) >> 8;
            o->vel_q8.y -= HERO_VY_BOOST_SPRINT_ABS;
        }
        h->ground_impact_ticks = 6;
    }

    if (!h->sliding) {

        if (dpad_x != sgn_i(o->vel_q8.x)) {
            o->vel_q8.x /= 2;
        }

        if (inp_just_pressed(INP_DPAD_D)) {
            o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
            o->tomove.y += 1;
        }

        if (o->vel_prev_q8.x == 0 && o->vel_q8.x != 0) {
            o->animation = 0;
        }

        if (dpad_x) {
            if (g->herodata.upgrades[HERO_UPGRADE_SPRINT]) {
                h->sprint_ticks += dpad_x;
            }
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
    }

    if (inp_just_pressed(INP_A) && !h->sliding && h->jump_btn_buffer <= 0) {
        // h->carrying = 1;
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
        h->jump_btn_buffer = 12;
    }

    if ((g->herodata.upgrades[HERO_UPGRADE_GLIDE]) && inp_pressed(INP_DPAD_U)) {
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

    int    dpad_x    = inp_dpad_x();
    int    dpad_y    = inp_dpad_y();
    bool32 usinghook = o->rope != NULL;
    if (dpad_y && usinghook) {
        hero_hook_reel(g, o, dpad_y * HERO_REEL_RATE);
    }

    h->jumpticks--;
    bool32 usehook = (o->rope != NULL);

    if (0 < h->jump_btn_buffer) {
        int    jump_wall   = hero_can_walljump(g, o);
        bool32 jump_ground = 0 < h->edgeticks;
        bool32 jump_midair = !usinghook &&           // not hooked
                             !jump_ground &&         // jump in air?
                             0 < h->airjumps_left && // air jumps left?
                             h->jumpticks <= -12;    // wait some ticks after last jump

        if (jump_midair && !jump_ground && !jump_wall) { // just above ground -> ground jump
            for (int y = 0; y < 8; y++) {
                rec_i32 rr = {o->pos.x, o->pos.y + o->h + y, o->w, 1};
                v2_i32  pp = {0, y + 1};
                if (game_traversable(g, rr) && obj_grounded_at_offs(g, o, pp)) {
                    jump_ground = 1;
                    hero_restore_grounded_stuff(g, o, h);
                    break;
                }
            }
        }

        if (jump_ground) {
            hero_start_jump(g, o, h, HERO_JUMP_GROUND);
        } else if (jump_wall && inp_dpad_x() == jump_wall) {
            o->vel_q8.x      = jump_wall * 520;
            h->walljumpticks = 12;
            hero_start_jump(g, o, h, HERO_JUMP_WALL);
        } else if (jump_midair) {
            int jumpindex = 2 + g->herodata.n_airjumps - h->airjumps_left;
            h->airjumps_left--;
            h->sprint_ticks = 0;
            hero_start_jump(g, o, h, jumpindex);
        }
    }

    if (dpad_x) {
        int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        int ax = (max_i(HERO_VX_WALK - i0, 0) * 32) >> 8;
        o->vel_q8.x += ax * dpad_x;
        o->drag_q8.x = 256;
    }

    if (dpad_x != sgn_i(h->sprint_ticks)) {
        h->sprint_ticks = 0;
    }

    if (inp_just_pressed(INP_B)) {
        hero_item_use(g, o, h);
    }
}

static void hero_hook_reel(game_s *g, obj_s *o, int dt)
{
    u32 l_new_q4 = o->rope->len_max_q4 + dt;
    l_new_q4     = clamp_i(l_new_q4,
                           HERO_ROPE_LEN_MIN,
                           hero_max_rope_len_q4(&g->herodata));
    rope_set_len_max_q4(o->rope, l_new_q4);
}

static void hero_update_air_hook(game_s *g, obj_s *o, hero_s *h)
{
    if (inp_pressed(INP_A) && 0 < h->jumpticks) {
        h->jumpticks--;
        i32 vadd = lerp_i32(20, 50, h->jumpticks, HERO_ROPEWALLJUMP_TICKS);
        o->vel_q8.x += h->ropewalljump_dir * vadd;
    } else {
        h->jumpticks = 0;
    }

    if (inp_just_pressed(INP_B)) {
        hero_item_use(g, o, h);
        return;
    }

    if (inp_just_pressed(INP_A)) {
        h->jump_btn_buffer = 8;
    }

    o->drag_q8.x = 253;
    o->drag_q8.y = 256;

    int dpad_x = inp_dpad_x();
    int dpad_y = inp_dpad_y();

    if (dpad_y) {
        hero_hook_reel(g, o, dpad_y * HERO_REEL_RATE);
    }

    u32    lm_q4       = o->rope->len_max_q4;
    u32    lc_q4       = rope_length_q4(g, o->rope);
    bool32 r_stretched = (lm_q4 <= lc_q4);
    v2_i32 rn_curr     = o->ropenode->p;
    v2_i32 rn_next     = ropenode_neighbour(o->rope, o->ropenode)->p;
    v2_i32 dtrope      = v2_sub(rn_next, rn_curr);
    int    dtrope_s    = sgn_i(dtrope.x);
    int    dtrope_a    = abs_i(dtrope.x);

    if (dpad_x) {
        if (dtrope_s == dpad_x) {
            o->vel_q8.x += 40 * dpad_x;
        } else {
            o->vel_q8.x += 10 * dpad_x;
        }
    }

    if (0 < h->jump_btn_buffer) {
        bool32 jump_ground = 0; // just above ground? -> ground jump

        for (int y = 0; y < 6; y++) {
            rec_i32 rr = {o->pos.x, o->pos.y + o->h + y, o->w, 1};
            v2_i32  py = {0, y + 1};
            if (game_traversable(g, rr) && obj_grounded_at_offs(g, o, py)) {
                jump_ground = 1;
                break;
            }
        }

        if (jump_ground) {
            hero_restore_grounded_stuff(g, o, h);
            hero_start_jump(g, o, h, HERO_JUMP_GROUND);
        } else { // "walljump"
            rec_i32 r_r    = {o->pos.x + o->w, o->pos.y, 8, o->h};
            rec_i32 r_l    = {o->pos.x - 8, o->pos.y, 8, o->h};
            bool32  wall_r = !game_traversable(g, r_r);
            bool32  wall_l = !game_traversable(g, r_l);

            h->ropewalljump_dir = wall_r ? -1 : (wall_l ? +1 : 0);
            o->vel_q8.x         = h->ropewalljump_dir * 250;
            if (h->ropewalljump_dir) {
                h->jumpticks       = HERO_ROPEWALLJUMP_TICKS;
                h->jump_btn_buffer = 0;
            }
        }
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

static void hook_verlet_sim(game_s *g, obj_s *o, rope_s *r)
{
    typedef struct {
        i32 i;
        v2i p;
    } verlet_pos_s;

    herodata_s  *hd             = &g->herodata;
    verlet_pos_s vpos[64]       = {0};
    int          n_vpos         = 0;
    u32          ropelen_max_q8 = r->len_max_q4 << 4;
    u32          ropelen_q8     = 1; // avoid div 0
    u32          dista          = 0;

    for (ropenode_s *r1 = r->head, *r2 = r1->next; r2; r1 = r2, r2 = r2->next) {
        ropelen_q8 += v2_len(v2_shl(v2_sub(r1->p, r2->p), 8));
    }

    verlet_pos_s vp_beg = {0, v2_shl(r->tail->p, 8)};
    vpos[n_vpos++]      = vp_beg;

    for (ropenode_s *r1 = r->tail, *r2 = r1->prev; r2; r1 = r2, r2 = r2->prev) {
        dista += v2_len(v2_shl(v2_sub(r1->p, r2->p), 8));
        int i = (dista * ROPE_VERLET_N) / ropelen_q8;
        if (1 <= i && i < ROPE_VERLET_N - 1) {
            verlet_pos_s vp = {i, v2_shl(r2->p, 8)};
            vpos[n_vpos++]  = vp;
        }
    }

    verlet_pos_s vp_end = {ROPE_VERLET_N - 1, v2_shl(r->head->p, 8)};
    vpos[n_vpos++]      = vp_end;

    // factor for constraint -> artificially shorten segment distance when getting tighter
    f32 k_len = min_f(1.f, (f32)ropelen_q8 / (f32)ropelen_max_q8);
    k_len     = k_len * k_len * k_len;
    k_len     = 0.75f + 0.25f * (1.f - k_len);

    int ll_q8 = (int)((f32)ropelen_max_q8 * k_len) / ROPE_VERLET_N;

    for (int n = 1; n < ROPE_VERLET_N - 1; n++) {
        hook_pt_s *pt  = &hd->hookpt[n];
        v2i        tmp = pt->p;
        pt->p.x += (pt->p.x - pt->pp.x);
        pt->p.y += (pt->p.y - pt->pp.y) + ROPE_VERLET_GRAV;
        pt->pp = tmp;
    }
    for (int n = n_vpos - 1; 0 <= n; n--) {
        hd->hookpt[vpos[n].i].p = vpos[n].p;
    }

    for (int k = 0; k < ROPE_VERLET_IT; k++) {
        for (int n = 1; n < ROPE_VERLET_N; n++) {
            hook_pt_s *p1 = &hd->hookpt[n - 1];
            hook_pt_s *p2 = &hd->hookpt[n];
            v2i        dt = v2_sub(p1->p, p2->p);
            v2f        df = {(f32)dt.x, (f32)dt.y};
            f32        dl = v2f_len(df);
            i32        dd = (i32)(dl + .5f) - ll_q8;
            if (dd <= 1) continue;
            dt    = v2_setlenl(dt, dl, dd >> 1);
            p1->p = v2_sub(p1->p, dt);
            p2->p = v2_add(p2->p, dt);
        }

        for (int n = n_vpos - 1; 0 <= n; n--) {
            hd->hookpt[vpos[n].i].p = vpos[n].p;
        }
    }
}

void hook_on_animate(game_s *g, obj_s *o)
{
    assert(o->rope && o->rope->head && o->rope->tail);

    rope_s     *r  = o->rope;
    herohook_s *h  = (herohook_s *)o->mem;
    ropenode_s *rn = ropenode_neighbour(r, o->ropenode);
    herodata_s *hd = &g->herodata;
    rope_update(g, r);
    hook_verlet_sim(g, o, r);

    if (o->state) return;

    v2i rndt               = v2_sub(o->ropenode->p, rn->p);
    v2f v                  = {(f32)rndt.x, (f32)rndt.y};
    h->anghist[h->n_ang++] = v;
    h->n_ang %= HEROHOOK_N_HIST;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        v = v2f_add(v, h->anghist[i]);
    }

    f32 ang  = (atan2f(v.y, v.x) * 16.f) / PI2_FLOAT;
    int imgy = (int)(ang + 16.f + 4.f) & 15;

    o->sprites[0].trec.r.y = imgy * 32;
}

bool32 hook_can_attach(game_s *g, obj_s *o, obj_s **ohook)
{
    rec_i32 hr = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};

    for (obj_each(g, it)) {
        // if (!(it->flags & OBJ_FLAG_HOOKABLE)) continue;
        if (!overlap_rec(hr, obj_aabb(it))) continue;
        if (!(it->flags & OBJ_FLAG_SOLID)) continue;
        if (ohook) {
            *ohook = it;
        }
        return 1;
    }

    if (tiles_hookable(g, hr)) {
        return 1;
    }
    return 0;
}

void hook_update(game_s *g, obj_s *h, obj_s *hook)
{
    rope_s *r = hook->rope;

    if (!rope_intact(g, r)) {
        hero_unhook(g, h);
        return;
    }

    obj_s *tohook = NULL;
    if (hook->state) {
        // check if still attached
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};

        if (hook_can_attach(g, hook, &tohook)) {
            obj_s *solid;
            if (obj_try_from_obj_handle(hook->linked_solid, &solid) &&
                !overlap_rec(hookrec, obj_aabb(solid))) {
                hook->linked_solid.o = NULL;
            }
        } else {
            hook->state          = 0;
            hook->linked_solid.o = NULL;
        }
    } else {
        v2_i32 hookp = hook->pos;
        obj_apply_movement(hook);
        actor_move(g, hook, hook->tomove);
        hook->tomove.x  = 0;
        hook->tomove.y  = 0;
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
        if (hook_can_attach(g, hook, &tohook)) {
            i32 mlen_q4 = hero_max_rope_len_q4(&g->herodata);
            i32 clen_q4 = rope_length_q4(g, r);

            int herostate = hero_determine_state(g, h, (hero_s *)h->mem);
            if (herostate == HERO_STATE_AIR || herostate == HERO_STATE_AIR_HOOK) {
                clen_q4 = (clen_q4 * 240) >> 8;
            }
            int newlen_q4 = clamp_i(clen_q4, HERO_ROPE_LEN_MIN_JUST_HOOKED, mlen_q4);
            rope_set_len_max_q4(r, newlen_q4);
            snd_play_ext(SNDID_HOOK_ATTACH, 1.f, 1.f);
            hook->state    = 1;
            hook->vel_q8.x = 0;
            hook->vel_q8.y = 0;

            for (obj_each(g, solid)) {
                if (!overlap_rec(hookrec, obj_aabb(solid))) continue;
                if (solid->ID == OBJ_ID_CRUMBLEBLOCK) {
                    crumbleblock_on_hooked(solid);
                }
                if (!(solid->flags & OBJ_FLAG_SOLID)) continue;

                int kk             = overlap_rec(hookrec, obj_aabb(solid));
                hook->linked_solid = obj_handle_from_obj(solid);
            }
        } else {
            if (hook->bumpflags & OBJ_BUMPED_X) {
                if (abs_i(hook->vel_q8.x) > 700) {
                    snd_play_ext(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
                }
                hook->vel_q8.x = -hook->vel_q8.x / 3;
            }
            if (hook->bumpflags & OBJ_BUMPED_Y) {
                if (abs_i(hook->vel_q8.y) > 700) {
                    snd_play_ext(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
                }
                hook->vel_q8.y = -hook->vel_q8.y / 3;
            }
            if (obj_grounded(g, hook)) {
                hook->vel_q8.x = (hook->vel_q8.x * 240) >> 8;
            }
        }
    }
    hook->bumpflags = 0;

    rec_i32 r_room = {0, 0, g->pixel_x, g->pixel_y};
    if (!overlap_rec_pnt(r_room, obj_pos_center(hook))) {
        hero_unhook(g, h);
        return;
    }

    rope_update(g, &g->herodata.rope);
    if (hook->state) {
        h->vel_q8 = obj_constrain_to_rope(g, h);
    } else {
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