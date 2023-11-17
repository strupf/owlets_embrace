// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "assets.h"
#include "render.h"
#include "rope.h"

u16 g_animated_tiles[65536];

static void backforeground_windparticles(game_s *g);

void game_init(game_s *g)
{
    for (int i = 0; i < ARRLEN(g_animated_tiles); i++) {
        g_animated_tiles[i] = i;
    }

    g->cam.w    = 400;
    g->cam.h    = 240;
    g->cam.mode = CAM_MODE_FOLLOW_HERO;

    map_world_load(&g->map_world, "assets/map/proj.ldtk");
    game_load_map(g, "assets/map/proj/Level_1.ldtkl");

    obj_s *oh = obj_hero_create(g);
    oh->pos.x = 60;

    g->herodata.aquired_items |= 1 << HERO_ITEM_BOMB;
    g->herodata.aquired_items |= 1 << HERO_ITEM_HOOK;
    g->herodata.aquired_items |= 1 << HERO_ITEM_BOW;
    hero_set_cur_item(&g->herodata, HERO_ITEM_BOMB);
}

void game_tick(game_s *g)
{
    g->tick++;
    hero_crank_item_selection(&g->herodata);
    obj_s *ohero_ = obj_get_tagged(g, OBJ_TAG_HERO);

    spriteanim_update(&ohero_->spriteanim[0]);

    transition_s *t = &g->transition;

    if (g->textbox.state == TEXTBOX_STATE_INACTIVE &&
        transition_finished(t)) {
        for (int i = 0; i < g->obj_nbusy; i++) {
            obj_s *o = g->obj_busy[i];
            switch (o->ID) {
            case OBJ_ID_SOLID: {
                int px      = 200 + ((sin_q16(g->tick * 300) * 100) >> 16);
                o->tomove.x = px - o->pos.x;
            } break;
            case OBJ_ID_HERO: {
                hero_update(g, o);
            } break;
            }
        }

        // integrate acceleration, velocity and drag
        // adds tomove accumulator
        for (int i = 0; i < g->obj_nbusy; i++) {
            obj_s *o = g->obj_busy[i];
            if (!(o->flags & OBJ_FLAG_MOVER)) continue;
            obj_apply_movement(o);
        }

        // move objects by tomove
        for (int i = 0; i < g->obj_nbusy; i++) {
            obj_s *o = g->obj_busy[i];
            if (o->flags & OBJ_FLAG_ACTOR) {
                actor_move(g, o, o->tomove);
            } else if (o->flags & OBJ_FLAG_SOLID) {
                solid_move(g, o, o->tomove);
            } else {
                o->pos = v2_add(o->pos, o->tomove);
            }
            o->tomove.x = 0;
            o->tomove.y = 0;
        }

        objs_cull_to_delete(g);

#ifdef SYS_DEBUG
        for (int n = 0; n < NUM_OBJ; n++)
            assert(g->obj_raw[n].magic == OBJ_GENERIC_MAGIC);
#endif

        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (ohero) {
            hero_room_transition(g, ohero);
        }
    }

    if (g->textbox.state != TEXTBOX_STATE_INACTIVE) {
        textbox_update(&g->textbox);
    }

    if (!transition_finished(t)) {
        transition_update(g, t);
    }

    cam_s *cam = &g->cam;
    cam_update(g, cam);

    backforeground_windparticles(g);
}

void game_draw(game_s *g)
{
    render(g);
    transition_draw(&g->transition);
}

static void backforeground_windparticles(game_s *g)
{
    // traverse backwards to avoid weird removal while iterating
    for (int n = g->n_windparticles - 1; n >= 0; n--) {
        windparticle_s *p = &g->windparticles[n];
        if ((p->p_q8.x >> 8) < -200 || (p->p_q8.x >> 8) > g->pixel_x + 200) {
            g->windparticles[n] = g->windparticles[--g->n_windparticles];
            continue;
        }

        p->circcooldown--;
        if (p->circcooldown <= 0 && rngr_u32(0, 65535) < 600) { // enter wind circle animation
            p->ticks        = 0;                                // rng_range_u32(15, 20);
            p->circticks    = p->ticks;
            p->circc.x      = p->p_q8.x;
            p->circc.y      = p->p_q8.y - BACKGROUND_WIND_CIRCLE_R;
            p->circcooldown = p->circticks + 70;
        }

        if (p->circticks > 0) { // run through circle but keep slowly moving forward
            i32 a     = (Q16_ANGLE_TURN * (p->ticks - p->circticks)) / p->ticks;
            int xx    = sin_q16_fast(a) * BACKGROUND_WIND_CIRCLE_R;
            int yy    = cos_q16_fast(a) * BACKGROUND_WIND_CIRCLE_R;
            p->p_q8.x = p->circc.x + (xx >> 16);
            p->p_q8.y = p->circc.y + (yy >> 16);
            p->circc.x += 200;
            p->circticks--;
        } else {
            p->p_q8 = v2_add(p->p_q8, p->v_q8);
            p->v_q8.y += rngr_i32(-60, +60);
            p->v_q8.y = clamp_i(p->v_q8.y, -400, +400);
        }

        p->pos_q8[p->n] = p->p_q8;
        p->n            = (p->n + 1) & (BG_WIND_PARTICLE_N - 1);
    }

    if (g->n_windparticles < BG_NUM_PARTICLES && rngr_u32(0, 65535) <= 4000) {
        windparticle_s *p = &g->windparticles[g->n_windparticles++];
        p->p_q8.x         = -(10 << 8);
        p->p_q8.y         = rngr_i32(0, g->pixel_y) << 8;
        p->v_q8.x         = rngr_i32(2000, 4000);
        p->v_q8.y         = 0;
        p->circcooldown   = 10;
        p->circticks      = 0;
        p->n              = 0;
        for (int i = 0; i < BG_WIND_PARTICLE_N; i++)
            p->pos_q8[i] = p->p_q8;
    }
}

obj_s *obj_closest_interactable(game_s *g, v2_i32 pos)
{
    u32    interactable_dt = pow2_i32(INTERACTABLE_DIST); // max distance
    obj_s *interactable    = NULL;
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_INTERACTABLE) {
            u32 d = v2_distancesq(pos, o->pos);
            if (d < interactable_dt) {
                interactable_dt = d;
                interactable    = o;
            }
        }
    }
    return interactable;
}

void game_trigger(game_s *g, int triggerID)
{
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        switch (o->ID) {
        default: break;
        }
    }
}

solid_rec_list_s game_solid_recs(game_s *g)
{
    static rec_i32 solidrecs[NUM_OBJ];

    solid_rec_list_s l = {0};
    l.recs             = solidrecs;

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_SOLID) {
            rec_i32 r     = obj_aabb(o);
            l.recs[l.n++] = r;
        }
    }
    return l;
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
    int tx0 = px0 >> 4; // divide by 16 (tile size)
    int ty0 = py0 >> 4;
    int tx1 = px1 >> 4;
    int ty1 = py1 >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        int y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        int y1 = (ty == ty1 ? py1 & 15 : 15);

        for (int tx = tx0; tx <= tx1; tx++) {
            int ID = g->tiles[tx + ty * g->tiles_x].collision;
            if (ID == TILE_EMPTY || NUM_TILE_BLOCKS <= ID) continue;
            if (ID == 1) return 1;
            int x0 = (tx == tx0 ? px0 & 15 : 0);
            int x1 = (tx == tx1 ? px1 & 15 : 15);
            int mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (int py = y0; py <= y1; py++)
                if (g_pxmask_tab[ID * 16 + py] & mk) return 1;
        }
    }
    return 0;
}

bool32 game_traversable(game_s *g, rec_i32 r)
{
    if (tiles_solid(g, r)) return 0;

    for (int n = 0; n < g->obj_nbusy; n++) {
        obj_s *o = g->obj_busy[n];
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(o))) {
            return 0;
        }
    }
    return 1;
}

static hitbox_s *hitbox_strongest(rec_i32 r, hitbox_s *boxes, int n_boxes)
{
    hitbox_s *strongest     = NULL;
    int       strongest_dmg = 0;
    for (int n = 0; n < n_boxes; n++) {
        hitbox_s *hb = &boxes[n];

        if (overlap_rec(r, hb->r) && hb->damage > strongest_dmg) {
            strongest     = hb;
            strongest_dmg = hb->damage;
        }
    }
    return strongest;
}

void game_apply_hitboxes(game_s *g, hitbox_s *boxes, int n_boxes)
{
    for (int n = 0; n < n_boxes; n++) {
        hitbox_s hb = boxes[n];
    }

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s    *o    = g->obj_busy[i];
        rec_i32   aabb = obj_aabb(o);
        hitbox_s *hb   = hitbox_strongest(aabb, boxes, n_boxes);

        if (!hb) continue;
    }
}

bounds_2D_s game_tilebounds_rec(game_s *g, rec_i32 r)
{
    v2_i32 pmin = {r.x, r.y};
    v2_i32 pmax = {r.x + r.w, r.y + r.h};
    return game_tilebounds_pts(g, pmin, pmax);
}

bounds_2D_s game_tilebounds_pts(game_s *g, v2_i32 p0, v2_i32 p1)
{
    bounds_2D_s b;
    b.x1 = max_i(p0.x, 0) >> 4; // div 16
    b.y1 = max_i(p0.y, 0) >> 4;
    b.x2 = min_i(p1.x, g->pixel_x - 1) >> 4;
    b.y2 = min_i(p1.y, g->pixel_y - 1) >> 4;
    return b;
}

bounds_2D_s game_tilebounds_tri(game_s *g, tri_i32 t)
{
    v2_i32 pmin = v2_min3(t.p[0], t.p[1], t.p[2]);
    v2_i32 pmax = v2_max3(t.p[0], t.p[1], t.p[2]);
    return game_tilebounds_pts(g, pmin, pmax);
}

int rtile_pack(int tx, int ty)
{
    return ((tx << 8) | (ty));
}

void rtile_unpack(int ID, int *tx, int *ty)
{
    *tx = (ID >> 8);
    *ty = (ID & 0xFF);
}

int tick_to_index_freq(int tick, int n_frames, int freqticks)
{
    int i = ((tick * n_frames) / freqticks) % n_frames;
    assert(0 <= i && i < n_frames);
    return i;
}

obj_s *obj_solid_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SOLID;
    o->flags |= OBJ_FLAG_SOLID;
    o->w = 32;
    o->h = 24;

    return o;
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
const tri_i32 tilecolliders[GAME_NUM_TILECOLLIDERS] = {
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
    0, 16, 16, 16, 16, 8, // 14
    0, 16, 16, 16, 16, 8,
    0, 0, 16, 0, 16, 8, // 15
    0, 0, 16, 0, 16, 8,
    0, 8, 0, 16, 16, 16, // 16
    0, 8, 0, 16, 16, 16,
    0, 0, 16, 0, 0, 8, // 17
    0, 0, 16, 0, 0, 8,
    16, 0, 16, 16, 8, 16, // 18
    16, 0, 16, 16, 8, 16,
    8, 0, 16, 0, 16, 16, // 19
    8, 0, 16, 0, 16, 16,
    0, 0, 0, 16, 8, 16, // 20
    0, 0, 0, 16, 8, 16,
    0, 0, 8, 0, 0, 16, // 21
    0, 0, 8, 0, 0, 16,
    //
};