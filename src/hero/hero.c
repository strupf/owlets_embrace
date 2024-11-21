// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "app.h"
#include "game.h"
#include "hero_hook.h"

typedef struct {
    i16 index;
    i16 ticks;
} hero_jump_s;

const hero_jumpvar_s g_herovar[NUM_HERO_JUMP] = {
    {800, 30, 100, 30}, // out of water
    {1100, 35, 65, 30}, // ground
    {1500, 40, 50, 20}, // ground boosted
    {600, 50, 140, 20}, // fly
    {1100, 35, 65, 20}, // wall
};

void hero_update_ground(g_s *g, obj_s *o);
void hero_update_climb(g_s *g, obj_s *o);
void hero_update_ladder(g_s *g, obj_s *o);
void hero_update_swimming(g_s *g, obj_s *o);
void hero_update_air(g_s *g, obj_s *o, bool32 rope_stretched);

obj_s *hero_create(g_s *g)
{
    obj_s  *o = obj_create(g);
    hero_s *h = (hero_s *)&g->hero_mem;
    o->ID     = OBJ_ID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->heap = h;

    o->flags = // OBJ_FLAG_MOVER |
        OBJ_FLAG_CLAMP_ROOM_X |
        OBJ_FLAG_KILL_OFFSCREEN |
        // OBJ_FLAG_RENDER_AABB |
        OBJ_FLAG_LIGHT |
        OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_MAP |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLIDE_Y_NEG;
    o->health_max      = 2;
    o->health          = o->health_max;
    o->render_priority = RENDER_PRIO_HERO;
    o->w               = 12;
    o->h               = HERO_HEIGHT;
    o->facing          = 1;
    o->n_sprites       = 1;
    o->light_radius    = 100;
    o->light_strength  = 7;
    return o;
}

void hero_check_rope_intact(g_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    hero_s *h     = (hero_s *)o->heap;
    obj_s  *ohook = obj_from_obj_handle(h->hook);
    if (!ohook) return;

    rope_s *r = o->rope;
    if (!rope_intact(g, r)) {
        hook_destroy(g, o, ohook);
    }
}

void hero_on_squish(g_s *g, obj_s *o)
{
}

void hero_hurt(g_s *g, obj_s *o, i32 damage)
{
    hero_s *h = (hero_s *)o->heap;
    if (h->invincibility_ticks) return;
    o->health              = max_i32((i32)o->health - damage, 0);
    h->health_restore_tick = 0;
    if (o->health) {
        h->invincibility_ticks = ticks_from_ms(1000);
    } else {
        hero_kill(g, o);
    }
}

void hero_kill(g_s *g, obj_s *o)
{
    o->health = 0;
    o->flags &= ~OBJ_FLAG_CLAMP_ROOM_Y; // let hero fall through floor
    hero_action_ungrapple(g, o);
    if (g->substate != SUBSTATE_GAMEOVER) {
        gameover_start(g);
    }
}

i32 hero_determine_state(g_s *g, obj_s *o, hero_s *h)
{
    if (o->health == 0) return HERO_STATE_DEAD;

    i32 water_depth = water_depth_rec(g, obj_aabb(o));
    if (18 <= water_depth) return HERO_STATE_SWIMMING;

    if (obj_grounded(g, o) && 0 <= o->v_q8.y) return HERO_STATE_GROUND;

    if (h->ladder_type) {
        if (tile_map_ladder_overlaps_rec(g, obj_aabb(o), NULL) &&
            h->ladderx == o->pos.x) {
            return HERO_STATE_LADDER;
        } else {
            h->ladder_type = 0;
        }
    }

    if (h->climbing) {
        return HERO_STATE_CLIMB;
    }

    return HERO_STATE_AIR;
}

void hero_start_jump(g_s *g, obj_s *o, i32 ID)
{
    hero_s *h = (hero_s *)o->heap;

    i32 jID = ID;
    if (ID == HERO_JUMP_GROUND && HERO_VX_SPRINT <= abs_i32(o->v_q8.x)) {
        jID = HERO_JUMP_GROUND_BOOSTED;
    }

#if 0
    if (pltf_sdl_key(SDL_SCANCODE_SPACE) && ID == HERO_JUMP_GROUND) {
        jID = NUM_HERO_JUMP;
    }
#endif

    hero_jumpvar_s jv  = g_herovar[jID];
    h->jump_index      = jID;
    h->edgeticks       = 0;
    h->jump_btn_buffer = 0;
    o->v_q8.y          = -jv.vy;
    h->jumpticks       = (u8)jv.ticks;
    if (ID == HERO_JUMP_GROUND) {
        snd_play(SNDID_SPEAK, 1.f, 0.5f);
        v2_i32 posc = obj_pos_bottom_center(o);
        posc.x -= 16;
        posc.y -= 32;
        rec_i32 trp = {0, 284, 32, 32};
        spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                           trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
    }
    if (ID == HERO_JUMP_FLY) {
        snd_play(SNDID_WING1, 2.f, rngr_f32(0.8f, 1.2f));
    }
}

void hero_on_update(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;

    h->interactable = obj_handle_from_obj(NULL);
    h->pushing      = 0;
    hero_handle_input(g, o);

    v2_i16 v_og   = o->v_q8;
    i32    state  = hero_determine_state(g, o, h);
    i32    dpad_x = inp_x();
    i32    dpad_y = inp_y();
    if (state == HERO_STATE_DEAD) {
        dpad_x = 0;
        dpad_y = 0;
    }

    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;

    if (h->climbing) {
        if (state != HERO_STATE_CLIMB || !hero_is_climbing(g, o, o->facing)) {
            h->climbing = 0;
            state       = hero_determine_state(g, o, h);
        }
    }

    if (h->invincibility_ticks) {
        h->invincibility_ticks--;
    }

    if (h->jump_ui_may_hide) {
        h->stamina_ui_fade_out = max_i32((i32)h->stamina_ui_fade_out - 1, 0);
    } else {
        h->stamina_ui_fade_out = min_i32((i32)h->stamina_ui_fade_out + 3, STAMINA_UI_TICKS_HIDE);
    }

    if (h->stamina_ui_collected_tick) {
        h->stamina_ui_collected_tick--;
    }

    if (1 <= o->health && o->health < o->health_max) {
        if (HERO_HEALTH_RESTORE_TICKS <= ++h->health_restore_tick) {
            o->health++;
            h->health_restore_tick = 0;
        }
    } else if (o->health == o->health_max) {
        h->health_restore_tick = 0;
    }

    if (state == HERO_STATE_AIR) {
        hero_stamina_update_ui(g, o, !h->stamina_ui_collected_tick);
    } else {
        h->stamina_ui_collected_tick = 0;
        hero_stamina_update_ui(g, o, g->save.stamina_upgrades << 6);
    }

    if (state == HERO_STATE_SWIMMING && h->statep == HERO_STATE_AIR) {
        h->swimsfx_delay = 0;
        o->animation     = 0;

        i32 vsplash_max = 2500;
        i32 v_clamped   = clamp_i32(o->v_q8.y, 0, vsplash_max);
        f32 vol         = 0.7f * (f32)v_clamped / (f32)vsplash_max;
        snd_play(SNDID_WATER_SPLASH_BIG, vol, rngr_f32(0.9f, 1.1f));

        particle_desc_s prt = {0};
        {
            i32 prt_vy = ease_lin(200, 600, v_clamped, vsplash_max);

            prt.p.p_q8      = v2_shl(obj_pos_center(o), 8);
            prt.p.v_q8.x    = 0;
            prt.p.v_q8.y    = -rngr_i32(prt_vy, prt_vy + 150);
            prt.p.a_q8.y    = 20;
            prt.p.size      = 2;
            prt.p.ticks_max = ease_lin(20, 50, v_clamped, vsplash_max);
            prt.ticksr      = 10;
            prt.pr_q8.x     = 3000;
            prt.pr_q8.y     = 1000;
            prt.vr_q8.x     = 150;
            prt.vr_q8.y     = 200;
            prt.ar_q8.y     = 5;
            prt.sizer       = 1;
            prt.p.gfx       = PARTICLE_GFX_CIR;
            prt.p.col       = GFX_COL_WHITE;
            i32 prt_n       = ease_in_quad(0, 80, v_clamped, vsplash_max);
            particles_spawn(g, prt, prt_n);
            prt.p.col = GFX_COL_BLACK;
            particles_spawn(g, prt, prt_n >> 1);
        }
    }

    if (h->attack_tick) {
        h->attack_tick++;
        if (HERO_ATTACK_TICKS <= h->attack_tick) {
            h->attack_tick = 0;
        }
    }

    if (h->jump_btn_buffer) {
        h->jump_btn_buffer--;
    }
    if (h->impact_ticks) {
        h->impact_ticks--;
        if (state == HERO_STATE_AIR) {
            if (!h->jumpticks) {
                h->impact_ticks = 0;
            }
        }
    }

    if (state != HERO_STATE_GROUND) {
        h->skidding       = 0;
        h->dropped_weapon = 0;
        h->sprint_dtap    = 0;
        if (hero_try_stand_up(g, o)) {
            h->crouch_standup = 0;
        }
    }

    if (dpad_x == 0) {
        h->sprint = 0;
    }

    if (h->skidding) {
        h->skidding--;
    }

    if ((state != HERO_STATE_DEAD) && (state != HERO_STATE_SWIMMING || !h->diving)) {
        i32 dt          = hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 100 : 5;
        h->breath_ticks = max_i32(h->breath_ticks - dt, 0);
    }

    if (state != HERO_STATE_AIR && state != HERO_STATE_GROUND) {
        h->edgeticks = 0;
    }

    if (state != HERO_STATE_AIR) {
        h->low_grav_ticks = 0;
        h->walljump_tick  = 0;
        h->jumpticks      = 0;
    }

    rope_s *r              = o->rope;
    bool32  rope_stretched = (r && (r->len_max_q4 * 254) <= (rope_len_q4(g, r) << 8));
    bool32  facing_locked  = 0;
    facing_locked |= h->attack_tick;
    facing_locked |= h->climbing;
    facing_locked |= h->skidding;
    facing_locked |= h->crawl;
    facing_locked |= h->crouched;
    facing_locked |= (state == HERO_STATE_AIR && rope_stretched);

    if (!facing_locked) {
        if (dpad_x) {
            o->facing = dpad_x;
        }
    }

    if ((o->bumpflags & OBJ_BUMP_Y) && !(o->bumpflags & OBJ_BUMP_Y_BOUNDS)) {
        if (o->bumpflags & OBJ_BUMP_Y_NEG) {
            h->jumpticks = 0;
        }

        if (state == HERO_STATE_AIR && rope_stretched) {
            obj_vy_q8_mul(o, -64);
        } else if (!(o->bumpflags & OBJ_BUMP_ON_HEAD)) {
            if (1000 <= o->v_q8.y) {
                f32 vol = (1.f * (f32)o->v_q8.y) / 2000.f;
                snd_play(SNDID_STEP, min_f32(vol, 1.f) * 1.2f, 1.f);
            }
            h->impact_ticks = min_i32(o->v_q8.y >> 9, 8);
            if (500 <= o->v_q8.y) {

                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= 16;
                posc.y -= 32;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                                   trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            o->v_q8.y = 0;
        }
    }

    if (o->bumpflags & OBJ_BUMP_X) {
        h->sprint = 0;
        if (state == HERO_STATE_AIR && rope_stretched) {
            if (dpad_x == sgn_i32(o->v_q8.x) || abs_i32(o->v_q8.x) < 600) {
                o->v_q8.x = 0;
            } else {
                obj_vx_q8_mul(o, -64);
            }
        } else {
            o->v_q8.x = 0;
        }
    }
    o->bumpflags = 0;

    switch (state) {
    default: h->holds_weapon = 0; break;
    case HERO_STATE_GROUND:
    case HERO_STATE_AIR: {
        if (!inp_action_jp(INP_DU) && !inp_action_jp(INP_DD)) break;
        if (hero_try_snap_to_ladder(g, o, dpad_y)) {
            state = HERO_STATE_LADDER;
        }
        break;
    }
    }

    switch (state) {
    case HERO_STATE_NULL: break;
    case HERO_STATE_GROUND:
        hero_update_ground(g, o);
        break;
    case HERO_STATE_AIR:
        hero_update_air(g, o, rope_stretched);
        break;
    case HERO_STATE_SWIMMING:
        hero_update_swimming(g, o);
        break;
    case HERO_STATE_LADDER:
        hero_update_ladder(g, o);
        break;
    case HERO_STATE_CLIMB:
        hero_update_climb(g, o);
        break;
    case HERO_STATE_DEAD: {
        break;
    }
    }

    obj_move_by_v_q8(g, o);
    h->statep = state;
}

bool32 hero_try_stand_up(g_s *g, obj_s *o)
{
    if (o->h != HERO_HEIGHT_CROUCHED) return 0;
    hero_s *h = (hero_s *)o->heap;
    assert(!o->rope);
    rec_i32 aabb_stand = {o->pos.x,
                          o->pos.y - (HERO_HEIGHT - HERO_HEIGHT_CROUCHED),
                          o->w,
                          HERO_HEIGHT};
    if (!map_traversable(g, aabb_stand)) return 0;
    obj_move(g, o, 0, -(HERO_HEIGHT - HERO_HEIGHT_CROUCHED));
    o->h              = HERO_HEIGHT;
    h->crouched       = 0;
    h->crawl          = 0;
    h->crouch_standup = HERO_CROUCHED_MAX_TICKS;
    return 1;
}

void hero_stamina_update_ui(g_s *g, obj_s *o, i32 amount)
{
    hero_s *h = (hero_s *)o->heap;
    if (!h->stamina_added) return;

    i32 d = min_i32(h->stamina_added, amount);
    h->stamina_added -= d;
    h->stamina += d;
}

i32 hero_stamina_modify(g_s *g, obj_s *o, i32 dt)
{
    hero_s *h    = (hero_s *)o->heap;
    i32     stap = hero_stamina_left(g, o);

    if (0 < dt) { // add stamina
        i32 d = min_i32(dt, hero_stamina_max(g, o) - stap);
        h->stamina += d;
    } else if (dt < 0) { // remove stamina
        i32 d = -dt;
        if (h->stamina) {
            i32 x = min_i32(h->stamina, d);
            h->stamina -= x;
            d -= x;
        }
        h->stamina_added = max_i32(0, h->stamina_added - d);
    }
    i32 sta = hero_stamina_left(g, o);
    if (sta < stap) {
        h->jump_ui_may_hide = 0;
    }
    return sta;
}

void hero_stamina_add_ui(g_s *g, obj_s *o, i32 dt)
{
    hero_s *h  = (hero_s *)o->heap;
    i32     ft = hero_stamina_left(g, o);
    h->stamina_added += min_i32(dt, hero_stamina_max(g, o) - ft);
    h->stamina_ui_collected_tick = 20;
}

i32 hero_stamina_left(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return (h->stamina + h->stamina_added);
}

i32 hero_stamina_ui_full(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina;
}

i32 hero_stamina_ui_added(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->stamina_added;
}

i32 hero_stamina_max(g_s *g, obj_s *o)
{
    return g->save.stamina_upgrades * HERO_TICKS_PER_STAMINA_UPGRADE;
}

bool32 hero_present_and_alive(g_s *g, obj_s **o)
{
    obj_s *ot = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ot) return 0;
    if (o) {
        *o = ot;
    }
    return (ot->health);
}

v2_i32 hero_hook_aim_dir(hero_s *h)
{
    v2_i32 aim = {-sin_q16(h->hook_aim) >> 8,
                  +cos_q16(h->hook_aim) >> 8};
    return aim;
}

void hero_action_throw_grapple(g_s *g, obj_s *o)
{
    hero_s *h       = (hero_s *)o->heap;
    v2_i32  vlaunch = {0};
    if (h->aim_mode) {
        vlaunch = hero_hook_aim_dir(h);
        vlaunch = v2_setlen(vlaunch, 2000);
    } else {
        vlaunch.x = inp_x() * 2650;
        vlaunch.y = inp_y() * 2650;
    }
    snd_play(SNDID_HOOK_THROW, 1.f, 1.f);

    v2_i32 center = obj_pos_center(o);

    g->rope.active = 1;
    rope_s *rope   = &g->rope;
    obj_s  *hook   = hook_create(g, rope, center, vlaunch);
    h->hook        = obj_handle_from_obj(hook);
    o->rope        = rope;
    o->ropenode    = rope->head;

    v2_i32 pcurr = v2_shl(center, 8);

    for (i32 n = 0; n < ROPE_VERLET_N; n++) {
        rope->ropept[n].p = pcurr;
        // "hint" the direction to the verlet sim
        i32 k             = ROPE_VERLET_N - 1 - n;
#if HERO_HOOK_OLD_AIM
        rope->ropept[n].pp.x = pcurr.x - ((dirx << 12) * k) / ROPE_VERLET_N;
        rope->ropept[n].pp.y = pcurr.y - ((diry << 12) * k) / ROPE_VERLET_N;
#else
        rope->ropept[n].pp.x = pcurr.x - ((vlaunch.x << 1) * k) / ROPE_VERLET_N;
        rope->ropept[n].pp.y = pcurr.y - ((vlaunch.y << 1) * k) / ROPE_VERLET_N;
#endif
    }
}

bool32 hero_action_ungrapple(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    if (!obj_handle_valid(h->hook)) return 0;

    obj_s *ohook;
    if (!obj_try_from_obj_handle(h->hook, &ohook)) return 0;
    bool32 attached = hook_is_attached(ohook);
    hook_destroy(g, o, ohook);

    if (obj_grounded(g, o) || !attached) return 1;

    i32 mulx = 260;
    i32 muly = 260;

    if (abs_i32(o->v_q8.x) < 600) {
        mulx = 280;
    }

    if (abs_i32(o->v_q8.y) < 600) {
        muly = 270;
    }

    v2_i16 vadd = {sgn_i32(o->v_q8.x) * 200,
                   o->v_q8.y < 0 ? -200 : 0};

    obj_v_q8_mul(o, mulx, muly);
    o->v_q8 = v2_i16_add(o->v_q8, vadd);

#define LOW_G_TICKS_VEL 4000
    i32 low_g_v         = min_i32(abs_i32(o->v_q8.y), LOW_G_TICKS_VEL);
    h->low_grav_ticks_0 = HERO_LOW_GRAV_TICKS;
    h->low_grav_ticks   = lerp_i32(0, HERO_LOW_GRAV_TICKS, low_g_v, LOW_G_TICKS_VEL);

    return 1;
}

void hero_action_attack(g_s *g, obj_s *o)
{
    hero_s *h      = (hero_s *)o->heap;
    h->attack_tick = 1;

    if (obj_grounded(g, o)) {
        o->v_q8.x = 0;
    }

    hitbox_s hb = {0};

    switch (h->attack_tick) {
    case 0: // slash
        hb.r.y        = o->pos.y - 5;
        hb.r.w        = 60;
        hb.r.h        = 32;
        hb.force_q8.x = o->facing * 800;
        hb.force_q8.y = -400;
        hb.damage     = 2;
        snd_play(SNDID_WOOSH_2, 1.f, rngr_f32(1.1f, 1.3f));
        break;
    case 1: // stab
        hb.r.w        = 48;
        hb.r.h        = 24;
        hb.r.y        = o->pos.y + 1;
        hb.force_q8.x = o->facing * 600;
        hb.force_q8.y = -200;
        hb.damage     = 1;
        snd_play(SNDID_WOOSH_1, 0.7f, rngr_f32(0.8f, 1.2f));
        break;
    }

    if (o->facing == 1) {
        hb.r.x = o->pos.x + o->w;
    } else {
        hb.r.x = o->pos.x - hb.r.w;
    }

    pltf_debugr(hb.r.x + g->cam_prev.x, hb.r.y + g->cam_prev.y, hb.r.w, hb.r.h, 255, 100, 0, 30);
    bool32 did_hit = obj_game_player_attackbox(g, hb);

#if 0
    // slash sprite
    rec_i32 rslash = {0, 1024 + 64, 64, 64};
    v2_i32  dcpos  = {-10, -40};
    i32     flip   = 0;
    if (o->facing < 0) {
        flip    = SPR_FLIP_X;
        dcpos.x = -40;
    }
    if (h->attack_flipflop) {
        rslash.x += 512;
    }

    spritedecal_create(g, 0xFFFF, o, dcpos, TEXID_HERO, rslash, 18, 8, flip);
#endif
}

void hero_on_stomped(g_s *g, obj_s *o)
{
}

void hero_stomped_ground(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    if (h->stomp) {
        h->stomp = 0;
        snd_play(SNDID_STOMP, 1.f, 1.f);

        particle_desc_s prt = {0};
        {
            prt.p.p_q8      = v2_shl(obj_pos_center(o), 8);
            prt.p.v_q8.x    = 0;
            prt.p.v_q8.y    = -300;
            prt.p.a_q8.y    = 20;
            prt.p.size      = 3;
            prt.p.ticks_max = 20;
            prt.ticksr      = 10;
            prt.pr_q8.x     = 5000;
            prt.pr_q8.y     = 1000;
            prt.vr_q8.x     = 400;
            prt.vr_q8.y     = 200;
            prt.ar_q8.y     = 5;
            prt.sizer       = 1;
            prt.p.gfx       = PARTICLE_GFX_CIR;
            prt.p.col       = GFX_COL_WHITE;
            particles_spawn(g, prt, 15);
            prt.p.col = GFX_COL_BLACK;
            particles_spawn(g, prt, 15);
        }
    }
}

bool32 hero_stomping(obj_s *o)
{
    return ((hero_s *)o->heap)->stomp;
}

i32 hero_register_jumped_on(obj_s *ohero, obj_s *o)
{
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumped_on; n++) {
        if (obj_from_obj_handle(h->jumped_on[n]) == o)
            return 1;
    }
    if (h->n_jumped_on == HERO_NUM_JUMPED_ON) return 0;
    h->jumped_on[h->n_jumped_on++] = obj_handle_from_obj(o);
    return 2;
}

i32 hero_register_stomped_on(obj_s *ohero, obj_s *o)
{
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_stomped_on; n++) {
        if (obj_from_obj_handle(h->stomped_on[n]) == o)
            return 1;
    }
    if (h->n_stomped_on == HERO_NUM_JUMPED_ON) return 0;
    h->stomped_on[h->n_stomped_on++] = obj_handle_from_obj(o);
    return 2;
}