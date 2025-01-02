// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "render.h"
#include "util/str.h"

typedef struct {
    u16 tile;
    u16 x;
    u16 y;
} map_prop_tile_s;

typedef struct map_header_s {
    u16 w;
    u16 h;
    u16 n_prop;
    u16 n_obj;
    u16 n_obj_fg;
    u16 n_bg;
    u16 n_fg;
    u32 offs_bg;
    u32 offs_fg;
    u32 offs_obj;
    u32 offs_obj_fg;
    u32 offs_terrain;
    u32 n_rle_terrain;
    u32 offs_bgauto;
    u32 n_rle_bgauto;
    u32 offs_bgtiles;
    u32 n_rle_bgtiles;
    u32 offs_fluids;
    u32 n_rle_fluids;
    u32 bytes_data;
} map_header_s;

typedef struct {
    u8 n; // run length - 1
    u8 b;
} map_rle_u8;

typedef struct {
    u16 n; // run length - 1
    u16 b;
} map_rle_u16;

typedef struct {
    u8 *t;
    i32 w;
    i32 h;
} tilelayer_u8;

typedef struct {
    u16 *t;
    i32  w;
    i32  h;
} tilelayer_u16;

static void map_rle_u8_uncompress(u8 *dst, void *rle_src, u32 n_rle)
{
    map_rle_u8 *r = (map_rle_u8 *)rle_src;
    u8         *d = dst;
    for (u32 n = 0; n < n_rle; n++, r++) {
        for (u32 k = 0; k <= r->n; k++) {
            *d++ = r->b;
        }
    }
}

static void map_rle_u16_uncompress(u16 *dst, void *rle_src, u32 n_rle)
{
    map_rle_u16 *r = (map_rle_u16 *)rle_src;
    u16         *d = dst;
    for (u32 n = 0; n < n_rle; n++, r++) {
        for (u32 k = 0; k <= r->n; k++) {
            *d++ = r->b;
        }
    }
}

static inline i32 map_terrain_pack(i32 type, i32 shape)
{
    return ((type << 8) | shape);
}

static inline i32 map_terrain_type(u16 t)
{
    return (t >> 8);
}

static inline i32 map_terrain_shape(u16 t)
{
    return (t & B16(00000000, 11111111));
}

enum {
    MAP_PROP_NULL,
    MAP_PROP_INT,
    MAP_PROP_FLOAT,
    MAP_PROP_STRING,
    MAP_PROP_OBJ,
    MAP_PROP_BOOL,
    MAP_PROP_POINT,
    MAP_PROP_ARRAY
};

typedef struct {
    u16  bytes;
    u16  type;
    char name[32];
    union {
        i32 n; // array: num elements
        i32 i;
        f32 f;
        u32 b;
        u32 o;
        u32 p;
    } u;
} map_prop_s;

typedef struct {
    void *p;
    i32   n;
} map_properties_s;

typedef struct {
    char c[64];
} map_string_s;

enum {
    TILE_FLIP_DIA = 1 << 0,
    TILE_FLIP_Y   = 1 << 1,
    TILE_FLIP_X   = 1 << 2,
};

static const v2_i8 g_autotile_coords[256];

static inline void map_proptile_decode(u16 t, i32 *tx, i32 *ty, i32 *f)
{
    *f  = (t >> 14);
    *tx = t & B8(01111111);
    *ty = (t >> 7) & B8(01111111);
}

static void             map_at_background(g_s *g, tilelayer_u8 tiles, i32 x, i32 y);
static void             map_at_terrain(g_s *g, tilelayer_u16 tiles, i32 x, i32 y);
static map_prop_s      *map_prop_get(map_properties_s p, const char *name);
static map_properties_s map_obj_properties(map_obj_s *mo);
static void             map_obj_parse(g_s *g, map_obj_s *o);
static void             map_obj_prop_fg_parse(g_s *g, map_obj_s *o);
//
static bool32           at_types_blending(i32 a, i32 b);

#define map_prop_strs(P, NAME, B) map_prop_str(P, NAME, B, sizeof(B))
static bool32 map_prop_str(map_properties_s p, const char *name, void *b, u32 bs);
static i32    map_prop_i32(map_properties_s p, const char *name);
static f32    map_prop_f32(map_properties_s p, const char *name);
static bool32 map_prop_bool(map_properties_s p, const char *name);
static v2_i16 map_prop_pt(map_properties_s p, const char *name);

static void map_obj_parse(g_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "Coin")) {
        coin_load(g, o);
    } else if (str_eq_nc(o->name, "Rotor")) {
        rotor_load(g, o);
    } else if (str_eq_nc(o->name, "Savepoint")) {
        savepoint_load(g, o);
    } else if (str_eq_nc(o->name, "Spiderboss")) {
        spiderboss_load(g, o);
    } else if (str_eq_nc(o->name, "Door")) {
        door_load(g, o);
    } else if (str_eq_nc(o->name, "Trampoline")) {
        trampoline_load(g, o);
    } else if (str_eq_nc(o->name, "Windarea")) {
        windarea_load(g, o);
    } else if (str_eq_nc(o->name, "Waterleaf")) {
        waterleaf_load(g, o);
    } else if (str_eq_nc(o->name, "Chest")) {
        chest_load(g, o);
    } else if (str_eq_nc(o->name, "Fallingblock")) {
        fallingblock_load(g, o);
    } else if (str_eq_nc(o->name, "Light")) {
        light_load(g, o);
    } else if (str_eq_nc(o->name, "Stompfloor")) {
        stompable_block_load(g, o);
    } else if (str_eq_nc(o->name, "Staminarestorer")) {
        staminarestorer_load(g, o);
    } else if (str_eq_nc(o->name, "Flyblob")) {
        flyblob_load(g, o);
    } else if (str_eq_nc(o->name, "Switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "Budplant")) {
        budplant_load(g, o);
    } else if (str_eq_nc(o->name, "Steamplatform")) {
        steam_platform_load(g, o);
    } else if (str_eq_nc(o->name, "Wallworm")) {
        wallworm_load(g, o);
    } else if (str_eq_nc(o->name, "Hookplant")) {
        hookplant_load(g, o);
    } else if (str_eq_nc(o->name, "Hero_Powerup")) {
        hero_powerup_obj_load(g, o);
    } else if (str_eq_nc(o->name, "NPC")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "Floater")) {
        floater_load(g, o);
    } else if (str_eq_nc(o->name, "Crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "Caterpillar")) {
        crawler_caterpillar_load(g, o);
    } else if (str_eq_nc(o->name, "Pushblock")) {
        pushblock_load(g, o);
    } else if (str_eq_nc(o->name, "Sign")) {
        sign_load(g, o);
    } else if (str_eq_nc(o->name, "Shroomy")) {
        shroomy_load(g, o);
    } else if (str_eq_nc(o->name, "Toggleblock")) {
        toggleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Moving_Plat")) {
        movingplatform_load(g, o);
    } else if (str_contains(o->name, "Spikes_")) {
        spikes_load(g, o);
    } else if (str_eq_nc(o->name, "Crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Teleport")) {
        teleport_load(g, o);
    } else if (str_eq_nc(o->name, "Stalactite")) {
        stalactite_load(g, o);
    } else if (str_eq_nc(o->name, "Walker")) {
        walker_load(g, o);
    } else if (str_eq_nc(o->name, "Flyer")) {
        flyer_load(g, o);
    } else if (str_eq_nc(o->name, "Clockpulse")) {
        clockpulse_load(g, o);
    } else if (str_eq_nc(o->name, "Triggerarea")) {
        triggerarea_load(g, o);
    } else if (str_eq_nc(o->name, "Hooklever")) {
        hooklever_load(g, o);
    } else if (str_eq_nc(o->name, "Cam_Attractor")) {
        camattractor_static_load(g, o);
    } else if (str_eq_nc(o->name, "Respawn")) {
        assert(g->n_respawns < NUM_RESPAWNS);
        v2_i16 *rp = &g->respawns[g->n_respawns++];
        rp->x      = o->x + o->w / 2;
        rp->y      = o->y + o->h;
    } else if (str_eq_nc(o->name, "Cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "Locked_X");
        cam->locked_y = map_obj_bool(o, "Locked_Y");
        if (cam->locked_x) {
            cam->pos_q8.x = (o->x + PLTF_DISPLAY_W / 2) << 8;
        }
        if (cam->locked_y) {
            cam->pos_q8.y = (o->y + PLTF_DISPLAY_H / 2) << 8;
        }
    } else if (str_eq_nc(o->name, "Water")) {
        i32 x1 = (o->x) >> 4;
        i32 y1 = (o->y) >> 4;
        i32 x2 = (o->x + o->w - 1) >> 4;
        i32 y2 = (o->y + o->h - 1) >> 4;

        for (i32 y = y1; y <= y2; y++) {
            for (i32 x = x1; x <= x2; x++) {
                g->tiles[x + y * g->tiles_x].type |= TILE_WATER_MASK;
            }
        }
    }
}

static void map_obj_prop_fg_parse(g_s *g, map_obj_s *o)
{
    foreground_prop_s *p = &g->foreground_props[g->n_foreground_props++];
    *p                   = (foreground_prop_s){0};
    p->pos.x             = o->x;
    p->pos.y             = o->y;
    p->tr                = asset_texrec(TEXID_TILESET_PROPS,
                                        (i32)o->tx << 4,
                                        (i32)o->ty << 4,
                                        (i32)o->tw << 4,
                                        (i32)o->th << 4);
}

void game_load_map(g_s *g, const char *mapfile)
{
    g->save.coins += g->coins_added;
    for (obj_each(g, o)) {
        if (o->ID != OBJ_ID_HERO) {
            obj_delete(g, o);
        }
    }
    objs_cull_to_delete(g);
    mclr(g->tiles, sizeof(g->tiles));
    mclr(g->rtiles, sizeof(g->rtiles));
    mclr(g->fluid_streams, sizeof(g->fluid_streams));
    g->n_foreground_props = 0;
    g->n_grass            = 0;
    g->n_deco_verlet      = 0;
    g->rope.active        = 0;
    g->particles.n        = 0;
    g->ocean.active       = 0;
    g->cam.locked_x       = 0;
    g->cam.locked_y       = 0;
    g->n_respawns         = 0;
    g->coins_added        = 0;
    g->coins_added_ticks  = 0;
    g->areaname.fadeticks = 1;

    game_allocator_reset(g);
    str_cpy(g->areaname.filename, mapfile);
    g->map_worldroom = map_world_find_room(&g->map_world, mapfile);
    assert(g->map_worldroom);

    // READ FILE ===============================================================
    char filepath[128];
    str_cpy(filepath, FILEPATH_MAP);
    str_append(filepath, mapfile);
    str_append(filepath, ".map");

    void *mapf = pltf_file_open_r(filepath);
    if (!mapf) {
        pltf_log("Can't load map file! %s\n", filepath);
        BAD_PATH
    }

    map_header_s hd = {0};
    pltf_file_r(mapf, &hd, sizeof(map_header_s));

    spm_push();
    byte *mapdata = spm_alloc_aligned(hd.bytes_data, 4);
    pltf_file_r(mapf, mapdata, hd.bytes_data);

    const i32 w = hd.w;
    const i32 h = hd.h;
    g->tiles_x  = w;
    g->tiles_y  = h;
    g->pixel_x  = w << 4;
    g->pixel_y  = h << 4;
    assert((w * h) <= NUM_TILES);

    // PROPERTIES ==============================================================
    map_properties_s mp = {&mapdata[0], hd.n_prop};

    map_prop_strs(mp, "Name", g->areaname.label);
    prerender_area_label(g);
    area_setup(g, &g->area, map_prop_i32(mp, "Area_ID"));
    g->dark          = map_prop_bool(mp, "Dark");
    char musname[32] = {0};
    map_prop_strs(mp, "Music", musname);
    mus_play(musname);

    i32 room_y1 = g->map_worldroom->y;
    i32 room_y2 = g->map_worldroom->y + g->map_worldroom->h;
    if (room_y1 < 0 && 0 < room_y2 && map_prop_bool(mp, "Ocean")) {
        g->ocean.y      = -room_y1;
        g->ocean.active = 1;
    }

    // TERRAIN =================================================================
    spm_push();
    tilelayer_u16 l_terrain = {spm_alloct(u16, w * h), w, h};
    map_rle_u16_uncompress(l_terrain.t, &mapdata[hd.offs_terrain], hd.n_rle_terrain);

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            i32 k       = x + y * w;
            i32 tt      = l_terrain.t[k];
            i32 ttshape = map_terrain_shape(tt);
            switch (ttshape) {
            case TILE_CLIMBWALL: {
                g->tiles[k].collision              = TILE_CLIMBWALL;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 7;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 21;
                break;
            }
            case TILE_LADDER: {
                i32 ty = 19 + (y & 3);
                if (0 < y &&
                    map_terrain_shape(l_terrain.t[k - w]) != TILE_LADDER) {
                    ty = 18;
                }

                g->tiles[k].collision                  = TILE_LADDER;
                g->rtiles[TILELAYER_PROP_BG][k].tx     = 5;
                g->rtiles[TILELAYER_PROP_BG][k].ty     = ty;
                g->rtiles[TILELAYER_PROP_BG][k - 1].tx = 4;
                g->rtiles[TILELAYER_PROP_BG][k - 1].ty = ty;
                g->rtiles[TILELAYER_PROP_BG][k + 1].tx = 6;
                g->rtiles[TILELAYER_PROP_BG][k + 1].ty = ty;
                break;
            }
            case TILE_LADDER_ONE_WAY: {
                g->tiles[k].collision              = TILE_LADDER_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 2;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 22;
                break;
            }
            case TILE_ONE_WAY: {
                g->tiles[k].collision              = TILE_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 1;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 22;
                break;
            }
            default: map_at_terrain(g, l_terrain, x, y); break;
            }
        }
    }
    spm_pop();

    // BACKGROUND ==============================================================
    spm_push();
    tilelayer_u8 l_bg = {spm_alloc(w * h), w, h};
    map_rle_u8_uncompress(l_bg.t, &mapdata[hd.offs_bgauto], hd.n_rle_bgauto);

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            map_at_background(g, l_bg, x, y);
        }
    }
    spm_pop();

    // FLUIDS ==================================================================
    map_rle_u8_uncompress(g->fluid_streams, &mapdata[hd.offs_fluids], hd.n_rle_fluids);

    // TILES_BG ================================================================
    spm_push();
    u16 *t_bg = (u16 *)spm_alloc(sizeof(u16) * w * h);
    map_rle_u16_uncompress(t_bg, &mapdata[hd.offs_bgtiles], hd.n_rle_bgtiles);

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            i32 i  = x + y * w;
            i32 tt = t_bg[i];
            if (!tt) continue;
            rtile_s *rt = &g->rtiles[TILELAYER_BG_TILE][i];
            i32      tx, ty, fl;
            map_proptile_decode(tt, &tx, &ty, &fl);
            rt->tx = tx;
            rt->ty = ty;
        }
    }
    spm_pop();

    // OBJECTS =================================================================
    byte *obj_ptr = &mapdata[hd.offs_obj];

    for (i32 n = 0; n < hd.n_obj; n++) {
        map_obj_s *o = (map_obj_s *)obj_ptr;
        map_obj_parse(g, o);
        obj_ptr += o->bytes;
    }

    spm_pop();
    pltf_file_close(mapf);
}

void game_prepare_new_map(g_s *g)
{
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    objs_animate(g);
    aud_allow_playing_new_snd(1);
    cam_init_level(g, &g->cam);
}

static bool32 autotile_bg_is(tilelayer_u8 tiles, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 1;
    return (0 < tiles.t[u + v * tiles.w]);
}

static bool32 at_type_dark(i32 t)
{
    return (TILE_TYPE_DARK_BEG <= t && t <= TILE_TYPE_DARK_END);
}

static bool32 at_type_bright(i32 t)
{
    return (TILE_TYPE_BRIGHT_BEG <= t && t <= TILE_TYPE_BRIGHT_END);
}

// if tile type a connects to a neighbour tiletype b
static bool32 at_types_blending(i32 a, i32 b)
{
    if (a == b) return 1;
    if (b == TILE_TYPE_INVISIBLE_NON_CONNECTING) return 0;
    if (b == TILE_TYPE_INVISIBLE_CONNECTING) return 1;
    if (a == TILE_TYPE_THORNS) return 1;
    if (at_type_dark(a) && at_type_dark(b)) return 1;
    if (at_type_bright(a) && at_type_bright(b)) return 1;

    return 0;
}

static bool32 autotile_terrain_is(tilelayer_u16 tiles, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 1;

    i32 a = tiles.t[x + y * tiles.w];
    i32 b = tiles.t[u + v * tiles.w];
    if (!at_types_blending(map_terrain_type(a), map_terrain_type(b))) return 0;

    switch (map_terrain_shape(b)) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == -1 && sy == -1));
    case TILE_SLOPE_45_1: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == -1 && sy == +1));
    case TILE_SLOPE_45_2: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == +1 && sy == -1));
    case TILE_SLOPE_45_3: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == +1 && sy == +1));
    default: break;
    }
    return 0;
}

// bits for marching squares (neighbours)
// 128  1 2
//  64 XX 4
//  32 16 8
enum {
    AT_N  = B8(00000001),
    AT_E  = B8(00000100),
    AT_S  = B8(00010000),
    AT_W  = B8(01000000),
    AT_NE = B8(00000010),
    AT_SE = B8(00001000),
    AT_SW = B8(00100000),
    AT_NW = B8(10000000)
};

static void map_at_background(g_s *g, tilelayer_u8 tiles, i32 x, i32 y)
{
    i32 index = x + y * tiles.w;
    i32 tile  = tiles.t[index];
    if (tile == 0) return;

    u32 march = 0;
    if (autotile_bg_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_bg_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_bg_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_bg_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_bg_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_bg_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_bg_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_bg_is(tiles, x, y, -1, -1)) march |= AT_NW;

    v2_i8    coords = g_autotile_coords[march];
    rtile_s *rtile  = &g->rtiles[TILELAYER_BG][index];
    rtile->tx       = coords.x;
    rtile->ty       = coords.y + tile * 8;
}

static u32 map_marching_squares(tilelayer_u16 tiles, i32 x, i32 y)
{
    if (!(0 <= x && x < tiles.w && 0 <= y && y < tiles.h)) return 0xFF;
    u32 tile = tiles.t[x + y * tiles.w];
    if (map_terrain_type(tile) < 3) return 0;

    u32 march = 0;
    if (autotile_terrain_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_terrain_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_terrain_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_terrain_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_terrain_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_terrain_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_terrain_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_terrain_is(tiles, x, y, -1, -1)) march |= AT_NW;
    return march;
}

static bool32 map_dual_border(tilelayer_u16 tiles, i32 x, i32 y,
                              i32 sx, i32 sy,
                              i32 type, u32 march)
{
    // tile types without dual tiles
    switch (map_terrain_type(tiles.t[x + y * tiles.w])) {
    case TILE_TYPE_DARK_STONE: break;
    default: return 0;
    }

    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 0;
    i32 t = tiles.t[u + v * tiles.w];
    if (map_terrain_type(t) == 6) return 0;

    u32 seed = ((x | u) + ((y | v)));
    u32 r    = rngs_u32(&seed);
    if (r < 0x80000000U) return 0;

    if (t != map_terrain_pack(type, TILE_BLOCK)) return 0;
    return (march == map_marching_squares(tiles, u, v));
}

static void map_at_terrain(g_s *g, tilelayer_u16 tiles, i32 x, i32 y)
{
    i32     index  = x + y * tiles.w;
    i32     tile   = tiles.t[index];
    tile_s *rtile  = &g->tiles[index];
    i32     ttype  = map_terrain_type(tile);
    i32     tshape = map_terrain_shape(tile);

    switch (ttype) {
    case TILE_TYPE_INVISIBLE_NON_CONNECTING:
    case TILE_TYPE_INVISIBLE_CONNECTING: rtile->collision = tshape;
    case 0: return;
    default: break;
    }

    rtile->type = ttype;

    switch (ttype) {
    case TILE_TYPE_THORNS:
        rtile->collision = TILE_SPIKES;
        break;
    default:
        rtile->collision = tshape;
        break;
    }

    u32   m      = map_marching_squares(tiles, x, y);
    v2_i8 tcoord = {0, ((i32)ttype - 2) << 3};
    v2_i8 coords = g_autotile_coords[m];

    switch (tshape) {
    case TILE_BLOCK: {
        i32 n_vari = 1; // number of variations in that tileset

#if 1
        switch (ttype) {
        case TILE_TYPE_DARK_STONE:
        case TILE_TYPE_DARK_LEAVES:
        case TILE_TYPE_BRIGHT_STONE:
            n_vari = 3;
            break;
        }
#endif

        i32 vari = rngr_i32(0, n_vari - 1);
        switch (m) { // coordinates of variation tiles
        case 17: {   // vertical
            static const v2_i8 altc_17[3] = {{7, 3}, {7, 4}, {7, 6}};
            coords                        = altc_17[vari];
            break;
        }
        case 31: { // left border
            static const v2_i8 altc_31[3] = {{0, 4}, {0, 5}, {5, 3}};

            if (0) {
            } else if ((y & 1) == 0 && map_dual_border(tiles, x, y, 0, -1, ttype, m)) {
                coords = (v2_i8){9, 5};

            } else if ((y & 1) == 1 && map_dual_border(tiles, x, y, 0, +1, ttype, m)) {
                coords = (v2_i8){8, 5};
            } else {
                coords = altc_31[0];
            }
            break;
        }
        case 199: { // bot border
            static const v2_i8 altc199[3] = {{4, 2}, {1, 6}, {3, 6}};

            if (0) {
            } else if ((x & 1) == 0 && map_dual_border(tiles, x, y, -1, 0, ttype, m)) {
                coords = (v2_i8){9, 7};
            } else if ((x & 1) == 1 && map_dual_border(tiles, x, y, +1, 0, ttype, m)) {
                coords = (v2_i8){8, 7};
            } else {
                coords = altc199[vari];
            }
            break;
        }
        case 241: { // right border
            static const v2_i8 altc241[3] = {{6, 1}, {6, 3}, {2, 4}};

            if (0) {
            } else if ((y & 1) == 0 && map_dual_border(tiles, x, y, 0, -1, ttype, m)) {
                coords = (v2_i8){9, 6};
            } else if ((y & 1) == 1 && map_dual_border(tiles, x, y, 0, +1, ttype, m)) {
                coords = (v2_i8){8, 6};
            } else {
                coords = altc241[vari];
            }
            break;
        }
        case 68: { // horizontal
            static const v2_i8 altc_68[3] = {{3, 7}, {4, 7}, {6, 7}};
            coords                        = altc_68[vari];
            break;
        }
        case 124: { // top border
            static const v2_i8 altc124[3] = {{4, 0}, {5, 0}, {3, 5}};

            if (0) {
            } else if ((x & 1) == 0 && map_dual_border(tiles, x, y, -1, 0, ttype, m)) {
                coords = (v2_i8){9, 4};
            } else if ((x & 1) == 1 && map_dual_border(tiles, x, y, +1, 0, ttype, m)) {
                coords = (v2_i8){8, 4};
            } else {
                coords = altc124[vari];
            }
            break;
        }
        case 255: { // mid
            static const v2_i8 altc255[3] = {{4, 1}, {1, 4}, {5, 1}};
            coords                        = altc255[vari];
            break;
        }
        }
        tcoord = v2_i8_add(tcoord, coords);

        if (0 < y && (ttype == 3) &&
            map_terrain_type(tiles.t[x + (y - 1) * tiles.w]) == 0) {
            grass_put(g, x, y - 1);
        }
        break;
    }
    case TILE_SLOPE_45_0:
    case TILE_SLOPE_45_1:
    case TILE_SLOPE_45_2:
    case TILE_SLOPE_45_3: {
        // Y row of tile
        static const u8 shapei[4]    = {0, 2, 1, 3};
        // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
        static const u8 nmasks[4][3] = {{AT_E, AT_S, AT_SE},
                                        {AT_E, AT_N, AT_NE},
                                        {AT_W, AT_S, AT_SW},
                                        {AT_W, AT_N, AT_NW}};

        i32 i  = tshape - TILE_SLOPE_45_0;
        i32 xn = (m & nmasks[i][0]) != 0;                      // neighbour x
        i32 yn = (m & nmasks[i][1]) != 0;                      // neighbour y
        i32 cn = (m & nmasks[i][2]) != 0;                      // neighbour dia
        tcoord.x += 8 + (xn && yn && cn ? 4 : xn | (yn << 1)); // index variant
        tcoord.y += shapei[i];                                 // index shape
        break;
    }
    }

    rtile->tx = tcoord.x;
    rtile->ty = tcoord.y;
}

static map_prop_s *map_prop_get(map_properties_s p, const char *name)
{
    if (!p.p) return 0;
    char *ptr = (char *)p.p;
    for (i32 n = 0; n < p.n; n++) {
        map_prop_s *prop = (map_prop_s *)ptr;
        if (str_eq_nc(prop->name, name)) {
            return prop;
        }
        ptr += prop->bytes;
    }
    pltf_log("No property: %s\n", name);
    return 0;
}

static bool32 map_prop_str(map_properties_s p, const char *name, void *b, u32 bs)
{
    if (!b || bs == 0) return 0;
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_STRING) return 0;
    char *s       = (char *)(prop + 1);
    char *d       = (char *)b;
    i32   written = 0;
    while (written < (i32)bs) {
        *d = *s;
        if (*s == '\0') break;
        s++;
        d++;
        written++;
    }
    ((char *)b)[bs - 1] = '\0';
    return 1;
}

static i32 map_prop_i32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_INT) return 0;
    return prop->u.i;
}

static f32 map_prop_f32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_FLOAT) return 0.f;
    return prop->u.f;
}

static bool32 map_prop_bool(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_BOOL) return 0;
    return prop->u.b;
}

static v2_i16 map_prop_pt(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_POINT) return (v2_i16){0};
    v2_i16 pt = {(i16)(prop->u.p & 0xFFFFU), (i16)(prop->u.p >> 16)};
    return pt;
}

static map_properties_s map_obj_properties(map_obj_s *mo)
{
    map_properties_s p = {mo + 1, mo->n_prop};
    return p;
}

bool32 map_obj_has_nonnull_prop(map_obj_s *mo, const char *name)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (!prop) return 0;
    return (prop->type != MAP_PROP_NULL);
}

bool32 map_obj_str(map_obj_s *mo, const char *name, void *b, u32 bs)
{
    return map_prop_str(map_obj_properties(mo), name, b, bs);
}

i32 map_obj_i32(map_obj_s *mo, const char *name)
{
    return map_prop_i32(map_obj_properties(mo), name);
}

f32 map_obj_f32(map_obj_s *mo, const char *name)
{
    return map_prop_f32(map_obj_properties(mo), name);
}

bool32 map_obj_bool(map_obj_s *mo, const char *name)
{
    return map_prop_bool(map_obj_properties(mo), name);
}

v2_i16 map_obj_pt(map_obj_s *mo, const char *name)
{
    return map_prop_pt(map_obj_properties(mo), name);
}

void *map_obj_arr(map_obj_s *mo, const char *name, i32 *num)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (!prop || prop->type != MAP_PROP_ARRAY) return 0;
    *num = prop->u.n;
    return (prop + 1);
}

void map_world_load(map_world_s *world, const char *worldfile)
{
    if (!world || !worldfile || worldfile[0] == '\0') return;

    FILEPATH_GEN(filepath, FILEPATH_MAP, worldfile);
    void *f = pltf_file_open_r(filepath);
    pltf_file_r(f, world, sizeof(map_world_s));
    pltf_file_close(f);
}

map_worldroom_s *map_world_overlapped_room(map_world_s *world, map_worldroom_s *cur, rec_i32 r)
{
    for (u32 i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (room == cur) continue;
        rec_i32 rr = {room->x, room->y, room->w, room->h};
        if (overlap_rec(rr, r))
            return room;
    }
    return NULL;
}

map_worldroom_s *map_world_find_room(map_world_s *world, const char *mapfile)
{
    for (u32 i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (str_eq(room->filename, mapfile))
            return room;
    }
    return NULL;
}

map_worldroom_s *map_worldroom_by_objID(map_world_s *world, u32 objID)
{
    i32 i = (objID >> 8) - 1;
    pltf_log("get room %i\n", i);
    map_worldroom_s *r = &world->rooms[i];
    return r;
}

// clang-format off
static const v2_i8 g_autotile_coords[256] = {
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 2}, {7, 5}, {7, 2}, {7, 5}, {2, 0}, {2, 1}, {2, 0}, {5, 6},
    {7, 2}, {7, 5}, {7, 2}, {7, 5}, {3, 0}, {1, 2}, {3, 0}, {3, 1},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {4, 3}, {4, 5}, {4, 3}, {4, 5}, {6, 0}, {6, 5}, {6, 0}, {3, 2},
    {4, 3}, {4, 5}, {4, 3}, {4, 5}, {4, 0}, {2, 2}, {4, 0}, {1, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 2}, {6, 4}, {7, 2}, {6, 4}, {2, 0}, {4, 4}, {2, 0}, {2, 6},
    {7, 2}, {6, 4}, {7, 2}, {6, 4}, {3, 0}, {3, 3}, {3, 0}, {5, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {4, 3}, {6, 1}, {4, 3}, {6, 1}, {6, 0}, {6, 2}, {6, 0}, {2, 3},
    {4, 3}, {6, 1}, {4, 3}, {6, 1}, {4, 0}, {2, 5}, {4, 0}, {4, 1}};
// clang-format on