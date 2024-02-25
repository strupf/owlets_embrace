// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render/render.h"

void game_init(game_s *g)
{
#ifdef SYS_SDL
    aud_mute(0);
    sys_set_volume(0.1f);
#endif
    mus_set_trg_vol(64);
    g->cam.mode = CAM_MODE_FOLLOW_HERO;

    map_world_load(&g->map_world, "world.world");
#if 0
    for (int i = 0; i < 1024; i++) {
        if ((i & 7) == 0) sys_printf("\n");
        int r = (int)(cosf(M_PI * 2.f * (f32)i / 1024.f) * 64.5f);
        if (r < 0) {
            sys_printf("-");
        } else {
            sys_printf("+");
        }

        sys_printf("0x%02X, ", abs_i(r));
    }
#endif
}

static void gameplay_tick(game_s *g)
{
    item_selector_update(&g->item_selector);
    g->events_frame          = 0;
    g->herodata.interactable = obj_handle_from_obj(NULL);

    for (obj_each(g, o)) {
        const v2_i32 posprev = o->pos;

        if (0 < o->invincible_tick) {
            o->invincible_tick--;
        }
        if (o->enemy.invincible) {
            o->enemy.invincible--;
        }

        obj_game_update(g, o);
        o->posprev = posprev;
#ifdef SYS_DEBUG
        assert(o->magic == OBJ_MAGIC);
        assert((o->flags & (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID)) !=
               (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID));
#endif
    }

    for (obj_each(g, o)) { // integrate acc, vel and drag: adds tomove accumulator
        if (o->flags & OBJ_FLAG_MOVER) {
            obj_apply_movement(o);
        }
    }

    for (obj_each(g, o)) { // move objects by tomove
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;
        solid_move(g, o, o->tomove);
        o->tomove.x = 0, o->tomove.y = 0;
    }

    for (obj_each(g, o)) {
        if (!(o->flags & OBJ_FLAG_ACTOR_PLATFORM)) continue;
        if ((o->flags & OBJ_FLAG_ACTOR_PLATFORM) == OBJ_FLAG_PLATFORM) {
            platform_move(g, o, o->tomove);
        } else if (actor_try_wiggle(g, o)) {
            actor_move(g, o, o->tomove);
        }

        o->tomove.x = 0, o->tomove.y = 0;
    }

    const rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) &&
            !overlap_rec(obj_aabb(o), roombounds)) {
            assert(o->ID != OBJ_ID_HERO);
            obj_delete(g, o);
        }
    }

    objs_cull_to_delete(g);
    coinparticle_update(g);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    // hero touching other objects
    if (ohero && 0 < ohero->health) {
        const rec_i32 heroaabb     = obj_aabb(ohero);
        bool32        herogrounded = obj_grounded(g, ohero);
        for (obj_each(g, o)) {
            switch (o->ID) {
            case OBJ_ID_SHROOMY: {
                if (herogrounded) break;
                rec_i32 rs = obj_aabb(o);
                rec_i32 ri;
                if (!intersect_rec(heroaabb, rs, &ri)) break;
                if (0 < ohero->vel_q8.y && heroaabb.y + heroaabb.h < rs.y + rs.h) {
                    ohero->vel_q8.y = -2000;
                    ohero->tomove.y -= ri.h;
                    shroomy_bounced_on(o);
                }
                break;
            }
            }
        }

#if GAME_JUMP_ATTACK
        void obj_game_player_jump_heads(game_s * g, obj_s * ohero);
        obj_game_player_jump_heads(g, ohero);
#endif

        hero_check_rope_intact(g, ohero);

        // touched hurting things?
        if (ohero->invincible_tick <= 0) {
            v2_i32 hcenter        = obj_pos_center(ohero);
            int    hero_dmg       = 0;
            v2_i32 hero_knockback = {0};

            for (obj_each(g, o)) {
                if (!(o->flags & OBJ_FLAG_HURT_ON_TOUCH)) continue;
                if (!overlap_rec(heroaabb, obj_aabb(o))) continue;
                v2_i32 ocenter   = obj_pos_center(o);
                v2_i32 dt        = v2_sub(hcenter, ocenter);
                hero_knockback.x = sgn_i(dt.x) * 1000;
                hero_knockback.y = -1000;
                hero_dmg         = max_i(hero_dmg, 1);

                switch (o->ID) {
                case OBJ_ID_CHARGER: {
                    int pushs        = sgn_i(hcenter.x - ocenter.x);
                    hero_knockback.x = pushs * 2000;
                    break;
                }
                }
            }

            if (hero_dmg) {
                hero_hurt(g, ohero, hero_dmg);
                snd_play_ext(SNDID_SWOOSH, 0.5f, 0.5f);
                ohero->vel_q8 = hero_knockback;
                ohero->bumpflags &= ~OBJ_BUMPED_Y; // have to clr y bump
                g->events_frame |= EVENT_HERO_DAMAGE;
            }
        }
    }

    if (sys_key(SYS_KEY_K) && ohero && 0 < ohero->health) {
        hero_kill(g, ohero);
    }

    objs_cull_to_delete(g);

#ifdef SYS_DEBUG
    for (obj_each(g, o)) {
        assert(o->magic == OBJ_MAGIC);
    }
#endif

    int to_freeze = 0;
    if (g->events_frame & EVENT_HIT_ENEMY) {
        to_freeze = max_i(to_freeze, 2);
    }
    if (g->events_frame & EVENT_HERO_DAMAGE) {
        to_freeze = max_i(to_freeze, 4);
    }

    // possibly enter new substates
    if (ohero) {
        obj_s *interactable = obj_from_obj_handle(g->herodata.interactable);
        if (ohero->health <= 0) {
            if (substate_finished(&g->substate))
                substate_respawn(g, &g->substate);
        } else if (substate_finished(&g->substate)) {

            bool32        collected_upgrade = 0;
            const rec_i32 heroaabb          = obj_aabb(ohero);
            for (obj_each(g, o)) {
                if (o->ID != OBJ_ID_HEROUPGRADE) continue;
                if (!overlap_rec(heroaabb, obj_aabb(o))) continue;
                heroupgrade_on_collect(g, o, &g->herodata);
                obj_delete(g, o);
                objs_cull_to_delete(g);
                collected_upgrade = 1;
                break;
            }

            if (!collected_upgrade) {
                bool32 t = substate_transition_try_hero_slide(g, &g->substate);
                if (t == 0) { // nothing happended
                    if (interactable && inp_just_pressed(INP_DPAD_U)) {
                        obj_game_interact(g, interactable);
                        g->herodata.interactable = obj_handle_from_obj(NULL);
                    } else if (to_freeze) {
                        substate_freeze(&g->substate, to_freeze);
                    }
                }
            }
        }
    }

    particles_update(g, &g->particles);
}

void game_tick(game_s *g)
{
    bool32 update_gameplay = 1;

    if (!substate_finished(&g->substate)) {
        substate_update(g, &g->substate);
        update_gameplay = !substate_blocks_gameplay(&g->substate);
    } else if (shop_active(g)) {
        shop_update(g);
        update_gameplay = 0;
    }

    if (update_gameplay) {
        gameplay_tick(g);

        for (obj_each(g, o)) {
            obj_game_animate(g, o);
        }

        for (int n = g->n_enemy_decals - 1; 0 <= n; n--) {
            g->enemy_decals[n].tick--;
            if (g->enemy_decals[n].tick <= 0) {
                g->enemy_decals[n] = g->enemy_decals[--g->n_enemy_decals];
            }
        }
    }

    cam_update(g, &g->cam);
    if (g->areaname.fadeticks) {
        g->areaname.fadeticks++;
        if (FADETICKS_AREALABEL <= g->areaname.fadeticks) {
            g->areaname.fadeticks = 0;
        }
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (sys_tick() & 1) {
        backforeground_animate_grass(g);
    }

    area_update(g, &g->area);
}

void game_open_inventory(game_s *g)
{
}

void game_resume(game_s *g)
{
}

void game_paused(game_s *g)
{
    render_pause(g);
}

void game_new_savefile(game_s *g, int slotID)
{
    assert(0);
    game_load_map(g, "map_01");
    g->savefile_slotID = slotID;
    obj_s *oh          = hero_create(g);
    oh->pos.x          = 300;
    oh->pos.y          = 500;
}

void game_write_savefile(game_s *g)
{
    savefile_s  sf   = {0};
    //
    herodata_s *hero = &g->herodata;
    strcpy(sf.area_filename, g->areaname.filename);
    strcpy(sf.hero_name, hero->name);
    memcpy(sf.upgrades, hero->upgrades, sizeof(hero->upgrades));
    sf.n_airjumps = hero->n_airjumps;
    sf.health     = hero->health;
    //
    g->save       = sf;
    savefile_write(g->savefile_slotID, &sf);
}

void game_load_savefile(game_s *g, savefile_s sf, int slotID)
{
    g->savefile_slotID = slotID;
    g->save            = sf;
    game_load_map(g, sf.area_filename);

    // mus_play("assets/mus/overworld.wav");

    herodata_s *hero = &g->herodata;
    memcpy(hero->upgrades, sf.upgrades, sizeof(sf.upgrades));
    hero->n_airjumps = sf.n_airjumps;
    hero->health     = sf.health;

#if 1
    obj_s *oh = hero_create(g);
    oh->pos.x = 200;
    oh->pos.y = 350;
#endif
}

void game_on_trigger(game_s *g, int trigger)
{
    if (!trigger) return;
    for (obj_each(g, o)) {
        obj_game_trigger(g, o, trigger);
    }

    if (trigger == 1000) {
        g->herodata.upgrades[HERO_UPGRADE_AIR_JUMP_1] = 1;
        g->herodata.upgrades[HERO_UPGRADE_HOOK]       = 1;
        snd_play_ext(SNDID_UPGRADE, 0.6f, 1.f);
    }
}

void game_put_grass(game_s *g, int tx, int ty)
{
    if (g->n_grass >= ARRLEN(g->grass)) return;
    grass_s *gr = &g->grass[g->n_grass++];
    *gr         = (grass_s){0};
    gr->pos.x   = tx * 16;
    gr->pos.y   = ty * 16;
    gr->type    = rngr_i32(0, 2);
}

static inline i32 ocean_height_logic_q6(i32 p, i32 t)
{
    return (sin_q6((p >> 2) + (t << 1) + 0x00) << 4) +
           (sin_q6((p >> 1) + (t << 1) + 0x80) << 3) +
           (sin_q6((p >> 0) - (t << 2) + 0x40) << 2);
}

int ocean_height(game_s *g, int pixel_x)
{
    i32 h = ocean_height_logic_q6(pixel_x, sys_tick());
    return (h >> 6) + g->ocean.y;
}

int ocean_render_height(game_s *g, int pixel_x)
{
    int p = pixel_x;
    int t = sys_tick();
    i32 y = ocean_height_logic_q6(p, t) +
            (sin_q6((p << 2) + (t << 4) + 0x20) << 1) +
            (sin_q6((p << 4) - (t << 5) + 0x04) << 0) +
            (sin_q6((p << 5) + (t << 6) + 0x10) >> 2);
    i32 h = (y >> 6) + g->ocean.y;
    return h;
}

int water_depth_rec(game_s *g, rec_i32 r)
{
    int f        = 0;
    int y_bottom = r.y + r.h - 1;
    int px       = r.x + (r.w >> 1);
    if (g->ocean.active) {
        f = max_i(0, y_bottom - ocean_height(g, px));
    }

    int d = 0;
    int i = (px >> 4) + (y_bottom >> 4) * g->tiles_x;
    if (g->tiles[i].type & TILE_WATER_MASK) {
        d = (y_bottom & 15);
        for (i -= g->tiles_x; 0 <= i; i -= g->tiles_x) {
            if (!(g->tiles[i].type & TILE_WATER_MASK)) break;
            d += 16;
        }
    }

    return max_i(f, d);
}

bool32 tiles_hookable(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 riarea;
    if (!intersect_rec(r, rgrid, &riarea)) return 0;

    int px0 = riarea.x;
    int py0 = riarea.y;
    int px1 = riarea.x + riarea.w - 1;
    int py1 = riarea.y + riarea.h - 1;
    int tx0 = px0 >> 4;
    int ty0 = py0 >> 4;
    int tx1 = px1 >> 4;
    int ty1 = py1 >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        int y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        int y1 = (ty == ty1 ? py1 & 15 : 15);

        for (int tx = tx0; tx <= tx1; tx++) {
            tile_s tile = g->tiles[tx + ty * g->tiles_x];
            int    c    = tile.collision;
            int    t    = tile.type;
            if (c == TILE_EMPTY || NUM_TILE_BLOCKS <= c) continue;
            if (!(t == TILE_TYPE_DIRT ||
                  t == TILE_TYPE_DIRT_DARK))
                continue;
            if (c == TILE_BLOCK) return 1;
            int x0 = (tx == tx0 ? px0 & 15 : 0);
            int x1 = (tx == tx1 ? px1 & 15 : 15);
            int mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (int py = y0; py <= y1; py++)
                if (g_pxmask_tab[(c << 4) + py] & mk) return 1;
        }
    }
    return 0;
}

bool32 tiles_solid(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 riarea;
    if (!intersect_rec(r, rgrid, &riarea)) return 0;

    int px0 = riarea.x;
    int py0 = riarea.y;
    int px1 = riarea.x + riarea.w - 1;
    int py1 = riarea.y + riarea.h - 1;
    int tx0 = px0 >> 4;
    int ty0 = py0 >> 4;
    int tx1 = px1 >> 4;
    int ty1 = py1 >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        int y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        int y1 = (ty == ty1 ? py1 & 15 : 15);

        for (int tx = tx0; tx <= tx1; tx++) {
            int c = g->tiles[tx + ty * g->tiles_x].collision;
            if (c == TILE_EMPTY || NUM_TILE_BLOCKS <= c) continue;
            if (c == TILE_BLOCK) return 1;
            int x0 = (tx == tx0 ? px0 & 15 : 0);
            int x1 = (tx == tx1 ? px1 & 15 : 15);
            int mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (int py = y0; py <= y1; py++)
                if (g_pxmask_tab[(c << 4) + py] & mk) return 1;
        }
    }
    return 0;
}

bool32 tiles_solid_pt(game_s *g, int x, int y)
{
    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 0;

    int ID = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x].collision;
    if (ID == TILE_EMPTY || NUM_TILE_BLOCKS <= ID) return 0;
    return (g_pxmask_tab[(ID << 4) + (y & 15)] & (0x8000 >> (x & 15)));
}

bool32 tile_one_way(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 riarea;
    if (!intersect_rec(r, rgrid, &riarea)) return 0;

    int tx0 = (riarea.x) >> 4;
    int ty0 = (riarea.y) >> 4;
    int tx1 = (riarea.x + riarea.w - 1) >> 4;
    int ty1 = (riarea.y + riarea.h - 1) >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            int t = g->tiles[tx + ty * g->tiles_x].collision;
            if (t == TILE_ONE_WAY || t == TILE_LADDER_ONE_WAY)
                return 1;
        }
    }
    return 0;
}

bool32 game_traversable(game_s *g, rec_i32 r)
{
    if (tiles_solid(g, r)) return 0;

    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(o)))
            return 0;
    }
    return 1;
}

bool32 game_traversable_pt(game_s *g, int x, int y)
{
    if (tiles_solid_pt(g, x, y)) return 0;

    v2_i32 p = {x, y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec_pnt(obj_aabb(o), p))
            return 0;
    }
    return 1;
}

bounds_2D_s game_tilebounds_rec(game_s *g, rec_i32 r)
{
    v2_i32 pmin = {r.x, r.y};
    v2_i32 pmax = {r.x + r.w, r.y + r.h};
    return game_tilebounds_pts(g, pmin, pmax);
}

bounds_2D_s game_tilebounds_pts(game_s *g, v2_i32 p0, v2_i32 p1)
{
    bounds_2D_s b = {
        max_i(p0.x - 1, 0) >> 4, // div 16
        max_i(p0.y - 1, 0) >> 4,
        min_i(p1.x + 1, g->pixel_x - 1) >> 4,
        min_i(p1.y + 1, g->pixel_y - 1) >> 4,
    };
    return b;
}

bounds_2D_s game_tilebounds_tri(game_s *g, tri_i32 t)
{
    v2_i32 pmin = v2_min3(t.p[0], t.p[1], t.p[2]);
    v2_i32 pmax = v2_max3(t.p[0], t.p[1], t.p[2]);
    return game_tilebounds_pts(g, pmin, pmax);
}

int tick_to_index_freq(int tick, int n_frames, int freqticks)
{
    int i = ((tick * n_frames) / freqticks) % n_frames;
    assert(0 <= i && i < n_frames);
    return i;
}

void game_on_solid_appear(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
    }

    for (obj_each(g, o)) {
        if (o->flags & OBJ_FLAG_ACTOR) {
            actor_try_wiggle(g, o);
        }
    }
}

#define ITEM_SELECTOR_CRANK_THRESHOLD 8000
void item_selector_update(item_selector_s *is)
{
    if (is->decoupled) return;

    int p_q16 = inp_prev_crank_q16();
    int c_q16 = inp_crank_q16();
    int dt    = inp_crank_calc_dt_q16(p_q16, c_q16);
    int dtp   = inp_crank_calc_dt_q16(p_q16, is->angle);
    int dtc   = inp_crank_calc_dt_q16(is->angle, c_q16);

    // turn item barrel, snaps into crank angle for small angles
    int angle_old = is->angle;
    if ((dt > 0 && dtp >= 0 && dtc >= 0) || (dt < 0 && dtp <= 0 && dtc <= 0)) {
        is->angle = c_q16;
    } else if (abs_i(dtc) < ITEM_SELECTOR_CRANK_THRESHOLD) {
        int d = sgn_i(dtc) * (ITEM_SELECTOR_CRANK_THRESHOLD - abs_i(dtc)) / 10;
        if (sgn_i(d) == sgn_i(dtc) && abs_i(d) > abs_i(dtc)) {
            d = dtc;
        }

        is->angle = (is->angle + d + 0x10000) & 0xFFFF;
    }

    // item selection based on new item barrel angle
    // check if the barrel "flipped over" the 180 deg position
    // -> select next/prev item
    int change = inp_crank_calc_dt_q16(angle_old, is->angle);
    if (change > 0 && angle_old < 0x8000 && is->angle >= 0x8000) {
#if 0
        do {
            is->item += 1;
            is->item %= NUM_HERO_ITEMS;
        } while (!(h->aquired_items & (1 << is->item)));
#else
        is->item = 1 - is->item;

#endif
    } else if (change < 0 && angle_old >= 0x8000 && is->angle < 0x8000) {
#if 0
        do {
            is->item += NUM_HERO_ITEMS - 1;
            is->item %= NUM_HERO_ITEMS;
        } while (!(h->aquired_items & (1 << is->item)));
#else
        is->item = 1 - is->item;
#endif
    }
}

bool32 ladder_overlaps_rec(game_s *g, rec_i32 r, v2_i32 *tpos)
{
    rec_i32 ri;
    rec_i32 rroom = {0, 0, g->pixel_x, g->pixel_y};
    if (!intersect_rec(rroom, r, &ri)) return 0;
    int tx1 = (ri.x) >> 4;
    int ty1 = (ri.y) >> 4;
    int tx2 = (ri.x + ri.w - 1) >> 4;
    int ty2 = (ri.y + ri.h - 1) >> 4;
    for (int ty = ty1; ty <= ty2; ty++) {
        for (int tx = tx1; tx <= tx2; tx++) {
            int t = g->tiles[tx + ty * g->tiles_x].collision;
            if (t == TILE_LADDER || t == TILE_LADDER_ONE_WAY) {
                if (tpos) {
                    tpos->x = tx;
                    tpos->y = ty;
                }
                return 1;
            }
        }
    }
    return 0;
}

static void *game_alloc_ctx(void *ctx, usize s)
{
    NOT_IMPLEMENTED
    game_s *g = (game_s *)ctx;
    return NULL;
}

alloc_s game_allocator(game_s *g)
{
    alloc_s a = {game_alloc_ctx, (void *)g};
    return a;
}

void backforeground_animate_grass(game_s *g)
{
    for (int n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->vel_q8.x >> 4;
            }
        }

        gr->v_q8 += rngr_sym_i32(6) - ((gr->x_q8 * 15) >> 8);
        gr->x_q8 += gr->v_q8;
        gr->x_q8 = clamp_i(gr->x_q8, -256, +256);
        gr->v_q8 = (gr->v_q8 * 230) >> 8;
    }
}

// operate on 16x16 tiles
// these are the collision masks per pixel row of a tile
const int g_pxmask_tab[32 * 16] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // empty
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // block
    0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, // SLOPES 45
    0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF, 0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001,
    0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF,
    0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0, 0xFFE0, 0xFFC0, 0xFF80, 0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000, 0x8000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x000F, 0x003F, 0x00FF, 0x03FF, 0x0FFF, 0x3FFF, 0xFFFF, // SLOPES LO
    0xFFFF, 0x3FFF, 0x0FFF, 0x03FF, 0x00FF, 0x003F, 0x000F, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xC000, 0xF000, 0xFC00, 0xFF00, 0xFFC0, 0xFFF0, 0xFFFC, 0xFFFF,
    0xFFFF, 0xFFFC, 0xFFF0, 0xFFC0, 0xFF00, 0xFC00, 0xF000, 0xC000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0001, 0x0003, 0x0003, 0x0007, 0x0007, 0x000F, 0x000F, 0x001F, 0x001F, 0x003F, 0x003F, 0x007F, 0x007F, 0x00FF, 0x00FF, // rotated
    0x00FF, 0x00FF, 0x007F, 0x007F, 0x003F, 0x003F, 0x001F, 0x001F, 0x000F, 0x000F, 0x0007, 0x0007, 0x0003, 0x0003, 0x0001, 0x0001,
    0x8000, 0x8000, 0xC000, 0xC000, 0xE000, 0xE000, 0xF000, 0xF000, 0xF800, 0xF800, 0xFC00, 0xFC00, 0xFE00, 0xFE00, 0xFF00, 0xFF00,
    0xFF00, 0xFF00, 0xFE00, 0xFE00, 0xFC00, 0xFC00, 0xF800, 0xF800, 0xF000, 0xF000, 0xE000, 0xE000, 0xC000, 0xC000, 0x8000, 0x8000,
    0x0003, 0x000F, 0x003F, 0x00FF, 0x03FF, 0x0FFF, 0x3FFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // SLOPES HI
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x3FFF, 0x0FFF, 0x03FF, 0x00FF, 0x003F, 0x000F, 0x0003,
    0xC000, 0xF000, 0xFC00, 0xFF00, 0xFFC0, 0xFFF0, 0xFFFC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFC, 0xFFF0, 0xFFC0, 0xFF00, 0xFC00, 0xF000, 0xC000,
    0x01FF, 0x01FF, 0x03FF, 0x03FF, 0x07FF, 0x07FF, 0x0FFF, 0x0FFF, 0x1FFF, 0x1FFF, 0x3FFF, 0x3FFF, 0x7FFF, 0x7FFF, 0xFFFF, 0xFFFF, // rotated
    0xFFFF, 0xFFFF, 0x7FFF, 0x7FFF, 0x3FFF, 0x3FFF, 0x1FFF, 0x1FFF, 0x0FFF, 0x0FFF, 0x07FF, 0x07FF, 0x03FF, 0x03FF, 0x01FF, 0x01FF,
    0xFF80, 0xFF80, 0xFFC0, 0xFFC0, 0xFFE0, 0xFFE0, 0xFFF0, 0xFFF0, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFC, 0xFFFE, 0xFFFE, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFE, 0xFFFE, 0xFFFC, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF0, 0xFFF0, 0xFFE0, 0xFFE0, 0xFFC0, 0xFFC0, 0xFF80, 0xFF80,
    //
};

// triangle coordinates
// x0 y0 x1 y1 x2 y2
const i32 tilecolliders[GAME_NUM_TILECOLLIDERS * 6] = {
    // dummy triangles
    0, 0, 0, 0, 0, 0, // empty
    0, 0, 0, 0, 0, 0, // solid
    // slope 45
    0, 16, 16, 16, 16, 0, // 2
    0, 0, 16, 0, 16, 16,  // 3
    0, 0, 0, 16, 16, 16,  // 4
    0, 0, 0, 16, 16, 0,   // 5
    // slope lo
    0, 16, 16, 16, 16, 8, // 6
    0, 0, 16, 0, 16, 8,   // 7
    0, 8, 0, 16, 16, 16,  // 8
    0, 0, 16, 0, 0, 8,    // 9
    16, 0, 16, 16, 8, 16, // 10
    8, 0, 16, 0, 16, 16,  // 11
    0, 0, 0, 16, 8, 16,   // 12
    0, 0, 8, 0, 0, 16,    // 13
    // slopes hi
    0, 16, 16, 16, 16, 0, 0, 8, 0, 0, 0, 0, // 14
    0, 0, 16, 0, 16, 16, 0, 8, 0, 0, 0, 0,  // 15
    0, 0, 16, 8, 16, 16, 0, 16, 0, 0, 0, 0, // 16
    0, 0, 16, 0, 16, 8, 0, 16, 0, 0, 0, 0,  // 17
    8, 0, 16, 0, 16, 16, 0, 16, 0, 0, 0, 0, // 18
    0, 0, 16, 0, 16, 16, 8, 16, 0, 0, 0, 0, // 19
    0, 0, 8, 0, 16, 16, 0, 16, 0, 0, 0, 0,  // 20
    0, 0, 16, 0, 8, 16, 0, 16, 0, 0, 0, 0,  // 21
    //
};