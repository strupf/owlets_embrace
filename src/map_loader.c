// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "render/render.h"
#include "util/str.h"

enum {
    AUTOTILE_TYPE_NONE,
    AUTOTILE_TYPE_FAKE_1,
    AUTOTILE_TYPE_FAKE_2,
    //
    AUTOTILE_TYPE_BRICK       = 3,
    AUTOTILE_TYPE_BRICK_SMALL = 4,
    AUTOTILE_TYPE_DIRT        = 5,
    AUTOTILE_TYPE_STONE       = 6,
    AUTOTILE_TYPE_1           = 7,
    AUTOTILE_TYPE_2           = 8,
    AUTOTILE_TYPE_3           = 9,
    AUTOTILE_TYPE_DIRT_DARK   = 10,
    AUTOTILE_TYPE_HUGE        = 11,
    //
    NUM_AUTOTILE_TYPES
};

typedef struct {
    i32 w;
    i32 h;
    i32 n_obj;
    i32 n_prop;
    u32 bytes_prop;
    u32 bytes_tiles_bg;
    u32 bytes_tiles_terrain;
    u32 bytes_tiles_prop_bg;
    u32 bytes_tiles_prop_fg;
    u32 bytes_obj;
} map_header_s;

typedef struct {
    u8 *tiles;
    int w;
    int h;
} tilelayer_bg_s;

typedef struct {
    u8 type;
    u8 shape;
} map_terraintile_s;

typedef struct {
    map_terraintile_s *tiles;
    int                w;
    int                h;
} tilelayer_terrain_s;

enum {
    MAP_PROP_INT,
    MAP_PROP_FLOAT,
    MAP_PROP_STRING,
    MAP_PROP_OBJ,
    MAP_PROP_BOOL,
    MAP_PROP_POINT,
    MAP_PROP_ARRAY,
};

typedef struct {
    u32  bytes;
    u32  type;
    char name[32];
    union {
        i32    n; // array: num elements
        i32    i;
        f32    f;
        u32    b;
        u32    o;
        v2_i16 p;
    } u;
} map_prop_s;

typedef struct {
    int   n;
    void *p;
} map_properties_s;

typedef struct {
    char c[64];
} map_string_s;

enum {
    TILE_FLIP_DIA = 1 << 0,
    TILE_FLIP_Y   = 1 << 1,
    TILE_FLIP_X   = 1 << 2,
};

static const u8 g_autotilemarch[256];

static inline void map_proptile_decode(u16 t, int *tx, int *ty, int *f)
{
    *f  = (t >> 14);
    *tx = t & B8(01111111);
    *ty = (t >> 7) & B8(01111111);
}

static void             map_at_background(game_s *g, tilelayer_bg_s tiles, int x, int y);
static void             map_at_terrain(game_s *g, tilelayer_terrain_s tiles, int x, int y);
static map_prop_s      *map_prop_get(map_properties_s p, const char *name);
static map_properties_s map_obj_properties(map_obj_s *mo);
//
static bool32           at_types_blending(int a, int b);

#define map_prop_strs(P, NAME, B) map_prop_str(P, NAME, B, sizeof(B))
static void   map_prop_str(map_properties_s p, const char *name, void *b, usize bs);
static i32    map_prop_i32(map_properties_s p, const char *name);
static f32    map_prop_f32(map_properties_s p, const char *name);
static bool32 map_prop_bool(map_properties_s p, const char *name);
static v2_i16 map_prop_pt(map_properties_s p, const char *name);

static void map_obj_parse(game_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "Switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "Heroupgrade")) {
        heroupgrade_load(g, o);
    } else if (str_eq_nc(o->name, "Boat")) {
        boat_load(g, o);
    } else if (str_eq_nc(o->name, "NPC")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "Floater")) {
        floater_load(g, o);
    } else if (str_eq_nc(o->name, "Crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "Caterpillar")) {
        crawler_caterpillar_load(g, o);
    } else if (str_eq_nc(o->name, "Charger")) {
        charger_load(g, o);
    } else if (str_eq_nc(o->name, "Pushable_Box")) {
        pushablebox_load(g, o);
    } else if (str_eq_nc(o->name, "Sign")) {
        sign_load(g, o);
    } else if (str_eq_nc(o->name, "Shroomy")) {
        shroomy_load(g, o);
    } else if (str_eq_nc(o->name, "Toggleblock")) {
        toggleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Moving_Plat")) {
        movingplatform_load(g, o);
    } else if (str_eq_nc(o->name, "Door_Swing")) {
        swingdoor_load(g, o);
    } else if (str_contains(o->name, "Spikes_")) {
        spikes_load(g, o);
    } else if (str_eq_nc(o->name, "Crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Carrier")) {
        carrier_load(g, o);
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
    } else if (str_eq_nc(o->name, "Trigger_Area")) {
        triggerarea_load(g, o);
    } else if (str_eq_nc(o->name, "Hooklever")) {
        hooklever_load(g, o);
    } else if (str_contains(o->name, "Wiggle")) {
        wiggle_deco_s *wd = &g->wiggle_deco[g->n_wiggle_deco++];
        *wd               = (wiggle_deco_s){0};
        if (str_eq_nc(o->name, "Wiggle2")) {
            wd->tr     = asset_texrec(TEXID_WIGGLE_DECO, 0, 64, 64, 64);
            wd->r      = (rec_i32){o->x - o->w / 2, o->y - o->h, o->w, o->h};
            wd->offs.x = -16;
            wd->offs.y = -48;
        } else if (str_eq_nc(o->name, "Wiggle1")) {
            wd->tr     = asset_texrec(TEXID_WIGGLE_DECO, 0, 0, 64, 64);
            wd->r      = (rec_i32){o->x - o->w / 2, o->y - o->h, o->w, o->h};
            wd->offs.x = -16;
            wd->offs.y = -48;
        }

    } else if (str_eq_nc(o->name, "Cam_Attractor")) {
        v2_i32 p                                 = {o->x, o->y};
        g->cam.attractors[g->cam.n_attractors++] = p;
    } else if (str_eq_nc(o->name, "Respawn")) {
        assert(g->n_respawns < NUM_RESPAWNS);
        v2_i32 *rp = &g->respawns[g->n_respawns++];
        rp->x      = o->x + o->w / 2;
        rp->y      = o->y + o->h;
    } else if (str_eq_nc(o->name, "Cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "Locked_X");
        cam->locked_y = map_obj_bool(o, "Locked_Y");
        if (cam->locked_x) {
            cam->pos.x = (f32)(o->x + SYS_DISPLAY_W / 2);
        }
        if (cam->locked_y) {
            cam->pos.y = (f32)(o->y + SYS_DISPLAY_H / 2);
        }
    } else if (str_eq_nc(o->name, "Water")) {
        int x1 = (o->x) >> 4;
        int y1 = (o->y) >> 4;
        int x2 = (o->x + o->w - 1) >> 4;
        int y2 = (o->y + o->h - 1) >> 4;

        for (int y = y1; y <= y2; y++) {
            for (int x = x1; x <= x2; x++) {
                g->tiles[x + y * g->tiles_x].type |= TILE_WATER_MASK;
            }
        }
    }
}

void game_load_map(game_s *g, const char *mapfile)
{
    g->save.coins += g->coins_added;
    g->coins_added         = 0;
    g->coins_added_ticks   = 0;
    g->obj_ndelete         = 0;
    g->n_objrender         = 0;
    g->obj_head_busy       = NULL;
    g->obj_head_free       = &g->obj_raw[0];
    g->obj_head_free->next = NULL;

    for (int n = 0; n < NUM_OBJ; n++) {
        obj_s *o         = &g->obj_raw[n];
        o->GID.index     = n;
        o->GID.gen       = 1;
        o->next          = g->obj_head_free;
        g->obj_head_free = o;
    }

    for (int n = 0; n < NUM_OBJ_TAGS; n++) {
        g->obj_tag[n] = NULL;
    }
    g->n_grass              = 0;
    g->hero_mem.rope_active = 0;
    g->particles.n          = 0;
    g->ocean.active         = 0;
    g->n_coinparticles      = 0;
    g->n_enemy_decals       = 0;
    g->n_wiggle_deco        = 0;
    g->cam.locked_x         = 0;
    g->cam.locked_y         = 0;
    g->cam.n_attractors     = 0;
    g->n_respawns           = 0;
    g->hero_mem             = (hero_s){0};
    marena_init(&g->arena, g->mem, sizeof(g->mem));
    memset(g->tiles, 0, sizeof(g->tiles));
    memset(g->rtiles, 0, sizeof(g->rtiles));
    g->areaname.fadeticks = 1;

    strcpy(g->areaname.filename, mapfile);
    g->map_worldroom = map_world_find_room(&g->map_world, mapfile);
    sys_printf("%s | %s\n", g->map_worldroom->filename, mapfile);

    spm_push();

    // READ FILE ===============================================================
    map_header_s header = {0};

    FILEPATH_GEN(filepath, FILEPATH_MAP, mapfile);
    str_append(filepath, ".map");
    void *mapf = sys_file_open(filepath, SYS_FILE_R);
    sys_file_read(mapf, &header, sizeof(map_header_s));

    const int w = header.w;
    const int h = header.h;
    g->tiles_x  = w;
    g->tiles_y  = h;
    g->pixel_x  = w << 4;
    g->pixel_y  = h << 4;
    assert((w * h) <= NUM_TILES);
    sys_printf("num tiles: %i\n", w * h);

    // PROPERTIES ==============================================================
    spm_push();
    map_properties_s mp = {0};
    mp.p                = spm_alloc(header.bytes_prop);
    mp.n                = header.n_prop;
    sys_file_read(mapf, mp.p, header.bytes_prop);
    map_prop_strs(mp, "Name", g->areaname.label);
    prerender_area_label(g);

    area_setup(g, &g->area, map_prop_i32(mp, "Area_ID"));

    char musname[64] = {0};
    map_prop_strs(mp, "Music", musname);
    char muspath[128] = {0};
    str_append(muspath, FILEPATH_MUS);
    str_append(muspath, musname);
    str_append(muspath, ".aud");
    mus_fade_to(muspath, 50, 50);

    int room_y1 = g->map_worldroom->y;
    int room_y2 = g->map_worldroom->y + g->map_worldroom->h;
    if (room_y1 < 0 && 0 < room_y2 && map_prop_bool(mp, "Ocean")) {
        g->ocean.y      = -room_y1;
        g->ocean.active = 1;
    }

    spm_pop();

    // TERRAIN =================================================================
    spm_push();
    tilelayer_terrain_s layer_terrain = {0};

    layer_terrain.w     = w;
    layer_terrain.h     = h;
    layer_terrain.tiles = (map_terraintile_s *)spm_alloc(header.bytes_tiles_terrain);
    sys_file_read(mapf, layer_terrain.tiles, header.bytes_tiles_terrain);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int               k  = x + y * w;
            map_terraintile_s tt = layer_terrain.tiles[k];
            switch (tt.shape) {
            case TILE_LADDER: {
                g->tiles[k].collision              = TILE_LADDER;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 0;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 6;
            } break;
            case TILE_LADDER_ONE_WAY: {
                g->tiles[k].collision              = TILE_LADDER_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 1;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 6;
            } break;
            case TILE_ONE_WAY: {
                g->tiles[k].collision              = TILE_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k].tx = 2;
                g->rtiles[TILELAYER_PROP_BG][k].ty = 6;
            } break;
            default:
                map_at_terrain(g, layer_terrain, x, y);
                break;
            }
        }
    }

    spm_pop();

    // BACKGROUND ==============================================================
    spm_push();
    tilelayer_bg_s layer_bg = {0};

    layer_bg.w     = w;
    layer_bg.h     = h;
    layer_bg.tiles = (u8 *)spm_alloc(header.bytes_tiles_bg);
    sys_file_read(mapf, layer_bg.tiles, header.bytes_tiles_bg);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            map_at_background(g, layer_bg, x, y);
        }
    }
    spm_pop();

    // PROPS_BG ================================================================
    spm_push();
    u16 *props_bg = (u16 *)spm_alloc(header.bytes_tiles_prop_bg);
    sys_file_read(mapf, props_bg, header.bytes_tiles_prop_bg);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int k  = x + y * w;
            int ID = props_bg[k];
            if (ID == 0) continue;
            int tx, ty, f;
            map_proptile_decode(ID, &tx, &ty, &f);
            rtile_s *rt = &g->rtiles[TILELAYER_PROP_BG][k];
            rt->tx      = tx;
            rt->ty      = ty;
        }
    }
    spm_pop();

    // PROPS_FG ================================================================
    spm_push();
    u16 *props_fg = (u16 *)spm_alloc(header.bytes_tiles_prop_fg);
    sys_file_read(mapf, props_fg, header.bytes_tiles_prop_fg);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int k  = x + y * w;
            int ID = props_fg[k];
            if (ID == 0) continue;
            int tx, ty, f;
            map_proptile_decode(ID, &tx, &ty, &f);
            rtile_s *rt = &g->rtiles[TILELAYER_PROP_FG][k];
            rt->tx      = tx;
            rt->ty      = ty;
        }
    }
    spm_pop();

    // OBJECTS =================================================================
    spm_push();
    char *op = (char *)spm_alloc(header.bytes_obj);
    sys_file_read(mapf, op, header.bytes_obj);

    for (int n = 0; n < header.n_obj; n++) {
        map_obj_s *o = (map_obj_s *)op;

        map_obj_parse(g, o);

        op += o->bytes;
    }
    spm_pop();

    spm_pop();
    sys_file_close(mapf);
}

void game_prepare_new_map(game_s *g)
{
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    for (obj_each(g, o)) {
        if (o->on_animate) { // just setting initial sprites for obj
            o->on_animate(g, o);
        }
    }
    aud_allow_playing_new_snd(1);
    cam_init_level(g, &g->cam);
}

static bool32 at_types_blending(int a, int b)
{
    if (a == b) return 1;
    if (b == AUTOTILE_TYPE_FAKE_1) return 0;
    if (a == AUTOTILE_TYPE_DIRT_DARK || b == AUTOTILE_TYPE_DIRT_DARK) return 0;
    return 1;
}

static int autotile_bg_is(tilelayer_bg_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;
    return (0 < tiles.tiles[u + v * tiles.w]);
}

static int autotile_terrain_is(tilelayer_terrain_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;

    map_terraintile_s a = tiles.tiles[x + y * tiles.w];
    map_terraintile_s b = tiles.tiles[u + v * tiles.w];
    if (!at_types_blending(a.type, b.type)) return 0;

    switch (b.shape) {
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
    case TILE_SLOPE_LO_0: return (sy == -1 && sx <= 0);
    case TILE_SLOPE_LO_2: return (sy == -1 && sx >= 0);
    case TILE_SLOPE_LO_1: return (sy == +1 && sx <= 0);
    case TILE_SLOPE_LO_3: return (sy == +1 && sx >= 0);
    case TILE_SLOPE_LO_4: return (sx == -1 && sy <= 0);
    case TILE_SLOPE_LO_6: return (sx == +1 && sy <= 0);
    case TILE_SLOPE_LO_5: return (sx == -1 && sy >= 0);
    case TILE_SLOPE_LO_7: return (sx == +1 && sy >= 0);
    case TILE_SLOPE_HI_0: return (sy <= 0);
    case TILE_SLOPE_HI_2: return (sy <= 0);
    case TILE_SLOPE_HI_1: return (sy >= 0);
    case TILE_SLOPE_HI_3: return (sy >= 0);
    case TILE_SLOPE_HI_4: return (sx <= 0);
    case TILE_SLOPE_HI_6: return (sx >= 0);
    case TILE_SLOPE_HI_5: return (sx <= 0);
    case TILE_SLOPE_HI_7: return (sx >= 0);
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
    AT_NW = B8(10000000),
};

static void map_at_background(game_s *g, tilelayer_bg_s tiles, int x, int y)
{
    int index = x + y * tiles.w;
    int tile  = tiles.tiles[index];
    if (tile == 0) return;

    flags32 march = 0;
    if (autotile_bg_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_bg_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_bg_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_bg_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_bg_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_bg_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_bg_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_bg_is(tiles, x, y, -1, -1)) march |= AT_NW;

    int      coords = g_autotilemarch[march];
    rtile_s *rtile  = &g->rtiles[TILELAYER_BG][index];
    rtile->tx       = 32 + (coords & 15);
    rtile->ty       = (tile * 8) + (coords >> 4);
}

static void map_at_terrain(game_s *g, tilelayer_terrain_s tiles, int x, int y)
{
    int               index = x + y * tiles.w;
    map_terraintile_s tile  = tiles.tiles[index];
    tile_s           *rtile = &g->tiles[index];
    if (tile.type == 0) return;
    if (tile.type == 1 || tile.type == 2) {
        rtile->collision = tile.shape;
        return;
    }
    rtile->type = tile.type;

    flags32 march = 0;
    if (autotile_terrain_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_terrain_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_terrain_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_terrain_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_terrain_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_terrain_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_terrain_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_terrain_is(tiles, x, y, -1, -1)) march |= AT_NW;

    int xcoord = 0;
    int ycoord = ((int)tile.type - 2) << 3;
    int coords = g_autotilemarch[march];

    switch (tile.shape) {
    case TILE_BLOCK: {

        int n_vari = 2; // number of variations in that tileset
        int vari   = rngr_i32(0, n_vari);
        switch (march) { // coordinates of variation tiles
        case 17: {       // vertical
            static const i32 altc_17[3] = {(7) | (3 << 4),
                                           (7) | (4 << 4),
                                           (7) | (6 << 4)};
            coords                      = altc_17[vari];
        } break;
        case 31: { // left border
            static const i32 altc_31[3] = {(0) | (4 << 4),
                                           (0) | (5 << 4),
                                           (5) | (3 << 4)};
            coords                      = altc_31[vari];
        } break;
        case 199: { // bot border
            static const i32 altc199[3] = {(4) | (2 << 4),
                                           (1) | (6 << 4),
                                           (3) | (6 << 4)};
            coords                      = altc199[vari];
        } break;
        case 241: { // right border
            static const i32 altc241[3] = {(6) | (1 << 4),
                                           (6) | (3 << 4),
                                           (2) | (4 << 4)};
            coords                      = altc241[vari];
        } break;
        case 68: { // horizontal
            static const i32 altc_68[3] = {(3) | (7 << 4),
                                           (4) | (7 << 4),
                                           (6) | (7 << 4)};
            coords                      = altc_68[vari];
        } break;
        case 124: { // top border
            static const i32 altc124[3] = {(4) | (0 << 4),
                                           (5) | (0 << 4),
                                           (3) | (5 << 4)};
            coords                      = altc124[vari];
        } break;
        case 255: { // mid
            static const i32 altc255[3] = {(4) | (1 << 4),
                                           (1) | (4 << 4),
                                           (5) | (1 << 4)};
            coords                      = altc255[vari];
        } break;
        }

        xcoord = coords & 15;
        ycoord += coords >> 4;
        rtile->collision = TILE_BLOCK;

        if (0 < y) {
            map_terraintile_s above = tiles.tiles[x + (y - 1) * tiles.w];
            if ((tile.type == AUTOTILE_TYPE_DIRT ||
                 tile.type == AUTOTILE_TYPE_BRICK) &&
                above.type == 0) {
                game_put_grass(g, x, y - 1);
            }
        }

    } break;
    case TILE_SLOPE_45_0:
    case TILE_SLOPE_45_1:
    case TILE_SLOPE_45_2:
    case TILE_SLOPE_45_3: {
        static const int nmasks[4 * 3] = {AT_E, AT_S, AT_SE, // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
                                          AT_E, AT_N, AT_NE,
                                          AT_W, AT_S, AT_SW,
                                          AT_W, AT_N, AT_NW};

        const int *nm = &nmasks[(tile.shape - TILE_SLOPE_45) * 3];
        int        xn = (march & nm[0]) != 0;                      // x neighbour
        int        yn = (march & nm[1]) != 0;                      // y neighbour
        int        cn = (march & nm[2]) != 0;                      // diagonal neighbour
        xcoord        = 8 + (xn && yn && cn ? 4 : xn | (yn << 1)); // slope image index
        ycoord += (tile.shape - TILE_SLOPE_45);

        rtile->collision = tile.shape;
    } break;
    case TILE_SLOPE_LO_0:
    case TILE_SLOPE_LO_1:
    case TILE_SLOPE_LO_2:
    case TILE_SLOPE_LO_3:
    case TILE_SLOPE_LO_4:
    case TILE_SLOPE_LO_5:
    case TILE_SLOPE_LO_6:
    case TILE_SLOPE_LO_7: {
        xcoord = 17; // slope image index
        ycoord += (tile.shape - TILE_SLOPE_LO);
        rtile->collision = tile.shape;
    } break;
    case TILE_SLOPE_HI_0:
    case TILE_SLOPE_HI_1:
    case TILE_SLOPE_HI_2:
    case TILE_SLOPE_HI_3:
    case TILE_SLOPE_HI_4:
    case TILE_SLOPE_HI_5:
    case TILE_SLOPE_HI_6:
    case TILE_SLOPE_HI_7: {
        xcoord = 23; // slope image index
        ycoord += (tile.shape - TILE_SLOPE_HI);
        rtile->collision = tile.shape;
    } break;
    }

    rtile->tx = xcoord;
    rtile->ty = ycoord;
}

static map_prop_s *map_prop_get(map_properties_s p, const char *name)
{
    if (p.p == NULL) return NULL;
    char *ptr = (char *)p.p;
    for (int n = 0; n < p.n; n++) {
        map_prop_s *prop = (map_prop_s *)ptr;
        if (str_eq_nc(prop->name, name)) {
            return prop;
        }
        ptr += prop->bytes;
    }
    sys_printf("No property: %s\n", name);
    return NULL;
}

static void map_prop_str(map_properties_s p, const char *name, void *b, usize bs)
{
    if (!b || bs == 0) return;
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_STRING) return;
    char *s       = (char *)(prop + 1);
    char *d       = (char *)b;
    usize written = 0;
    while (1) {
        if (bs <= written) break;
        *d = *s;
        if (*s == '\0') break;
        s++;
        d++;
        written++;
    }
    ((char *)b)[bs - 1] = '\0';
}

static i32 map_prop_i32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_INT) return 0;
    return prop->u.i;
}

static f32 map_prop_f32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_FLOAT) return 0.f;
    return prop->u.f;
}

static bool32 map_prop_bool(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_BOOL) return 0;
    return prop->u.b;
}

static v2_i16 map_prop_pt(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_POINT) return (v2_i16){0};
    return prop->u.p;
}

static map_properties_s map_obj_properties(map_obj_s *mo)
{
    map_properties_s p = {0};
    p.p                = (void *)(mo + 1);
    p.n                = mo->n_prop;
    return p;
}

void map_obj_str(map_obj_s *mo, const char *name, void *b, usize bs)
{
    map_prop_str(map_obj_properties(mo), name, b, bs);
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

void *map_obj_arr(map_obj_s *mo, const char *name, int *num)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (prop == NULL || prop->type != MAP_PROP_ARRAY) return NULL;
    *num = prop->u.n;
    return (prop + 1);
}

void map_world_load(map_world_s *world, const char *worldfile)
{
    if (!world || !worldfile || worldfile[0] == '\0') return;

    FILEPATH_GEN(filepath, FILEPATH_MAP, worldfile);
    void *f = sys_file_open(filepath, SYS_FILE_R);
    sys_file_read(f, world, sizeof(map_world_s));
    sys_file_close(f);
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
    int i = (objID >> 8) - 1;
    sys_printf("get room %i\n", i);
    map_worldroom_s *r = &world->rooms[i];
    return r;
}

// exported from autotiles_marching.xlsx
// maps marchID -> x + y * 16
static const u8 g_autotilemarch[256] = {
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x27,
    0x57,
    0x27,
    0x57,
    0x02,
    0x12,
    0x02,
    0x65,
    0x27,
    0x57,
    0x27,
    0x57,
    0x03,
    0x21,
    0x03,
    0x13,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x34,
    0x54,
    0x34,
    0x54,
    0x06,
    0x56,
    0x06,
    0x23,
    0x34,
    0x54,
    0x34,
    0x54,
    0x04,
    0x22,
    0x04,
    0x31,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x27,
    0x46,
    0x27,
    0x46,
    0x02,
    0x44,
    0x02,
    0x62,
    0x27,
    0x46,
    0x27,
    0x46,
    0x03,
    0x33,
    0x03,
    0x25,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x34,
    0x16,
    0x34,
    0x16,
    0x06,
    0x26,
    0x06,
    0x32,
    0x34,
    0x16,
    0x34,
    0x16,
    0x04,
    0x52,
    0x04,
    0x14};