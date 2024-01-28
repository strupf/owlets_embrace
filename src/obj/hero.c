// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "game.h"

#define HERO_ROPE_LEN_MIN   500
#define HERO_ROPE_LEN_SHORT 2000
#define HERO_ROPE_LEN_LONG  4000
#define HERO_GRAVITY        60

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
    {700, 25, 60, 30}, // lo ground
    {825, 25, 60, 30}, // hi ground
    {200, 25, 250, 0}, // airj 1
    {150, 25, 180, 0}, // airj 2
    {100, 25, 130, 0}, // airj 3
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
//
static obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8);
static void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);

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
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_ONE_WAY_PLAT;

    o->health_max   = g->herodata.health;
    o->health       = o->health_max;
    o->n_sprites    = 2;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->gravity_q8.y = HERO_GRAVITY;
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
    sprite->mode   = obj_invincible_frame(o) ? SPR_MODE_INV : 0;

    hero_s *h    = (hero_s *)o->mem;
    o->n_sprites = 1;
    if (h->windgush_ticks) {
        h->windgush_ticks++;
        if (h->windgush_ticks == 25) {
            h->windgush_ticks = 0;
        } else {
            o->n_sprites          = 2;
            sprite_simple_s *spr3 = &o->sprites[1];
            spr3->trec            = asset_texrec(TEXID_WINDGUSH, 0, 0, 64, 64);

            spr3->trec.r.x = 64 * ((h->windgush_ticks * 6) / 25);
            v2_i32 pdt     = v2_sub(h->jumped_at, o->pos);
            spr3->offs.x   = pdt.x - 30;
            spr3->offs.y   = pdt.y - 5;
        }
    }
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
        g->substate      = GAME_SUBSTATE_HERO_DIE;
        g->substate_tick = 0;
        obj_delete(g, o);
        // kill
    }
}

int hero_determine_state(game_s *g, obj_s *o, hero_s *h)
{
    bool32 grounded = obj_grounded(g, o) && 0 <= o->vel_q8.y;
    if (grounded) return HERO_STATE_GROUND;

    int water_depth = water_depth_rec(g, obj_aabb(o));
    if (5 < water_depth) return HERO_STATE_SWIMMING;

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

    int state  = hero_determine_state(g, o, hero);
    int dpad_x = inp_dpad_x();
    int dpad_y = inp_dpad_y();

    hero->jump_btn_buffer--;
    hero->edgeticks--;
    o->facing_locked = 0;
    if (dpad_x != 0 && !o->facing_locked) {
        o->facing = dpad_x;
    }

    if (o->bumpflags & OBJ_BUMPED_Y) {
        if (1000.f <= o->vel_q8.y) {
            f32 vol = (.7f * (f32)o->vel_q8.y) / 2000.f;
            snd_play_ext(SNDID_STEP, min_f(vol, 0.7f), 1.f);
        }
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;

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
        hero->onladder = 1;
        hero->ladderx  = rladder.x;
        state          = HERO_STATE_LADDER;
        break;
    }
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
        hero_update_ground(g, o, hero);
        break;
    case HERO_STATE_AIR:
        hero_update_air(g, o, hero);
        break;
    case HERO_STATE_SWIMMING:
        hero_update_swimming(g, o, hero);
        break;
    case HERO_STATE_LADDER:
        hero_update_ladder(g, o, hero);
        break;
    }

    o->drag_q8.x = 256;
    o->drag_q8.y = 256;
    o->flags |= OBJ_FLAG_MOVER;
    switch (state) {
    case HERO_STATE_GROUND:
        o->drag_q8.x = 250;
        break;
    case HERO_STATE_AIR:
        o->drag_q8.x = 255;
        break;
    case HERO_STATE_SWIMMING:
        o->drag_q8.x = 240;
        break;
    case HERO_STATE_LADDER:
        o->flags &= ~OBJ_FLAG_MOVER;
        break;
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
    snd_play_ext(SNDID_SPEAK, 0.5f, 0.5f);
    h->jump_index      = ID;
    hero_jumpvar_s jv  = g_herovar[h->jump_index];
    h->edgeticks       = 0;
    h->jump_btn_buffer = 0;
    o->vel_q8.y        = -jv.vy;
    h->jumpticks       = jv.ticks;
}

static void hero_item_use(game_s *g, obj_s *o, hero_s *h)
{
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

    o->tomove.y = inp_dpad_y() << 1;
}

static void hero_update_swimming(game_s *g, obj_s *o, hero_s *h)
{
    int water_depth = water_depth_rec(g, obj_aabb(o));
    o->drag_q8.y    = 240;
    o->vel_q8.y -= 25;
    h->swimticks--; // swim ticks are reset when grounded later on
    if (0 < h->swimticks) {
        int j0 = min_i(water_depth, 60);
        int j1 = 60;
        o->vel_q8.y -= (j0 * 80) / j1;
    } else {
        sys_printf("no swim ticks left\n");
        o->vel_q8.y -= water_depth;
    }

    if (water_depth < 5 && inp_pressed(INP_A)) {
        hero_start_jump(g, o, h, 0);
    }
}

static void hero_update_ground(game_s *g, obj_s *o, hero_s *h)
{
    bool32 can_swim = hero_has_upgrade(&g->herodata, HERO_UPGRADE_SWIM);
    h->edgeticks    = 6;

    if (inp_just_pressed(INP_DPAD_U)) {
        v2_i32 posc         = obj_pos_center(o);
        obj_s *interactable = obj_closest_interactable(g, posc);

        if (interactable) {
            obj_interact(g, interactable);
        }
    }

    if (inp_just_pressed(INP_A) || 0 < h->jump_btn_buffer) {
        bool32 hjump = hero_has_upgrade(&g->herodata, HERO_UPGRADE_HIGH_JUMP);
        hero_start_jump(g, o, h, hjump ? 1 : 0);
    }

    int dpad_x = inp_dpad_x();

    if (dpad_x != sgn_i(o->vel_q8.x)) {
        o->vel_q8.x /= 2;
    }
    if (dpad_x) {
        int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        int ax = (max_i(512 - i0, 0) * 256) >> 8;
        o->vel_q8.x += ax * dpad_x;
    }
}

static void hero_update_air(game_s *g, obj_s *o, hero_s *h)
{
    // dynamic jump height

    if (0 < h->jumpticks && !inp_pressed(INP_A)) {
        h->jumpticks = 0;
    }

    if (0 < h->jumpticks) {
        hero_jumpvar_s jv = g_herovar[h->jump_index];
        u32            t0 = pow_u32(jv.ticks, 4);
        u32            ti = pow_u32(h->jumpticks, 4) - t0;
        o->vel_q8.y -= jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
    }

    h->jumpticks--;
    bool32 usehook     = 0;
    bool32 jump_midair = !usehook &&             // not hooked
                         h->edgeticks <= 0 &&    // jump in air?
                         0 < h->airjumps_left && // air jumps left?
                         h->jumpticks <= -15 &&  // wait some ticks after last jump
                         inp_pressed(INP_A);

    if (jump_midair) {
        int jumpindex = 2 + g->herodata.n_airjumps - h->airjumps_left;
        h->airjumps_left--;
        hero_start_jump(g, o, h, jumpindex);
    }

    if (inp_just_pressed(INP_A)) {
        h->jump_btn_buffer = 8;
    }

    int dpad_x = inp_dpad_x();
    if (dpad_x) {
        int i0 = (dpad_x == sgn_i(o->vel_q8.x) ? abs_i(o->vel_q8.x) : 0);
        int ax = (max_i(512 - i0, 0) * 32) >> 8;
        o->vel_q8.x += ax * dpad_x;
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

static void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
    obj_delete(g, ohook);
    ohero->ropenode = NULL;
    ohero->rope     = NULL;
    ohook->rope     = NULL;
    ohook->ropenode = NULL;
    g->n_ropes      = 0;
}