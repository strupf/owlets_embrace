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

void   hero_inair_jump(g_s *g, obj_s *o, inp_s inp);
void   hero_leave_and_clear_inair(obj_s *o);
void   hero_restore_grounded_stuff(g_s *g, obj_s *o);
i32    hero_get_actual_state(g_s *g, obj_s *o);
void   hero_start_jump(g_s *g, obj_s *o, i32 ID);
void   hero_hurt(g_s *g, obj_s *o, i32 damage);
void   hero_on_squish(g_s *g, obj_s *o);
bool32 hero_try_stand_up(g_s *g, obj_s *o);
void   hero_start_jump(g_s *g, obj_s *o, i32 ID);

obj_s *hero_create(g_s *g)
{
    obj_s  *o     = obj_create(g);
    hero_s *h     = (hero_s *)&g->hero;
    o->ID         = OBJID_HERO;
    o->on_animate = hero_on_animate;
    o->on_squish  = hero_on_squish;
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

void hero_abort_grapple(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;
    grapplinghook_destroy(g, &g->ghook);
    h->b_hold_tick = 0;
}

// return 0 to skip the hero update afterwards
i32 hero_special_input(g_s *g, obj_s *o, inp_s inp)
{
    hero_s            *h        = (hero_s *)o->heap;
    i32                dpad_x   = inps_x(inp);
    i32                dpad_y   = inps_y(inp);
    i32                st       = hero_get_actual_state(g, o);
    bool32             grounded = obj_grounded(g, o);
    rope_s            *r        = o->rope;
    hero_interaction_s i        = hero_get_interaction(g, o);
    if (h->grabbing && i.action != HERO_INTERACTION_GRAB) {
        hero_ungrab(g, o);
    }
    switch (i.action) {
    default: break;
    case HERO_INTERACTION_NONE: {

        break;
    }
    case HERO_INTERACTION_GRAB: {
        if (!inps_btn_jp(inp, INP_B)) break;

        obj_s *k       = obj_from_obj_handle(i.interact);
        h->grabbing    = o->facing;
        h->obj_grabbed = i.interact;
        o->animation   = 0;
        if (k && k->on_grab) {
            k->on_grab(g, k);
        }
        return 0;
    }
    case HERO_INTERACTION_INTERACT: {
        if (!inps_btn_jp(inp, INP_DU)) break;

        obj_s *oi = obj_from_obj_handle(i.interact);
        if (!oi) break;

        oi->on_interact(g, oi);
        return 0;
    }
    case HERO_INTERACTION_UNHOOK: {
        break;
    }
    }

    bool32 can_stomp = st == HERO_ST_AIR &&
                       hero_has_upgrade(g, HERO_UPGRADE_STOMP) &&
                       !h->holds_spear &&
                       !h->b_hold_tick &&
                       !h->stomp &&
                       !o->rope &&
                       0 < dpad_y;
    if (can_stomp && inps_btn_jp(inp, INP_A)) {
        h->stomp  = 1;
        o->v_q8.x = 0;
        o->v_q8.y = 0;
        hero_leave_and_clear_inair(o);
        return 0;
    }

    bool32 can_snap_to_ladder = (st == HERO_ST_GROUND || st == HERO_ST_AIR) &&
                                !h->stomp &&
                                !h->ladder &&
                                !h->grabbing;
    if (can_snap_to_ladder && inps_btn_jp(inp, INP_DU) &&
        hero_try_snap_to_ladder_or_climbwall(g, o)) {
        grapplinghook_destroy(g, &g->ghook);
        h->b_hold_tick = 0;

        if (grounded) {
            obj_move(g, o, 0, -1); // move upwards so player isn't grounded anymore -> removes ladder state
        }
        return 0;
    }

    bool32 can_spin_attack = (st == HERO_ST_GROUND || st == HERO_ST_AIR) &&
                             !h->spinattack &&
                             !h->stomp &&
                             !h->ladder &&
                             !h->crouched &&
                             !o->rope &&
                             !h->grabbing;
    if (can_spin_attack && inps_btn_jp(inp, INP_SHAKE)) {
        h->spinattack      = 1;
        h->jumpticks       = 0;
        h->walljump_tick   = 0;
        h->air_block_ticks = 0;
        o->v_q8.y          = 0;
        return 0;
    }

    bool32 can_swap = st == HERO_ST_GROUND;
    if (h->holds_spear) {
        if (inps_btn_jp(inp, INP_B)) {
            h->b_hold_tick     = 1;
            h->attack_flipflop = 1 - h->attack_flipflop;
            h->attack_tick     = 1;

            if (can_swap) {
                ui_itemswap_start(g);
            }

            if (obj_grounded(g, o)) {
                o->v_q8.x    = 0;
                h->attack_ID = HERO_ATTACK_GROUND;
            } else {
                h->attack_ID = HERO_ATTACK_AIR;
                h->jumpticks = 0;
                o->v_q8.y >>= 1;
            }
        } else if (h->b_hold_tick) {
            if (inps_btn(inp, INP_B)) {
                h->b_hold_tick = u8_adds(h->b_hold_tick, 1);

                if (can_swap) {
                    ui_itemswap_set_progress(g, h->b_hold_tick);
                }
            } else {
                if (can_swap) {
                    ui_itemswap_exit(g);
                }

                h->b_hold_tick = 0;
            }
        }
    } else {
        if (inps_btn_jp(inp, INP_B)) {
            h->b_hold_tick = 1;
            if (o->rope) {
                hero_action_ungrapple(g, o);
                h->ungrappled = 1;
            } else {
                h->ungrappled      = 0;
                h->attack_tick     = 0;
                h->attack_ID       = 0;
                h->attack_flipflop = 1 - h->attack_flipflop;
            }
            if (can_swap) {
                ui_itemswap_start(g);
            }
        } else if (h->b_hold_tick) {
            if (inps_btn(inp, INP_B)) {
                h->b_hold_tick = u8_adds(h->b_hold_tick, 1);
                if (can_swap) {
                    ui_itemswap_set_progress(g, h->b_hold_tick);
                }
            } else {
                if (can_swap) {
                    ui_itemswap_exit(g);
                }

                if (h->b_hold_tick < UI_ITEMSWAP_TICKS_POP_UP && !h->ungrappled) {
                    //     0 = upwards
                    // 32768 = downwards
                    i32 ang = 0;
                    i32 dx  = dpad_x;
                    i32 dy  = dpad_y;
                    if (dpad_x == 0 && dpad_y == 0) {
                        dx = o->facing;
                        dy = -1;
                    }

                    if (dx && dy) { // diagonal up/down
                        ang = -dx * (0 < dy ? 20000 : 8000);
                    } else if (dx == 0 && dy) { // up/down
                        ang = 0 < dy ? 32768 : 0;
                    } else if (dx && dy == 0) { // sideways
                        ang = -dx * 12000;
                    }

                    hero_action_throw_grapple(g, o, ang, 5000);
                }
                h->b_hold_tick = 0;
            }
        }
    }

    if (!can_swap) {
        ui_itemswap_exit(g);
    }
    if (can_swap && g->ui_itemswap.started && UI_ITEMSWAP_TICKS <= g->ui_itemswap.progress) {
        ui_itemswap_exit(g);
        h->holds_spear = 1 - h->holds_spear;
        h->b_hold_tick = 0;
    }

    h->crank_acc_q16 += inps_crankdt_q16(inp);
    i32 dtl = h->crank_acc_q16 >> 7;
    h->crank_acc_q16 &= (1 << 7) - 1;
    if (o->rope && g->ghook.state != GRAPPLINGHOOK_FLYING) {
        o->rope->len_max_q4 =
            clamp_i32(o->rope->len_max_q4 + dtl,
                      300, 4000);
    }

    return 1;
}

void hero_on_update(g_s *g, obj_s *o, inp_s inp)
{
    if (hero_special_input(g, o, inp) == 0) {
        return;
    }

    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);
    i32     st     = hero_get_actual_state(g, o);

    if (st != HERO_ST_GROUND) {
        ui_itemswap_exit(g);
    }

    if (st == HERO_ST_WATER && h->state_prev != HERO_ST_WATER) {
        v2_i32 splashpos = obj_pos_center(o);

        for (i32 n = 0; n < g->n_fluid_areas; n++) {
            fluid_area_s *fa  = &g->fluid_areas[n];
            rec_i32       rfa = {fa->x, fa->y, fa->w, fa->h};
            if (overlap_rec(obj_aabb(o), rfa)) {
                fluid_area_impact(fa, splashpos.x - fa->x, 12, o->v_q8.y >> 2,
                                  FLUID_AREA_IMPACT_FLAT);
            }
        }

        i32 particleID = PARTICLE_EMIT_ID_HERO_WATER_SPLASH;
        if (1000 <= o->v_q8.y) {
            particleID = PARTICLE_EMIT_ID_HERO_WATER_SPLASH_BIG;
        }
        particle_emit_ID(g, particleID, splashpos);
    }

    bool32 might_jump = !h->spinattack && !h->attack_tick;
    if (h->impact_ticks) {
        h->impact_ticks--;
    }
    if (h->spinattack) {
        h->spinattack++;
        if (HERO_TICKS_SPIN_ATTACK <= h->spinattack) {
            h->sprint     = 1;
            h->spinattack = -50;
        }
    }
    if (h->attack_tick) {
        h->attack_tick++;

        i32 tick2 = 0;
        switch (h->attack_ID) {
        case HERO_ATTACK_GROUND: {
            tick2 = HERO_ATTACK_TICKS;
            break;
        }
        case HERO_ATTACK_AIR: {
            tick2 = HERO_ATTACK_TICKS_AIR;
            break;
        }
        }
        if (tick2 <= h->attack_tick) {
            h->attack_tick = 0;
            h->attack_ID   = 0;
        }
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
    if (h->squish) {
        h->squish = u8_adds(h->squish, 1);
        if (h->squish & 1) {
            o->render_priority = RENDER_PRIO_INFRONT_TERRAIN_LAYER;
        } else {
            o->render_priority = RENDER_PRIO_HERO;
        }
        g->objrender_dirty = 1;
    }

    ui_itemswap_update(g);

    if (h->jump_ui_may_hide) {
        h->stamina_ui_fade_out = max_i32((i32)h->stamina_ui_fade_out - 1, 0);
    } else {
        h->stamina_ui_fade_out = min_i32((i32)h->stamina_ui_fade_out + 3, STAMINA_UI_TICKS_HIDE);
    }
    if (h->stamina_ui_collected_tick) {
        h->stamina_ui_collected_tick--;
    }
    if (st == HERO_ST_AIR) {
        hero_stamina_update_ui(o, (i32)!h->stamina_ui_collected_tick << 7);
    } else {
        h->stamina_ui_collected_tick = 0;
        hero_stamina_update_ui(o, h->stamina_upgrades << 7);
    }

    if (o->health && o->health < o->health_max) {
        if (HERO_HEALTH_RESTORE_TICKS <= ++h->health_restore_tick) {
            o->health++;
            h->health_restore_tick = 0;
        }
    } else if (o->health == o->health_max) {
        h->health_restore_tick = 0;
    }

    rope_s *r              = o->rope;
    bool32  rope_stretched = (r && (r->len_max_q4 * 254) <= (rope_len_q4(g, r) << 8));

    // facing
    bool32 facing_locked = o->health == 0 ||
                           st == HERO_ST_CLIMB ||
                           st == HERO_ST_LADDER ||
                           h->skidding ||
                           h->crouched ||
                           h->attack_tick ||
                           h->spinattack ||
                           rope_stretched ||
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
            if (1000 <= o->v_q8.y) {
                f32 vol = (1.f * (f32)o->v_q8.y) / 2000.f;
                snd_play(SNDID_STEP, min_f32(vol, 1.f) * 1.2f, 1.f);
            }
            if (500 <= o->v_q8.y) {
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.y -= 2;
                i32 pID = (1500 <= o->v_q8.y ? PARTICLE_EMIT_ID_HERO_LAND_HARD
                                             : PARTICLE_EMIT_ID_HERO_LAND);
                particle_emit_ID(g, pID, posc);
            }
            if (250 <= o->v_q8.y) {
                h->impact_ticks = min_i32(o->v_q8.y >> 8, 8) + 2;
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

    if (st != HERO_ST_GROUND && h->crouched) {
        hero_try_stand_up(g, o);
    }

    if (h->crouched) {
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

            if (h->crouched) {
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
        h->stomp = i8_adds(h->stomp, 1);

        if (HERO_TICKS_STOMP_INIT <= h->stomp) {
            i32 move_y = min_i32((h->stomp - HERO_TICKS_STOMP_INIT) * 1, 10);
            obj_move(g, o, dpad_x, move_y);
        }
        break;
    }
    case HERO_ST_AIR: {
        i32 grav = HERO_GRAVITY;

        if (h->low_grav_ticks) {
            i32 gt0 = pow2_i32(h->low_grav_ticks_0);
            i32 gt1 = pow2_i32(h->low_grav_ticks - h->low_grav_ticks_0);
            grav    = lerp_i32(HERO_GRAVITY_LOW, HERO_GRAVITY, gt1, gt0);
            h->low_grav_ticks--;
        }
        if (h->spinattack) {
            grav >>= 1;
        }

        o->v_q8.y += grav;
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

    h->state_prev = h->state_curr;
    h->state_curr = st;
}

void hero_on_squish(g_s *g, obj_s *o)
{
    if (o->health) {
        hero_s *h = (hero_s *)o->heap;
        hero_action_ungrapple(g, o);
        g->freeze_tick = 10;
        h->squish      = 1;
        o->health      = 0;
        o->v_q8.x      = 0;
        o->v_q8.y      = 0;
        o->flags &= ~OBJ_FLAG_ACTOR;

        v2_i32 p = obj_pos_bottom_center(o);
        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_SQUISH, p);

        if (g->substate != SUBSTATE_GAMEOVER) {
            gameover_start(g);
        }
    }
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

    hero_jumpvar_s jv = g_herovar[jID];
    h->jump_index     = jID;
    h->edgeticks      = 0;
    h->skidding       = 0;
    h->attack_tick    = 0;
    h->attack_ID      = 0;
    h->jumpticks      = (u8)jv.ticks;
    o->v_q8.y         = -jv.vy;
    if (ID == HERO_JUMP_GROUND) {
        snd_play(SNDID_JUMP, 1.f, 1.f);
        v2_i32 prtp = obj_pos_bottom_center(o);
        prtp.y -= 4;
        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP, prtp);
    }
    if (ID == HERO_JUMP_FLY) {
        snd_play(SNDID_WING1, 2.f, rngr_f32(0.8f, 1.2f));
    }
}

i32 hero_get_actual_state(g_s *g, obj_s *o)
{
    hero_s *h = (hero_s *)o->heap;

    i32 water_depth = water_depth_rec(g, obj_aabb(o));
    if (HERO_WATER_THRESHOLD <= water_depth) {
        if (!h->swimming) { // just started swimming
            h->swimming    = SWIMMING_SURFACE;
            h->climbing    = 0; // end climbing
            h->ladder      = 0; // end ladder
            h->stomp       = 0;
            h->edgeticks   = 0;
            h->jumpticks   = 0;
            h->attack_tick = 0;
            h->attack_ID   = 0;
            h->spinattack  = 0;
            grapplinghook_destroy(g, &g->ghook);
        }
        h->swimming = SWIMMING_SURFACE;
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

    if (h->stomp) {
        return HERO_ST_STOMP;
    }

    if (obj_grounded(g, o)) {
        if (h->attack_tick && h->attack_ID == HERO_ATTACK_AIR) {
            h->attack_tick = 0;
            h->attack_ID   = 0;
        }
        return HERO_ST_GROUND;
    }

    return (HERO_ST_AIR);
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
    }
    staminarestorer_respawn_all(g, o);
    hero_stamina_add_ui(o, 10000);
    if (h->stamina_added == 0) {
        h->jump_ui_may_hide = 1;
    }
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
    h->crouch_standup = HERO_CROUCHED_MAX_TICKS;
    return 1;
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

    hero_s          *h        = (hero_s *)o->heap;
    grapplinghook_s *gh       = &g->ghook;
    bool32           attached = gh->state && gh->state != GRAPPLINGHOOK_FLYING;
    grapplinghook_destroy(g, gh);

    if (obj_grounded(g, o) || !attached) return 1;

    i32 mulx = 260;
    i32 muly = 265;

    if (abs_i32(o->v_q8.x) < 600) {
        mulx = 280;
    }
    if (abs_i32(o->v_q8.y) < 600) {
        muly = 270;
    }

    obj_v_q8_mul(o, mulx, muly);
    o->v_q8.x += 200 * sgn_i32(o->v_q8.x);
    o->v_q8.y += o->v_q8.y < 0 ? -200 : 0;

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
    }
}

bool32 hero_stomping(obj_s *o)
{
    return ((hero_s *)o->heap)->stomp;
}

hero_interaction_s hero_get_interaction(g_s *g, obj_s *o)
{
    hero_s            *h      = (hero_s *)o->heap;
    inp_s              inp    = inp_cur();
    i32                dpad_x = inps_x(inp);
    i32                st     = hero_get_actual_state(g, o);
    hero_interaction_s i      = {0};

    bool32 can_interact = st == HERO_ST_GROUND &&
                          !h->crouched &&
                          !h->grabbing &&
                          !h->climbing &&
                          !h->spinattack &&
                          !h->attack_tick &&
                          !h->stomp &&
                          !o->rope;
    bool32 can_grab = st == HERO_ST_GROUND &&
                      !h->crouched &&
                      !h->climbing &&
                      !h->spinattack &&
                      !h->attack_tick &&
                      !h->stomp &&
                      !o->rope;

    if (o->rope) {
        can_interact = 0;
        can_grab     = 0;
        i.action     = HERO_INTERACTION_UNHOOK;
    }

    if (can_interact) {
        obj_s *interactable = hero_interactable_available(g, o);
        if (interactable) {
            can_grab   = 0;
            i.action   = HERO_INTERACTION_INTERACT;
            i.interact = obj_handle_from_obj(interactable);
        }
    }

    if (can_grab) {
        i32 grabdir = o->facing;
        if (!h->grabbing && dpad_x) {
            grabdir = dpad_x;
        }
        rec_i32 hgrabr = {o->pos.x + (0 < grabdir ? o->w : -1),
                          o->pos.y,
                          1,
                          8};

        for (obj_each(g, k)) {
            if (!(k->flags & OBJ_FLAG_GRABBABLE_SOLID)) continue;

            rec_i32 irec = obj_aabb(k);
            if (!overlap_rec(hgrabr, irec)) continue;

            i.action   = HERO_INTERACTION_GRAB;
            i.interact = obj_handle_from_obj(k);
            break;
        }
    }
    return i;
}

void hero_do_inair(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h      = (hero_s *)o->heap;
    i32     dpad_x = inps_x(inp);
    i32     dpad_y = inps_y(inp);

    o->animation++;

    if (h->edgeticks) {
        h->edgeticks--;
    }
    if (h->walljump_tick) {
        h->walljump_tick -= sgn_i32(h->walljump_tick);
    }
    if (h->air_block_ticks) {
        h->air_block_ticks -= sgn_i32(h->air_block_ticks);
    }

    i32    staminap       = hero_stamina_left(o);
    bool32 rope_stretched = (o->rope && (o->rope->len_max_q4 * 254) <= (rope_len_q4(g, o->rope) << 8));

    if (rope_stretched) { // swinging
        h->jumpticks     = 0;
        h->walljump_tick = 0;
        v2_i32 rn_curr   = o->ropenode->p;
        v2_i32 rn_next   = ropenode_neighbour(o->rope, o->ropenode)->p;
        v2_i32 dtrope    = v2_i32_sub(rn_next, rn_curr);
        i32    dtrope_s  = sgn_i32(dtrope.x);
        i32    dtrope_a  = abs_i32(dtrope.x);

        if (sgn_i32(o->v_q8.x) != dpad_x) {
            obj_vx_q8_mul(o, 254);
        }

        o->v_q8.x += ((dtrope_s == dpad_x) ? 45 : 10) * dpad_x;
    } else {
        bool32 do_x_movement  = 1;
        bool32 start_climbing = hero_has_upgrade(g, HERO_UPGRADE_CLIMB) &&
                                hero_is_climbing_offs(g, o, dpad_x, 0, 0) &&
                                !o->rope &&
                                !h->holds_spear;
        bool32 can_inair_jump = hero_ibuf_pressed(h, INP_A, 6) &&
                                !h->holds_spear &&
                                !o->rope &&
                                !h->spinattack &&
                                !h->b_hold_tick;

        if (start_climbing) {
            do_x_movement = 0;
            o->animation  = 0;
            if (inps_btn_jp(inp, INP_A)) {
                hero_walljump(g, o, -dpad_x);
            } else {
                h->climbing    = 1;
                o->facing      = dpad_x;
                o->v_q8.x      = 0;
                o->v_q8.y      = 0;
                h->attack_tick = 0;
                h->attack_ID   = 0;
                h->spinattack  = 0;
                h->b_hold_tick = 0;
                hero_leave_and_clear_inair(o);
            }
        } else if (staminap <= 0 && h->jump_index == HERO_JUMP_FLY) {
            h->jumpticks = 0;
        } else if (0 < h->jumpticks) { // dynamic jump height
            if (inps_btn(inp, INP_A)) {
                hero_jumpvar_s jv = g_herovar[h->jump_index];
                i32            t0 = pow_i32(jv.ticks, 4);
                i32            ti = pow_i32(h->jumpticks, 4) - t0;
                i32            ch = jv.v0 - ((jv.v1 - jv.v0) * ti) / t0;
                o->v_q8.y -= ch;
                if (h->jump_index == HERO_JUMP_FLY) {
                    hero_stamina_modify(o, -64);
                    if (h->jumpticks == jv.ticks - 8) {
                        snd_play(SNDID_WING1, 1.8f, 0.5f);
                    }
                }
                h->jumpticks--;
            } else { // cut jump short
                obj_vy_q8_mul(o, 128);
                h->jumpticks = 0;
                h->walljump_tick >>= 1;
                h->air_block_ticks >>= 1;
            }
        } else if (can_inair_jump) {
            hero_inair_jump(g, o, inp);
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
    for (i32 n = 0; n < 6; n++) {
        if (hero_is_climbing_offs(g, o, -dpad_x, -dpad_x * n, 0)) {
            jump_wall = 1;
            break;
        }
    }

    bool32 jump_midair = !usinghook && // not hooked
                         !h->jumpticks &&
                         !h->stomp &&
                         hero_has_upgrade(g, HERO_UPGRADE_FLY) &&
                         hero_stamina_left(o);

    if (ground_jump) {
        hero_restore_grounded_stuff(g, o);
        hero_start_jump(g, o, HERO_JUMP_GROUND);
    } else if (jump_wall) {
        hero_walljump(g, o, dpad_x);
    } else if (jump_midair) {
        hero_stamina_modify(o, -192);
        hero_start_jump(g, o, HERO_JUMP_FLY);

        v2_i32 dcpos = obj_pos_center(o);
        dcpos.x -= 16;
        dcpos.y -= 8;
        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP_AIR, dcpos);
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

    if (h->swimming == SWIMMING_DIVING && !submerged) {
        h->swimming = SWIMMING_SURFACE;
    }

    v2_i32  hcenter  = obj_pos_center(o);
    rec_i32 heroaabb = obj_aabb(o);

    if (hero_has_upgrade(g, HERO_UPGRADE_DIVE) && 0 < inps_y(inp)) {
        obj_move(g, o, 0, +10);
        o->v_q8.y   = +1000;
        h->swimming = SWIMMING_DIVING;
        return;
    }
    if (inps_btn_jp(inp, INP_A) && water_depth < HERO_HEIGHT - 4) { // jump out of water
        for (i32 n = 0; n < g->n_fluid_areas; n++) {
            fluid_area_s *fa = &g->fluid_areas[n];
            rec_i32       fr = {fa->x, fa->y - 4, fa->w, 16};
            if (!overlap_rec(fr, heroaabb)) continue;

            i32 ax = abs_i32(o->v_q8.x);
            fluid_area_impact(fa, hcenter.x - fa->x, 12, -150, FLUID_AREA_IMPACT_FLAT);
        }

        obj_move(g, o, 0, -water_depth);
        hero_start_jump(g, o, HERO_JUMP_WATER);
        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_WATER_SPLASH, hcenter);
        snd_play(SNDID_WATER_OUT_OF, 0.5f, 1.f);
        return;
    }

    // animation and sfx
    if ((dpad_x == 0 || inps_xp(inp) == 0) && dpad_x != inps_xp(inp)) {
        o->animation = 0;
    } else {
        o->animation++;
    }

    if (h->swimsfx_delay) {
        h->swimsfx_delay--;
    } else if (dpad_x) {
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

    if (h->swimming == SWIMMING_DIVING && hero_has_upgrade(g, HERO_UPGRADE_DIVE)) {
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
    }

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa = &g->fluid_areas[n];
        rec_i32       fr = {fa->x, fa->y - 4, fa->w, 16};
        if (!overlap_rec(fr, heroaabb)) continue;

        i32 ax = abs_i32(o->v_q8.x);
        fluid_area_impact(fa, hcenter.x - fa->x, 10, rngr_sym_i32(10), FLUID_AREA_IMPACT_FLAT);
    }

    if (dpad_x != sgn_i32(o->v_q8.x)) {
        o->v_q8.x /= 2;
    }
    if (dpad_x) {
        i32 i0 = (dpad_x == sgn_i32(o->v_q8.x) ? abs_i32(o->v_q8.x) : 0);
        i32 ax = (max_i32(512 - i0, 0) * 32) >> 8;
        o->v_q8.x += ax * dpad_x;
    }

    if (h->swimming == SWIMMING_DIVING) {
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
    i32     sta    = hero_stamina_modify(o, -4);

    if (inps_btn_jp(inp, INP_A)) {
        hero_walljump(g, o, -o->facing);
    } else if (dpad_y && sta) {
        i32 N = 3;
        if (dpad_y < 0) {
            hero_stamina_modify(o, -24);
            N = 2;
            o->animation += 2;
        } else {
            o->animation = 0;
        }

        i32 n_moved = 0;
        for (i32 n = 0; n < N; n++) {
            rec_i32 r = {o->pos.x, o->pos.y + dpad_y, o->w, o->h};
            if (map_blocked(g, r) ||
                !hero_is_climbing_offs(g, o, o->facing, 0, dpad_y)) {
                pltf_log("STOP");
                break;
            }

            obj_move(g, o, 0, dpad_y);
            n_moved++;
        }

        if (!n_moved) {
            bool32 stop_climbing = 1;

            if (dpad_y < 0) {
                o->v_q8.y = -1100;
            } else if (inps_btn_jp(inp, INP_DD)) {
                o->v_q8.y = +256;
            } else {
                stop_climbing = 0;
            }

            if (stop_climbing) {
                h->climbing     = 0;
                h->impact_ticks = 0;
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
        obj_move(g, o, 0, 1);
        o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
    } else if (0 < h->crouched && h->crouched < HERO_CROUCHED_MAX_TICKS) {
        h->crouched++;
        o->v_q8.x = 0;
    } else {
        if (0 < h->crouched && dpad_x) {
            h->crouched = -1;
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
                             !h->spinattack &&
                             !h->attack_tick &&
                             !o->rope &&
                             !h->holds_spear;
    bool32 can_jump = !h->spinattack &&
                      !h->attack_tick &&
                      !h->b_hold_tick;
    if (o->v_prev_q8.x == 0 && o->v_q8.x != 0) {
        o->animation = 0;
    }

    if (h->stomp < 0) {
        h->stomp++;
    } else if (h->grabbing) {
        obj_s *i = obj_from_obj_handle(h->obj_grabbed);

        if (inps_btn(inp, INP_B)) {
            if (dpad_x) {
                h->grabbing = dpad_x << 1;
            } else {
                h->grabbing = o->facing;
            }

            if (i && i->on_pushpull) {
                i32 res = i->on_pushpull(g, i, dpad_x);
                if (res && dpad_x == o->facing) {
                    obj_move(g, o, dpad_x, 0);
                }
            }
        } else {
            hero_ungrab(g, o);
        }
    } else if (can_start_crawl && 0 < dpad_y) { // start crawling
        o->pos.y += HERO_HEIGHT - HERO_HEIGHT_CROUCHED;
        o->h              = HERO_HEIGHT_CROUCHED;
        h->crouched       = 1;
        h->crouch_standup = 0;
        h->sprint         = 0;
        h->b_hold_tick    = 0;
    } else if (hero_ibuf_pressed(h, INP_A, 6) && can_jump) {
        h->impact_ticks = 2;
        hero_start_jump(g, o, HERO_JUMP_GROUND);
    } else if (!h->attack_tick) {
        if (h->crouch_standup) {
            h->crouch_standup--;
            obj_vx_q8_mul(o, 220);
        }

        bool32 can_sprint          = !o->rope;
        bool32 can_start_sprinting = hero_ibuf_tap(h, INP_DL | INP_DR, 15) &&
                                     hero_ibuf_pressed(h, INP_DL | INP_DR, 2);
        if (!can_sprint) {
            h->sprint = 0;
        } else if (!h->sprint && can_start_sprinting) {
            h->sprint = 1;

            if (dpad_x == -1) {
                o->v_q8.x = min_i32(o->v_q8.x, -HERO_VX_WALK);
            } else if (dpad_x == +1) {
                o->v_q8.x = max_i32(o->v_q8.x, +HERO_VX_WALK);
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
                h->sprint   = 0;
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
            h->sprint = 0;
            ax        = min_i32(70, va);
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
        h->ladder      = snap_to;
        o->v_q8.x      = 0;
        o->v_q8.y      = 0;
        h->jumpticks   = 0;
        h->edgeticks   = 0;
        h->spinattack  = 0;
        h->attack_tick = 0;
        h->attack_ID   = 0;
        h->gliding     = 0;
    }
    return snap_to;
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

i32 hero_get_actual_state(g_s *g, obj_s *o);

void hero_post_update(g_s *g, obj_s *o, inp_s inp)
{
    hero_s *h   = (hero_s *)o->heap;
    o->blinking = o->n_ignored_solids || h->invincibility_ticks;

    i32 rp = o->n_ignored_solids ? RENDER_PRIO_INFRONT_FLUID_AREA
                                 : RENDER_PRIO_HERO;
    if (rp != o->render_priority) {
        o->render_priority = rp;
        g->objrender_dirty = 1;
    }

    trampolines_do_bounce(g);

    b32 bounced = 0;

    // jump or stomped on
    for (i32 n = 0; n < h->n_jumpstomped; n++) {
        jumpstomped_s js = h->jumpstomped[n];
        obj_s        *i  = obj_from_obj_handle(js.h);
        if (!i) continue;

        if (js.stomped) {
            // only on stomp
        } else {
            // only on jump
        }

        if (i->enemy.hurt_on_jump && i->health) {
            enemy_hurt(g, i, 1);
            bounced = 1;
        }

        { // jump or stomp
            switch (i->ID) {
            default: break;
            case OBJID_SWITCH: switch_on_interact(g, i); break;
            case OBJID_CHEST: chest_on_open(g, i); break;
            }
        }
    }

    h->n_jumpstomped = 0;

    if (bounced) {
        o->v_q8.y = min_i32(o->v_q8.y, -1000);
        o->bumpflags &= ~OBJ_BUMP_Y_POS;
        h->stomp = 0;
    }

    if (h->stomp) {
        rec_i32 rbot = obj_rec_bottom(o);
        for (obj_each(g, i)) {
            if (i->ID != OBJID_STOMPABLE_BLOCK) continue;
            if (overlap_rec(rbot, obj_aabb(i))) {
                stompable_block_on_destroy(g, i);
                h->stomp  = 0;
                o->v_q8.y = -750;
                o->bumpflags &= ~OBJ_BUMP_Y_POS;
                particle_emit_ID(g, PARTICLE_EMIT_ID_STOMPBLOCK_DESTROY,
                                 obj_pos_bottom_center(o));
                cam_screenshake_xy(&g->cam, 20, 0, 4);
            }
        }

        if (obj_grounded(g, o)) {
            cam_screenshake_xy(&g->cam, 10, 0, 4);
            h->stomp  = -HERO_STOMP_LANDING_TICKS;
            v2_i32 pb = obj_pos_bottom_center(o);
            pb.y -= 4;
            particle_emit_ID(g, PARTICLE_EMIT_ID_STOMP, pb);
        }
    }

    v2_i32  hcenter  = obj_pos_center(o);
    rec_i32 heroaabb = obj_aabb(o);
    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa  = &g->fluid_areas[n];
        rec_i32       fr1 = {fa->x, fa->y - 4, fa->w, 16};
        rec_i32       fr2 = {fa->x, fa->y - 1, fa->w, 2};
        if (overlap_rec(fr1, heroaabb)) {
            i32 imp = clamp_sym_i32(abs_i32(o->v_q8.x) / 20, 100);
            fluid_area_impact(fa, hcenter.x - fa->x, 12, imp, FLUID_AREA_IMPACT_COS);
        }
    }

    // rope
    rope_s          *r  = o->rope;
    grapplinghook_s *gh = &g->ghook;
    if (r && grapplinghook_rope_intact(g, gh)) {
        grapplinghook_update(g, gh);
        rope_update(g, r);
        grapplinghook_animate(g, gh);

        if (gh->state) {
            if (gh->state == GRAPPLINGHOOK_FLYING) {
                v2_i32 v_hook = rope_recalc_v(g, r, gh->rn,
                                              v2_i32_from_i16(gh->p_q8),
                                              v2_i32_from_i16(gh->v_q8));
                gh->v_q8      = v2_i16_from_i32(v_hook);
            } else {
                bool32 calc_v = 0;
                calc_v |= obj_grounded(g, o) && 271 <= rope_stretch_q8(g, r);
                calc_v |= !obj_grounded(g, o);
                if (calc_v) {
                    v2_i32 v_hero = rope_recalc_v(g, r, o->ropenode,
                                                  v2_i32_from_i16(o->subpos_q8),
                                                  v2_i32_from_i16(o->v_q8));
                    o->v_q8       = v2_i16_from_i32(v_hero);
                }
            }
        }
    }

    bool32 collected_upgrade = 0;
    i32    hero_dmg          = 0;
    v2_i16 hero_knockback    = {0};

    for (i32 n = 0; n < g->n_hitbox_tmp; n++) {
        hitbox_tmp_s *hb      = &g->hitbox_tmp[n];
        bool32        overlap = 0;
        v2_i32        dt      = {0};

        switch (hb->type) {
        case HITBOX_TMP_CIR: {
            v2_i32 pcir = {hb->x, hb->y};
            dt          = v2_i32_sub(hcenter, pcir);
            if ((i32)v2_i32_lensq(dt) <= hb->cir_r * hb->cir_r) {
                overlap = 1;
            }
            break;
        }
        case HITBOX_TMP_REC: {
            rec_i32 rh = {hb->x, hb->y, hb->rec_w, hb->rec_h};
            if (overlap_rec(rh, heroaabb)) {
                overlap        = 1;
                v2_i32 rcenter = {rh.x + rh.w / 2, rh.y + rh.h / 2};
                dt             = v2_i32_sub(hcenter, rcenter);
            }
            break;
        }
        }

        if (overlap) {
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
        }
    }

    for (obj_each(g, it)) {
        if (!overlap_rec(heroaabb, obj_aabb(it))) continue;

        if (it->flags & OBJ_FLAG_HURT_ON_TOUCH) {
            v2_i32 ocenter   = obj_pos_center(it);
            v2_i32 dt        = v2_i32_sub(hcenter, ocenter);
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
            if (it->on_touchhurt_hero) {
                it->on_touchhurt_hero(g, it);
            }
        }

        switch (it->ID) {
        case OBJID_COIN: {
            if (!overlap_rec(heroaabb, obj_aabb(it))) break;
            coins_change(g, +1);
            snd_play(SNDID_COIN, 1.f, 1.f);
            obj_delete(g, it);
            break;
        }
        case OBJID_HERO_POWERUP: {
            hero_powerup_collected(g, hero_powerup_obj_ID(it));
            save_event_register(g, hero_powerup_saveID(it));
            obj_delete(g, it);
            objs_cull_to_delete(g);
            collected_upgrade = 1;
            break;
        }
        case OBJID_STAMINARESTORER: {
            staminarestorer_try_collect(g, it, o);
            break;
        }
        case OBJID_PROJECTILE: projectile_on_collision(g, it); break;
        case OBJID_FALLINGSTONE: fallingstone_burst(g, it); break;
        case OBJID_STALACTITE:
            if (it->flags & OBJ_FLAG_HURT_ON_TOUCH) {
                stalactite_burst(g, it);
            }
            break;
        }
    }

    i32 bx1 = max_i32(heroaabb.x >> 4, 0);
    i32 by1 = max_i32(heroaabb.y >> 4, 0);
    i32 bx2 = min_i32((heroaabb.x + heroaabb.w - 1) >> 4, g->tiles_x - 1);
    i32 by2 = min_i32((heroaabb.y + heroaabb.h - 1) >> 4, g->tiles_y - 1);

    for (i32 y = by1; y <= by2; y++) {
        for (i32 x = bx1; x <= bx2; x++) {
            if (g->tiles[x + y * g->tiles_x].collision != TILE_SPIKES)
                continue;
            v2_i32 ptilec    = {(x << 4) + 8, (y << 4) + 8};
            v2_i32 dt        = v2_i32_sub(hcenter, ptilec);
            hero_knockback.x = 1000 * sgn_i32(dt.x);
            hero_knockback.y = 1000 * sgn_i32(dt.y);
            hero_dmg         = max_i32(hero_dmg, 1);
        }
    }

    if (!collected_upgrade) {
        maptransition_try_hero_slide(g);
    }

    if (hero_dmg && !h->invincibility_ticks) {
        hero_hurt(g, o, hero_dmg);
        snd_play(SNDID_SWOOSH, 0.5f, 0.5f);
        o->v_q8 = hero_knockback;
        o->bumpflags &= ~(OBJ_BUMP_Y | OBJ_BUMP_X);
        g->events_frame |= EVENT_HERO_DAMAGE;
        if (o->health == 0) {
            g->events_frame |= EVENT_HERO_DEATH;
        }
    }

    if (o->health && obj_grounded(g, o) && o->v_q8.x == 0) {
        coins_show_idle(g);
    }
}
