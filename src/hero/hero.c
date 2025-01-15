// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero.h"
#include "app.h"
#include "game.h"

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

void   hero_do_swimming(g_s *g, obj_s *o, inp_s inp);
void   hero_do_climbing(g_s *g, obj_s *o, inp_s inp);
void   hero_do_crawling(g_s *g, obj_s *o, inp_s inp);
void   hero_do_walking(g_s *g, obj_s *o, inp_s inp);
void   hero_do_ladder(g_s *g, obj_s *o, inp_s inp);
void   hero_do_inair(g_s *g, obj_s *o, inp_s inp);
void   hero_do_dead_on_ground(g_s *g, obj_s *o);
void   hero_do_dead_in_air(g_s *g, obj_s *o);
void   hero_do_dead_in_water(g_s *g, obj_s *o);
i32    hero_try_snap_to_ladder_or_climbwall(g_s *g, obj_s *o);
i32    hero_ladder_or_climbwall_snapdata(g_s *g, obj_s *o, i32 offx, i32 offy,
                                         i32 *dt_snap_x);
bool32 hero_on_valid_ladder_or_climbwall(g_s *g, obj_s *o, i32 offx, i32 offy);

obj_s *hero_create(g_s *g)
{
    obj_s  *o = obj_create(g);
    hero_s *h = (hero_s *)&g->hero;
    o->ID     = OBJID_HERO;
    obj_tag(g, o, OBJ_TAG_HERO);
    o->heap = h;

    o->flags =
        OBJ_FLAG_ACTOR |
        OBJ_FLAG_CLAMP_ROOM_X |
        OBJ_FLAG_KILL_OFFSCREEN |
        OBJ_FLAG_LIGHT |
        OBJ_FLAG_SPRITE;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_SLIDE_Y_NEG;
    o->health_max      = 2;
    o->health          = o->health_max;
    o->render_priority = RENDER_PRIO_HERO;
    o->w               = HERO_WIDTH;
    o->h               = HERO_HEIGHT;
    o->facing          = 1;
    o->n_sprites       = 1;
    o->light_radius    = 100;
    o->light_strength  = 7;
    return o;
}

i32 hero_item_button(g_s *g, inp_s inp, obj_s *o)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);

    if (inps_btn_jp(inp, INP_B)) {
        if (o->rope) {
            hero_action_ungrapple(g, o);
        } else {
            h->b_hold_tick = 1;
            // g->grapple_tick = 1;
        }
        return 1;
    } else if (h->b_hold_tick) {
        if (inps_btn(inp, INP_B)) {
            h->b_hold_tick = u8_adds(h->b_hold_tick, 1);
        } else {
            h->b_hold_tick = 0;
            i32 ang        = -(dpad_x ? dpad_x : o->facing) * 8000;
            hero_action_throw_grapple(g, o, ang, 5000);
        }
        return 1;
    }
    return 0;
}

void hero_check_rope_intact(g_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return;
    hero_s *h = (hero_s *)o->heap;
    rope_s *r = o->rope;
    if (!rope_is_intact(g, r)) {
        grapplinghook_destroy(g, &g->ghook);
    }
}

void hero_on_squish(g_s *g, obj_s *o)
{
}

void hero_hurt(g_s *g, obj_s *o, i32 damage)
{
    hero_s *h = (hero_s *)o->heap;
    if (h->invincibility_ticks || !o->health) return;

    h->health_restore_tick = 0;
    o->health              = u8_subs(o->health, damage);

    if (o->health) {
        h->hurt_ticks          = HERO_HURT_TICKS;
        h->invincibility_ticks = HERO_INVINCIBILITY_TICKS;
    } else {
        h->dead_bounce = 0;
        o->animation   = 0;
        o->bumpflags   = 0;
        o->v_q8.y      = -1800;
        o->flags &= ~OBJ_FLAG_CLAMP_ROOM_Y; // let hero fall through floor
        hero_action_ungrapple(g, o);
        if (g->substate != SUBSTATE_GAMEOVER) {
            gameover_start(g);
        }
    }
}

void hero_start_jump(g_s *g, obj_s *o, i32 ID)
{
    hero_s *h = (hero_s *)o->heap;

    i32 jID = ID;
    if (ID == HERO_JUMP_GROUND && HERO_VX_SPRINT <= abs_i32(o->v_q8.x)) {
        jID = HERO_JUMP_GROUND_BOOSTED;
    }

    hero_jumpvar_s jv  = g_herovar[jID];
    h->jump_index      = jID;
    h->edgeticks       = 0;
    h->jump_btn_buffer = 0;
    h->jumpticks       = (u8)jv.ticks;
    o->v_q8.y          = -jv.vy;
    if (ID == HERO_JUMP_GROUND) {
        snd_play(SNDID_SPEAK, 1.f, 0.5f);
        v2_i32 posc = obj_pos_bottom_center(o);
        posc.x -= 16;
        posc.y -= 32;
        rec_i32 trp = {0, 284, 32, 32};
        spritedecal_create(g, RENDER_PRIO_HERO + 1, 0, posc, TEXID_MISCOBJ,
                           trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
    }
    if (ID == HERO_JUMP_FLY) {
        snd_play(SNDID_WING1, 2.f, rngr_f32(0.8f, 1.2f));
    }
}

i32 hero_get_actual_state(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;

    if (HERO_WATER_THRESHOLD <= water_depth_rec(g, obj_aabb(o))) {
        if (!h->swimming) { // just started swimming
            h->swimming            = 1;
            h->climbing            = 0; // end climbing
            h->ladder              = 0; // end ladder
            h->stomp               = 0;
            h->stomp_landing_ticks = 0;
            h->edgeticks           = 0;
            h->jumpticks           = 0;
            h->jump_btn_buffer     = 0;
            grapplinghook_destroy(g, &g->ghook);
        }
        h->swimming = 1;
        return HERO_ST_WATER;
    }
    h->swimming = 0;

    // if still on ladder, verify
    if (o->health && h->ladder &&
        hero_on_valid_ladder_or_climbwall(g, o, 0, 0))
        return HERO_ST_LADDER;
    h->ladder = 0;

    if (o->health && h->climbing &&
        hero_is_climbing_offs(g, o, o->facing, 0, 0))
        return HERO_ST_CLIMB;
    h->climbing = 0;

    return (obj_grounded(g, o) ? HERO_ST_GROUND : HERO_ST_AIR);
}

void hero_restore_grounded_stuff(g_s *g, obj_s *o)
{
    hero_s *h          = (hero_s *)o->heap;
    h->swimticks       = HERO_SWIM_TICKS;
    h->air_block_ticks = 0;
    h->walljump_tick   = 0;
    h->edgeticks       = 6;
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
    staminarestorer_respawn_all(g, o);
    hero_stamina_add_ui(g, o, 10000);
    if (h->stamina_added == 0) {
        h->jump_ui_may_hide = 1;
    }
}

i32 hero_can_grab(g_s *g, obj_s *o, i32 dirx)
{
    if (!obj_grounded(g, o)) return 0;
    if (!dirx) return 0;

    i32 x  = o->pos.x + (0 < dirx ? o->w : -1);
    i32 y1 = o->pos.y;
    i32 y2 = o->pos.y + o->h - 1;

    for (i32 y = y1; y <= y2; y++) {
        if (map_blocked_pt(g, x, y)) return 0;
    }
    return dirx;
}

void hero_walljump(g_s *g, obj_s *o, i32 dir)
{
    hero_s *h             = (hero_s *)o->heap;
    h->climbing           = 0;
    h->impact_ticks       = 0;
    h->air_block_ticks_og = WALLJUMP_MOM_TICKS;
    h->air_block_ticks    = dir * WALLJUMP_MOM_TICKS;
    o->animation          = 0;
    o->facing             = dir;
    o->v_q8.x             = dir * 700;
    hero_start_jump(g, o, HERO_JUMP_WALL);
}

i32 hero_is_climbing_offs(g_s *g, obj_s *o, i32 facing, i32 dx, i32 dy)
{
    if (!facing) return 0;
    rec_i32 r = {o->pos.x + dx, o->pos.y + dy, o->w, o->h};
    if (!!map_blocked(g, r)) return 0;
    if (obj_grounded(g, o)) return 0;

    i32 x  = dx + (0 < facing ? o->pos.x + o->w : o->pos.x - 1);
    i32 y1 = dy + o->pos.y + 2;
    i32 y2 = dy + o->pos.y + o->h - 1 - 4;

    for (i32 y = y1; y <= y2; y++) {
        i32 r = map_climbable_pt(g, x, y);
        if (r != MAP_CLIMBABLE_SUCCESS)
            return 0;
    }

    return 1;
}

i32 hero_breath_tick(obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    return h->breath_ticks;
}

i32 hero_breath_tick_max(g_s *g)
{
    return (hero_has_upgrade(g, HERO_UPGRADE_DIVE) ? 2500 : 100);
}

i32 hero_swim_frameID(i32 animation)
{
    return ((animation >> 3) % 6);
}

i32 hero_swim_frameID_idle(i32 animation)
{
    return ((animation >> 4) & 7);
}

void hero_on_update(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);
    i32     st     = hero_get_actual_state(g, o);

    if (inps_btn_jp(inp, INP_A)) {
        h->jump_btn_buffer = 6;
    } else if (h->jump_btn_buffer) {
        h->jump_btn_buffer--;
    }

    if (h->jump_ui_may_hide) {
        h->stamina_ui_fade_out = max_i32((i32)h->stamina_ui_fade_out - 1, 0);
    } else {
        h->stamina_ui_fade_out = min_i32((i32)h->stamina_ui_fade_out + 3, STAMINA_UI_TICKS_HIDE);
    }
    if (h->stamina_ui_collected_tick) {
        h->stamina_ui_collected_tick--;
    }
    if (st == HERO_ST_AIR) {
        hero_stamina_update_ui(g, o, (i32)!h->stamina_ui_collected_tick << 7);
    } else {
        h->stamina_ui_collected_tick = 0;
        hero_stamina_update_ui(g, o, g->hero.stamina_upgrades << 7);
    }

    if (o->health && o->health < o->health_max) {
        if (HERO_HEALTH_RESTORE_TICKS <= ++h->health_restore_tick) {
            o->health++;
            h->health_restore_tick = 0;
        }
    } else if (o->health == o->health_max) {
        h->health_restore_tick = 0;
    }

    if (h->gliding) {
        h->gliding--;
    }
    if (h->invincibility_ticks) {
        h->invincibility_ticks--;
    }
    if (h->hurt_ticks) {
        h->hurt_ticks--;
    }

    rope_s *r              = o->rope;
    bool32  rope_stretched = (r && (r->len_max_q4 * 254) <= (rope_len_q4(g, r) << 8));

    // facing
    bool32 facing_locked = o->health == 0 ||
                           st == HERO_ST_CLIMB ||
                           st == HERO_ST_LADDER ||
                           h->skidding ||
                           h->crawl ||
                           h->crouched ||
                           h->grabbing;

    if (dpad_x && !facing_locked) {
        o->facing = dpad_x;
    }

    if ((o->bumpflags & OBJ_BUMP_Y) && !(o->bumpflags & OBJ_BUMP_Y_BOUNDS)) {
        if (o->bumpflags & OBJ_BUMP_Y_NEG) {
            h->jumpticks = 0;
        }

        if (st == HERO_ST_AIR && rope_stretched) {
            obj_vy_q8_mul(o, -64);
        } else if (o->health) {
            h->impact_ticks = min_i32(o->v_q8.y >> 9, 8);

            if (1000 <= o->v_q8.y) {
                f32 vol = (1.f * (f32)o->v_q8.y) / 2000.f;
                snd_play(SNDID_STEP, min_f32(vol, 1.f) * 1.2f, 1.f);
            }

            if (500 <= o->v_q8.y) {
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= 16;
                posc.y -= 32;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO + 1, NULL, posc, TEXID_MISCOBJ,
                                   trp, 15, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            o->v_q8.y = 0;
        } else {
            if (h->dead_bounce) {
                o->v_q8.y = 0;
                if (h->dead_bounce == 1) {
                    o->animation = 0;
                    h->dead_bounce++;
                }
            } else {
                h->dead_bounce++;
                o->v_q8.y = -(o->v_q8.y * 3) / 4;
            }
        }
    }

    if (o->bumpflags & OBJ_BUMP_X) {
        h->sprint = 0;
        if (st == HERO_ST_AIR && rope_stretched) {
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

    if (st != HERO_ST_GROUND && (h->crouched || h->crawl)) {
        hero_try_stand_up(g, o);
    }

    if (h->crouched || h->crawl) {
        assert(o->h == HERO_HEIGHT_CROUCHED);
    } else {
        assert(o->h == HERO_HEIGHT);
    }

    switch (st) {
    case HERO_ST_WATER: {
        if (o->health) {
            hero_do_swimming(g, o, inp);
        } else {
            hero_do_dead_in_water(g, o);
        }
        obj_move_by_v_q8(g, o);
        break;
    }
    case HERO_ST_GROUND: {
        hero_restore_grounded_stuff(g, o);
        o->v_q8.y += HERO_GRAVITY;

        if (o->health) {
            if (dpad_y <= 0) {
                hero_try_stand_up(g, o);
            }

            if (h->crouched || h->crawl) {
                hero_do_crawling(g, o, inp);
            } else {
                hero_do_walking(g, o, inp);
            }
        } else {
            hero_do_dead_on_ground(g, o);
        }
        obj_move_by_v_q8(g, o);
        break;
    }
    case HERO_ST_CLIMB: {
        hero_do_climbing(g, o, inp);
        obj_move_by_v_q8(g, o);
        break;
    }
    case HERO_ST_LADDER: {
        hero_do_ladder(g, o, inp);
        break;
    }
    case HERO_ST_STOMP: {
        h->stomp = u8_adds(h->stomp, 1);

        if (HERO_TICKS_STOMP_INIT <= h->stomp) {
            i32 move_y = min_i32((h->stomp - HERO_TICKS_STOMP_INIT) * 1, 10);
            obj_move(g, o, dpad_x, move_y);
        }
        break;
    }
    case HERO_ST_AIR: {
        if (h->low_grav_ticks) {
            i32 gt0 = pow2_i32(h->low_grav_ticks_0);
            i32 gt1 = pow2_i32(h->low_grav_ticks - h->low_grav_ticks_0);
            o->v_q8.y += lerp_i32(HERO_GRAVITY_LOW, HERO_GRAVITY, gt1, gt0);
            h->low_grav_ticks--;
        } else {
            o->v_q8.y += HERO_GRAVITY;
        }
        if (o->health) {
            hero_do_inair(g, o, inp);
        } else {
            hero_do_dead_in_air(g, o);
        }

        o->v_q8.y = min_i32(o->v_q8.y, 1792);
        obj_move_by_v_q8(g, o);
        break;
    }
    }
}

void hero_leave_and_clear_inair(obj_s *o)
{
    hero_s *h             = (hero_s *)o->heap;
    h->jumpticks          = 0;
    h->jump_btn_buffer    = 0;
    h->walljump_tech_tick = 0;
    h->walljump_tick      = 0;
    h->air_block_ticks    = 0;
    h->air_block_ticks_og = 0;
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
    if (!!map_blocked(g, aabb_stand)) return 0;
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
    h->stamina_ui_collected_tick = 10;
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
    return g->hero.stamina_upgrades * HERO_TICKS_PER_STAMINA_UPGRADE;
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

void hero_action_throw_grapple(g_s *g, obj_s *o, i32 ang_q16, i32 vel)
{
    hero_s *h       = (hero_s *)o->heap;
    v2_i32  vlaunch = grapplinghook_vlaunch_from_angle(ang_q16, vel);
    snd_play(SNDID_HOOK_THROW, 1.f, 1.f);
    v2_i32 center = obj_pos_center(o);

    if (0 < o->v_q8.y) {
        o->v_q8.y >>= 1;
    }
    grapplinghook_create(g, &g->ghook, o, center, vlaunch);
}

bool32 hero_action_ungrapple(g_s *g, obj_s *o)
{
    if (!o->rope) return 0;

    hero_s          *h  = (hero_s *)o->heap;
    grapplinghook_s *gh = &g->ghook;

    bool32 attached = gh->state && gh->state != GRAPPLINGHOOK_FLYING;
    grapplinghook_destroy(g, gh);

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

i32 hero_register_jumped_or_stomped_on(obj_s *ohero, obj_s *o)
{
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumped_or_stomped_on; n++) {
        if (obj_from_obj_handle(h->jumped_or_stomped_on[n]) == o) {
            return 1;
        }
    }
    if (h->n_jumped_or_stomped_on == HERO_NUM_JUMPED_ON) return 0;
    h->jumped_or_stomped_on[h->n_jumped_or_stomped_on++] =
        obj_handle_from_obj(o);
    return 2;
}

i32 hero_register_jumped_on(obj_s *ohero, obj_s *o)
{
    hero_register_jumped_or_stomped_on(ohero, o);
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumped_on; n++) {
        if (obj_from_obj_handle(h->jumped_on[n]) == o) {
            return 1;
        }
    }
    if (h->n_jumped_on == HERO_NUM_JUMPED_ON) return 0;
    h->jumped_on[h->n_jumped_on++] = obj_handle_from_obj(o);
    return 2;
}

i32 hero_register_stomped_on(obj_s *ohero, obj_s *o)
{
    hero_register_jumped_or_stomped_on(ohero, o);
    hero_s *h = (hero_s *)ohero->heap;
    for (i32 n = 0; n < h->n_stomped_on; n++) {
        if (obj_from_obj_handle(h->stomped_on[n]) == o)
            return 1;
    }
    if (h->n_stomped_on == HERO_NUM_JUMPED_ON) return 0;
    h->stomped_on[h->n_stomped_on++] = obj_handle_from_obj(o);
    return 2;
}

hero_interaction_s hero_get_interaction(g_s *g, obj_s *o)
{
    hero_s            *h = (hero_s *)o->heap;
    hero_interaction_s i = {0};

    if (o->rope) {
        i.action = HERO_INTERACTION_UNHOOK;
        return i;
    }

    if (obj_grounded(g, o) && !h->crawl && !h->crouched && o->v_q8.x == 0 &&
        !h->climbing && !h->stomp_landing_ticks && !o->rope) {
        obj_s *interactable = hero_interactable_available(g, o);
        if (interactable) {
            i.action   = HERO_INTERACTION_INTERACT;
            i.interact = obj_handle_from_obj(interactable);
            return i;
        }

        inp_s inp    = inp_cur();
        i32   dpad_x = inps_x(inp);
        if (hero_can_grab(g, o, dpad_x)) {
            i.action = HERO_INTERACTION_GRAB;
            return i;
        }
    }

    return i;
}

void hero_interaction_do(g_s *g, obj_s *o, hero_interaction_s i)
{
    switch (i.action) {
    default: break;
    case HERO_INTERACTION_GRAB: {
        break;
    }
    case HERO_INTERACTION_INTERACT: {

        break;
    }
    case HERO_INTERACTION_UNHOOK: {
        break;
    }
    }
}

void hero_inair_jump(g_s *g, obj_s *o, inp_s inp);

void hero_do_inair(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);

    o->animation++;
    if (h->impact_ticks) {
        h->impact_ticks--;
    }
    if (h->edgeticks) {
        h->edgeticks--;
    }
    if (h->walljump_tick) {
        h->walljump_tick -= sgn_i32(h->walljump_tick);
    }
    if (h->air_block_ticks) {
        h->air_block_ticks -= sgn_i32(h->air_block_ticks);
    }

    i32    ibutton   = hero_item_button(g, inp, o);
    i32    staminap  = hero_stamina_left(g, o);
    bool32 can_stomp = hero_has_upgrade(g, HERO_UPGRADE_STOMP) &&
                       !h->holds_weapon &&
                       !h->b_hold_tick &&
                       !o->rope &&
                       !ibutton;
    bool32 rope_stretched = (o->rope && (o->rope->len_max_q4 * 254) <= (rope_len_q4(g, o->rope) << 8));

    if (rope_stretched) { // swinging
        h->jumpticks     = 0;
        h->walljump_tick = 0;
        v2_i32 rn_curr   = o->ropenode->p;
        v2_i32 rn_next   = ropenode_neighbour(o->rope, o->ropenode)->p;
        v2_i32 dtrope    = v2_sub(rn_next, rn_curr);
        i32    dtrope_s  = sgn_i32(dtrope.x);
        i32    dtrope_a  = abs_i32(dtrope.x);

        if (sgn_i32(o->v_q8.x) != dpad_x) {
            obj_vx_q8_mul(o, 254);
        }

        o->v_q8.x += ((dtrope_s == dpad_x) ? 45 : 10) * dpad_x;
    } else if (can_stomp && inp_btn_jp(INP_DD)) {
        h->stomp  = 1;
        o->v_q8.x = 0;
        o->v_q8.y = 0;
        hero_leave_and_clear_inair(o);
    } else if (inp_btn_jp(INP_DU) && hero_try_snap_to_ladder_or_climbwall(g, o)) {
        // snapped to ladder
        grapplinghook_destroy(g, &g->ghook);
    } else {
        bool32 do_x_movement  = 1;
        bool32 start_climbing = hero_has_upgrade(g, HERO_UPGRADE_CLIMB) &&
                                hero_is_climbing_offs(g, o, dpad_x, 0, 0) &&
                                !o->rope &&
                                !ibutton;

        if (start_climbing) {
            do_x_movement = 0;
            o->animation  = 0;
            if (inps_btn_jp(inp, INP_A)) {
                hero_walljump(g, o, -dpad_x);
            } else {
                h->climbing = 1;
                o->facing   = dpad_x;
                o->v_q8.x   = 0;
                o->v_q8.y   = 0;
                hero_leave_and_clear_inair(o);
                // hero_start_climbing(g, o, dpad_x);
            }
        } else if (!ibutton && !o->rope && h->jump_btn_buffer) {
            hero_inair_jump(g, o, inp);
        } else {
            if (staminap <= 0 && h->jump_index == HERO_JUMP_FLY) {
                h->jumpticks = 0;
            } else if (0 < h->jumpticks) { // dynamic jump height
                if (inps_btn(inp, INP_A)) {
                    hero_jumpvar_s jv = g_herovar[h->jump_index];
                    i32            t0 = pow_i32(jv.ticks, 4);
                    i32            ti = pow_i32(h->jumpticks, 4) - t0;
                    i32            ch = jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
                    o->v_q8.y -= ch;
                    if (h->jump_index == HERO_JUMP_FLY) {
                        hero_stamina_modify(g, o, -64);
                        if (h->jumpticks == jv.ticks - 8) {
                            snd_play(SNDID_WING1, 1.8f, 0.5f);
                        }
                    }
                    h->jumpticks--;
                } else { // cut jump short
                    obj_vy_q8_mul(o, 128);
                    h->jumpticks = 0;
                    h->walljump_tick >>= 1;
                }
            }
        }

        if (do_x_movement) {
            i32 vs = sgn_i32(o->v_q8.x);
            i32 va = abs_i32(o->v_q8.x);
            i32 ax = 0;

            if (vs == 0) {
                ax = 200;
            } else if (dpad_x == +vs && va < HERO_VX_WALK) { // press same dir as velocity
                ax = lerp_i32(200, 20, va, HERO_VX_WALK);
                ax = min_i32(ax, HERO_VX_WALK - va);
            } else if (dpad_x == -vs) {
                ax = min_i32(100, va);
            }

            if (h->air_block_ticks && sgn_i32(h->air_block_ticks) == -dpad_x) {
                i32 i0 = h->air_block_ticks_og - abs_i32(h->air_block_ticks);
                i32 i1 = h->air_block_ticks_og;
                ax     = lerp_i32(0, ax, pow2_i32(i0), pow2_i32(i1));
            }

            if (dpad_x != vs) {
                obj_vx_q8_mul(o, 235);
            }
            o->v_q8.x += ax * dpad_x;
        }
    }
}

void hero_inair_jump(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h           = (hero_s *)o->heap;
    i32     dpad_x      = inps_x(inp);
    i32     dpad_y      = inps_y(inp);
    rope_s *r           = o->rope;
    bool32  usinghook   = r != 0;
    bool32  ground_jump = (0 < h->edgeticks);
    for (i32 y = 0; y < 6; y++) {
        rec_i32 rr = {o->pos.x, o->pos.y + o->h + y, o->w, 1};
        v2_i32  pp = {0, y + 1};
        if (!map_blocked(g, rr) && obj_grounded_at_offs(g, o, pp)) {
            ground_jump = 1;
            break;
        }
    }

    bool32 jump_wall = 0;
    for (i32 n = 0; n < 4; n++) {
        if (hero_is_climbing_offs(g, o, -dpad_x, -dpad_x * n, 0)) {
            jump_wall = 1;
            break;
        }
    }

    bool32 jump_midair = !usinghook && // not hooked
                         !h->jumpticks &&
                         !h->stomp &&
                         hero_has_upgrade(g, HERO_UPGRADE_FLY) &&
                         hero_stamina_left(g, o);

    if (ground_jump) {
        hero_restore_grounded_stuff(g, o);
        hero_start_jump(g, o, HERO_JUMP_GROUND);
    } else if (jump_wall) {
        hero_walljump(g, o, dpad_x);
    } else if (jump_midair) {
        hero_stamina_modify(g, o, -192);
        hero_start_jump(g, o, HERO_JUMP_FLY);

        rec_i32 rwind = {0, 0, 64, 64};
        v2_i32  dcpos = obj_pos_center(o);
        dcpos.x -= 32;
        dcpos.y += 0;
        i32 flip = rngr_i32(0, 1) ? 0 : SPR_FLIP_X;

        spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, dcpos, TEXID_WINDGUSH, rwind, 18, 6, flip);
    }
}

void hero_do_swimming(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);
    obj_vx_q8_mul(o, 240);
    h->jump_ui_may_hide = 1;
    i32    water_depth  = water_depth_rec(g, obj_aabb(o));
    bool32 submerged    = HERO_HEIGHT <= water_depth;
    if (h->diving && !submerged) {
        h->diving = 0;
    }

    // animation and sfx
    if ((dpad_x == 0 || inps_xp(inp) == 0) && dpad_x != inps_xp(inp)) {
        o->animation = 0;
    } else {
        o->animation++;
    }

    if (h->swimsfx_delay) {
        h->swimsfx_delay--;
    } else {
        if (dpad_x) {
            i32 fid1 = hero_swim_frameID(o->animation);
            i32 fid2 = hero_swim_frameID(o->animation + 1);
            if (o->animation == 0 || (fid2 == 0 && fid1 != fid2)) {
                h->swimsfx_delay = 20;
                snd_play(SNDID_WATER_SWIM_1, 0.30f, rngr_f32(0.9f, 1.1f));
            }
        } else {
            i32 fid1 = hero_swim_frameID_idle(o->animation);
            i32 fid2 = hero_swim_frameID_idle(o->animation + 1);
            if (o->animation == 0 || ((fid2 == 0 || fid2 == 4) && fid1 != fid2)) {
                snd_play(SNDID_WATER_SWIM_2, 0.15f, rngr_f32(0.9f, 1.1f));
            }
        }
    }

    if (h->diving && hero_has_upgrade(g, HERO_UPGRADE_DIVE)) {
        obj_vy_q8_mul(o, 230);
        if (dpad_y) {
            i32 i0 = (dpad_y == sgn_i32(o->v_q8.y) ? abs_i32(o->v_q8.y) : 0);
            i32 ay = (max_i32(512 - i0, 0) * 128) >> 8;
            o->v_q8.y += ay * dpad_y;
        }
    } else {
        o->v_q8.y += HERO_GRAVITY;
        obj_vy_q8_mul(o, 220);
        if (!hero_has_upgrade(g, HERO_UPGRADE_SWIM) && 0 < h->swimticks) {
            h->swimticks--; // swim ticks are reset when grounded later on
        }
        bool32 can_swim = hero_has_upgrade(g, HERO_UPGRADE_SWIM) ||
                          0 < h->swimticks;
        if (1) {
            i32 ch = lerp_i32(HERO_GRAVITY,
                              HERO_GRAVITY + 20,
                              water_depth - HERO_WATER_THRESHOLD,
                              (o->h - HERO_WATER_THRESHOLD));

            o->v_q8.y -= ch;
            if (HERO_WATER_THRESHOLD <= water_depth &&
                water_depth < HERO_HEIGHT) {
                i32 vmin  = (water_depth - HERO_WATER_THRESHOLD) << 8;
                o->v_q8.y = max_i32(o->v_q8.y, -vmin);
            }
        } else {
            o->v_q8.y -= min_i32(5 + water_depth, 40);
        }

        if (hero_has_upgrade(g, HERO_UPGRADE_DIVE) && 0 < inps_y(inp)) {
            obj_move(g, o, 0, +10);
            o->v_q8.y = +1000;
            h->diving = 1;
        } else if (inps_btn_jp(inp, INP_A)) {
            // see if we can jump
            i32 tx1 = max_i32((o->pos.x + 0) >> 4, 0);
            i32 ty1 = max_i32((o->pos.y - 16) >> 4, 0);
            i32 ty2 = min_i32((o->pos.y + 12) >> 4, g->tiles_y - 1);
            i32 tx2 = min_i32((o->pos.x + o->w - 1) >> 4, g->tiles_x - 1);

            for (i32 yy = ty1; yy <= ty2; yy++) {
                for (i32 xx = tx1; xx <= tx2; xx++) {
                    i32 i = xx + yy * g->tiles_x;
                    if (g->tiles[i].collision ||
                        (g->tiles[i].type & TILE_WATER_MASK))
                        continue;
                    obj_move(g, o, 0, -12);
                    hero_start_jump(g, o, HERO_JUMP_WATER);
                    snd_play(SNDID_WATER_OUT_OF, 0.5f, 1.f);
                    particle_desc_s prt = {0};
                    {
                        v2_i32 prt_p = v2_shl(obj_pos_center(o), 8);
                        prt_p.y -= 10 << 8;
                        prt.p.p_q8      = prt_p;
                        prt.p.v_q8.x    = o->v_q8.x;
                        prt.p.v_q8.y    = -500;
                        prt.p.a_q8.y    = 30;
                        prt.p.size      = 2;
                        prt.p.ticks_max = 30;
                        prt.ticksr      = 10;
                        prt.pr_q8.x     = 2000;
                        prt.pr_q8.y     = 1000;
                        prt.vr_q8.x     = 100;
                        prt.vr_q8.y     = 100;
                        prt.ar_q8.y     = 5;
                        prt.sizer       = 1;
                        prt.p.gfx       = PARTICLE_GFX_CIR;
                        prt.p.col       = GFX_COL_WHITE;
                        particles_spawn(g, prt, 30);
                        prt.p.col = GFX_COL_BLACK;
                        particles_spawn(g, prt, 30 >> 1);
                    }

                    goto BREAK_JUMP;
                }
            }
        BREAK_JUMP:;
        }
    }

    if (dpad_x != sgn_i32(o->v_q8.x)) {
        o->v_q8.x /= 2;
    }
    if (dpad_x) {
        i32 i0 = (dpad_x == sgn_i32(o->v_q8.x) ? abs_i32(o->v_q8.x) : 0);
        i32 ax = (max_i32(512 - i0, 0) * 32) >> 8;
        o->v_q8.x += ax * dpad_x;
    }

    if (h->diving) {
        i32 breath_tm   = hero_breath_tick_max(g);
        h->breath_ticks = min_i32(h->breath_ticks + 1, breath_tm);
        if (breath_tm <= h->breath_ticks) {
            // hero_kill(g, o);
        }
    }
}

void hero_do_climbing(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);
    i32     sta    = hero_stamina_modify(g, o, -4);

    if (inps_btn_jp(inp, INP_A)) {
        hero_walljump(g, o, -o->facing);
    } else if (dpad_y && sta) {
        i32 N = 3;
        if (dpad_y < 0) {
            hero_stamina_modify(g, o, -24);
            N = 2;
            o->animation += 2;
        } else {
            o->animation = 0;
        }

        i32 n_moved = 0;
        for (i32 n = 0; n < N; n++) {
            rec_i32 r = {o->pos.x, o->pos.y + dpad_y, o->w, o->h};
            if (!!map_blocked(g, r) ||
                !hero_is_climbing_offs(g, o, o->facing, 0, dpad_y))
                break;
            obj_move(g, o, 0, dpad_y);
            n_moved++;
        }

        if (!n_moved) {
            h->climbing     = 0;
            h->impact_ticks = 0;
            if (dpad_y < 0) {
                o->v_q8.y = -1100;
            } else {
                o->v_q8.y = +256;
            }
        }
    } else if (!sta) {
        o->v_q8.y += 20;
        o->v_q8.y = min_i32(o->v_q8.y, 256);
        obj_move_by_q8(g, o, 0, o->v_q8.y);
    }

    o->linked_solid = obj_handle_from_obj(0);
    if (h->climbing) {
        for (obj_each(g, it)) {
            if (!(it->flags & OBJ_FLAG_SOLID)) continue;
            rec_i32 r = o->facing == 1 ? obj_rec_right(o) : obj_rec_left(o);
            if (overlap_rec(r, obj_aabb(it))) {
                o->linked_solid = obj_handle_from_obj(it);
            }
        }
    }
}

void hero_do_crawling(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);

    if (0 < dpad_y && inps_btn_jp(inp, INP_A)) {
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
        h->jump_btn_buffer = 0;
        obj_move(g, o, 0, 1);
        o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    } else if (h->crouched < HERO_CROUCHED_MAX_TICKS) {
        h->crouched++;
        o->v_q8.x = 0;
    } else {
        if (!h->crawl && dpad_x) {
            h->crawl = dpad_x;
        }
        o->v_q8.x = dpad_x * HERO_VX_CRAWL;
    }
}

void hero_do_walking(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h               = (hero_s *)o->heap;
    i32     dpad_x          = inps_x(inp);
    i32     dpad_y          = inps_y(inp);
    rope_s *r               = o->rope;
    bool32  can_start_crawl = !inps_btn(inp, INP_B) &&
                             !o->rope;

    if (o->v_prev_q8.x == 0 && o->v_q8.x != 0) {
        o->animation = 0;
    }

    if (h->stomp_landing_ticks) {
        h->stomp_landing_ticks++;
        if (HERO_STOMP_LANDING_TICKS <= h->stomp_landing_ticks) {
            h->stomp_landing_ticks = 0;
        }
    } else if (0 < dpad_y && !r) { // start crawling
        o->pos.y += HERO_HEIGHT - HERO_HEIGHT_CROUCHED;
        o->h              = HERO_HEIGHT_CROUCHED;
        h->crouched       = 1;
        h->crouch_standup = 0;
        h->sprint         = 0;
        h->b_hold_tick    = 0;
    } else if (h->jump_btn_buffer) {
        h->impact_ticks = 2;
        hero_start_jump(g, o, HERO_JUMP_GROUND);
    } else if (inp_btn_jp(INP_DU) && hero_try_snap_to_ladder_or_climbwall(g, o)) {
        // snapped to ladder
        h->b_hold_tick = 0;
        obj_move(g, o, 0, -1); // move upwards so player isn't grounded anymore
    } else {
        i32    ibutton    = hero_item_button(g, inp, o);
        bool32 can_sprint = !ibutton & !o->rope;

        if (h->crouch_standup) {
            h->crouch_standup--;
            obj_vx_q8_mul(o, 220);
        }

        if (!can_sprint) {
            h->sprint = 0;
        } else if (!h->sprint) {
            if (h->sprint_dtap) {
                h->sprint_dtap--;
            }

            if (inps_btn_jp(inp, INP_DR) || inps_btn_jp(inp, INP_DL)) {
                if (h->sprint_dtap) {
                    h->sprint      = 1;
                    h->sprint_dtap = 0;
                    if (dpad_x == -1) {
                        o->v_q8.x = min_i32(o->v_q8.x, -HERO_VX_WALK);
                    } else if (dpad_x == +1) {
                        o->v_q8.x = max_i32(o->v_q8.x, +HERO_VX_WALK);
                    }
                } else {
                    h->sprint_dtap = 12;
                }
            }
        }

        i32 vs = sgn_i32(o->v_q8.x);
        i32 va = abs_i32(o->v_q8.x);
        i32 ax = 0;

        if (h->skidding) {
            h->skidding--;
            obj_vx_q8_mul(o, 256 <= va ? 220 : 128);
        } else if (dpad_x != vs) {
            if (HERO_VX_SPRINT <= va) {
                o->facing   = vs;
                h->skidding = 15;
            } else {
                obj_vx_q8_mul(o, 128);
            }
        }

        if (vs == 0 && h->skidding < 6) {
            ax = 200;
        } else if (dpad_x == +vs) { // press same dir as velocity
            if (0) {
            } else if (va < HERO_VX_WALK) {
                ax = lerp_i32(200, 20, va, HERO_VX_WALK);
            } else if (va < HERO_VX_SPRINT && h->sprint) {
                ax = min_i32(20, HERO_VX_SPRINT - va);
            }
        } else if (dpad_x == -vs && h->skidding < 6) {
            ax = min_i32(70, va);
        }

        o->v_q8.x += ax * dpad_x;

        if (HERO_VX_MAX_GROUND < abs_i32(o->v_q8.x)) {
            obj_vx_q8_mul(o, 252);
            if (abs_i32(o->v_q8.x) < HERO_VX_MAX_GROUND) {
                o->v_q8.x = sgn_i32(o->v_q8.x) * HERO_VX_MAX_GROUND;
            }
        }

        if (o->rope) {
            ropenode_s *rn = ropenode_neighbour(o->rope, o->ropenode);
            if (260 <= rope_stretch_q8(g, o->rope) &&
                sgn_i32(rn->p.x - o->pos.x) == -sgn_i32(o->v_q8.x)) {
                o->v_q8.x = 0;
            }
        }
    }
}

void hero_do_dead_on_ground(g_s *g, obj_s *o)
{
    obj_vx_q8_mul(o, 192);
    if (o->v_q8.x == 0) {
        o->animation++;
    }
}

void hero_do_dead_in_air(g_s *g, obj_s *o)
{
    o->animation++;
}

void hero_do_dead_in_water(g_s *g, obj_s *o)
{
}

bool32 hero_step_on_ladder(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    rec_i32 r = {o->pos.x + sx, o->pos.y + sy, o->w, o->h};
    if (!map_blocked(g, r) &&
        hero_on_valid_ladder_or_climbwall(g, o, sx, sy)) {
        o->pos.x += sx;
        o->pos.y += sy;
        if (obj_grounded(g, o)) {
            hero_s *h = (hero_s *)o->heap;
            h->ladder = 0;
            return 0;
        } else {
            return 1;
        }
    }
    return 0;
}

void hero_do_ladder(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);

    if (inps_btn_jp(inp, INP_A)) { // jump
        h->ladder = 0;
        hero_start_jump(g, o, 0);
        o->v_q8.x = dpad_x * 200;
    } else if (inps_btn_jp(inp, INP_B)) {
        h->ladder = 0;
    } else {
        i32 moved_x = 0;
        i32 moved_y = 0;
        i32 dy      = dpad_y * 2;
        i32 dx      = h->ladder == HERO_LADDER_WALL ? dpad_x * 2 : 0;

        if (dx && dy) { // move by timer to avoid camera 1/0/1/0... jitter
            if (o->timer & 1) {
                dx = 0;
                dy = 0;
            }
            o->timer++;
        } else {
            o->timer = 0;
        }

        for (i32 m = abs_i32(dy); m; m--) {
            if (!hero_step_on_ladder(g, o, 0, dpad_y)) break;
            moved_y++;
        }

        for (i32 m = abs_i32(dx); m; m--) {
            if (!hero_step_on_ladder(g, o, dpad_x, 0)) break;
            moved_x++;
        }
        if (moved_y) { // up/down and diagonal
            o->animation -= (dpad_y * moved_y * (moved_x ? 3 : 2)) / 2;
        } else { // left/right movement only
            o->animation -= moved_x;
        }
    }
}

i32 hero_try_snap_to_ladder_or_climbwall(g_s *g, obj_s *o)
{
    hero_s *h       = (hero_s *)o->heap;
    i32     snap_to = 0;
    i32     dt_x    = 0;
    i32     t       = hero_ladder_or_climbwall_snapdata(g, o, 0, 0, &dt_x);

    switch (t) {
    case HERO_LADDER_WALL:
        snap_to = HERO_LADDER_WALL;
        break;
    case HERO_LADDER_VERTICAL:
        if (dt_x == 0) {
            snap_to = HERO_LADDER_VERTICAL;
        } else if (0 < dt_x) {
            rec_i32 rsnap = {o->pos.x, o->pos.y,
                             o->w + dt_x, o->h};
            if (!map_blocked(g, rsnap)) {
                o->pos.x += dt_x;
                snap_to = HERO_LADDER_VERTICAL;
            }
        } else if (dt_x < 0) {
            rec_i32 rsnap = {o->pos.x + dt_x, o->pos.y,
                             o->w - dt_x, o->h};
            if (!map_blocked(g, rsnap)) {
                o->pos.x += dt_x;
                snap_to = HERO_LADDER_VERTICAL;
            }
        }
        break;
    }

    if (snap_to) {
        h->ladder          = snap_to;
        o->v_q8.x          = 0;
        o->v_q8.y          = 0;
        h->jumpticks       = 0;
        h->edgeticks       = 0;
        h->gliding         = 0;
        h->jump_btn_buffer = 0;
    }
    return snap_to;
}

// returns HERO_LADDER_VERTICAL or HERO_LADDER_WALL, and fills dt_snap_x
// with the amount the player would have to shift on x to be considered in
// a valid ladder or climbwall position
i32 hero_ladder_or_climbwall_snapdata(g_s *g, obj_s *o, i32 offx, i32 offy,
                                      i32 *dt_snap_x)
{
    // left and right most bounds of player (inclusive)
    i32 px1 = o->pos.x + offx;
    i32 px2 = o->pos.x + offx + o->w - 1;

    // test ladder
    // one of the two ladder sensor points has to be above a ladder tile
    // snap distance = the distance the player would have to shift on x
    // to be on a valid ladder
    // x sensor positions ladder, width 16 == tile width
    {
        i32     x1 = px1 + (o->w - 16) / 2;
        i32     x2 = px2 - (o->w - 16) / 2;
        v2_i32  p1 = {x1, o->pos.y + offy + o->h / 2}; // sensor left
        v2_i32  p2 = {x2, o->pos.y + offy + o->h / 2}; // sensor right
        tile_s *t1 = tile_map_at_pos(g, p1);
        tile_s *t2 = tile_map_at_pos(g, p2);
        if (t1 && (t1->collision == TILE_LADDER ||
                   t1->collision == TILE_LADDER_ONE_WAY)) {
            // have to shift left
            *dt_snap_x = -(x1 & 15);
            return HERO_LADDER_VERTICAL;
        }
        if (t2 && (t2->collision == TILE_LADDER ||
                   t2->collision == TILE_LADDER_ONE_WAY)) {
            // have to shift right
            *dt_snap_x = 15 - (x2 & 15);
            return HERO_LADDER_VERTICAL;
        }
    }

    // test climbwall
    //  // both wall sensor points have to be above a climbwall tile to be valid
    // no snapping but "smaller" snap distance instead
    // x sensor positions climbwall, width 8 == tile idth
    {
        i32     x1 = px1 + (o->w - 8) / 2;
        i32     x2 = px2 - (o->w - 8) / 2;
        v2_i32  p1 = {x1, o->pos.y + offy + o->h / 2}; // sensor left
        v2_i32  p2 = {x2, o->pos.y + offy + o->h / 2}; // sensor right
        tile_s *t1 = tile_map_at_pos(g, p1);
        tile_s *t2 = tile_map_at_pos(g, p2);
        if (t1 && t2 &&
            t1->collision == TILE_CLIMBWALL && t2->collision == TILE_CLIMBWALL) {
            *dt_snap_x = 0;
            return HERO_LADDER_WALL;
        }
    }
    return 0;
}

bool32 hero_on_valid_ladder_or_climbwall(g_s *g, obj_s *o, i32 offx, i32 offy)
{
    i32 dt_x = 0;
    i32 t    = hero_ladder_or_climbwall_snapdata(g, o, offx, offy, &dt_x);
    return (t && dt_x == 0);
}

bool32 hero_has_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    return (h->upgrades & ((u32)1 << ID));
}

void hero_add_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    h->upgrades |= (u32)1 << ID;
    pltf_log("# ADD UPGRADE: %i\n", ID);
}

void hero_rem_upgrade(g_s *g, i32 ID)
{
    hero_s *h = &g->hero;
    h->upgrades &= ~((u32)1 << ID);
    pltf_log("# DEL UPGRADE: %i\n", ID);
}

void hero_set_name(g_s *g, const char *name)
{
    hero_s *h = &g->hero;
    str_cpy((char *)h->name, name);
}

char *hero_get_name(g_s *g)
{
    hero_s *h = &g->hero;
    return (char *)&h->name[0];
}

void hero_inv_add(g_s *g, i32 ID, i32 n)
{
}

void hero_inv_rem(g_s *g, i32 ID, i32 n)
{
}

i32 hero_inv_count_of(g_s *g, i32 ID)
{

    return 0;
}

void hero_coins_change(g_s *g, i32 n)
{
    if (n == 0) return;
    hero_s *h  = &g->hero;
    i32     ct = h->coins + g->coins_added + n;
    if (ct < 0) return;

    if (g->coins_added == 0 || g->coins_added_ticks) {
        g->coins_added_ticks = 100;
    }
    g->coins_added += n;
}

i32 hero_coins(g_s *g)
{
    hero_s *h = &g->hero;
    i32     c = h->coins + g->coins_added;
    assert(0 <= c);
    return c;
}

i32 saveID_put(g_s *g, i32 ID)
{
    if (ID == 0) return 0;

    if (saveID_has(g, ID)) return 2;
    if (g->n_saveIDs == NUM_SAVEIDS) return 0;
    g->saveIDs[g->n_saveIDs++] = ID;
    return 1;
}

bool32 saveID_has(g_s *g, i32 ID)
{
    if (ID == 0) return 0;

    for (i32 n = 0; n < g->n_saveIDs; n++) {
        if (g->saveIDs[n] == ID) return 1;
    }
    return 0;
}