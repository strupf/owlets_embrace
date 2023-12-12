// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "util/str.h"

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html

typedef struct {
    int w;
    int h;
    int n_obj;
    int n_props_bg;
    int n_props_fg;
    int n_properties;
} map_meta_s;

typedef struct {
    char name[64];
    char type;
    union {
        char s[64];
        i32  i;
        f32  f;
        int  o;
        int  b;
    } v;
} map_property_s;

typedef struct {
    u8 type;
    u8 shape;
} map_terraintile_s;

typedef struct {
    char name[64];
    char image[64];
    int  id;
    int  x;
    int  y;
    int  w;
    int  h;
    int  n_polygon;
    int  n_polyline;
    int  n_properties;
} map_obj_s;

typedef struct {
    char name[64];
    char image[64];
    int  id;
    int  x;
    int  y;
    int  w;
    int  h;
    int  n_polygon;
    int  n_polyline;
    int  n_properties;

    v2_i32         *poly;
    map_property_s *properties;
} map_object_s;

typedef struct {
    u8 *tiles;
    int w;
    int h;
} tilelayer_bg_s;

typedef struct {
    map_terraintile_s *tiles;
    int                w;
    int                h;
} tilelayer_terrain_s;

enum {
    AUTOTILE_TYPE_NONE,
    AUTOTILE_TYPE_BRICK,
    AUTOTILE_TYPE_BRICK_SMALL,
    AUTOTILE_TYPE_DIRT,
    AUTOTILE_TYPE_STONE,
};

enum {
    TILE_FLIP_DIA = 1 << 0,
    TILE_FLIP_Y   = 1 << 1,
    TILE_FLIP_X   = 1 << 2,
};

static const u8 g_autotilemarch[256];

static map_object_s map_read_object(void *f, void *(*allocf)(usize s));
static void         map_autotile_background(game_s *g, tilelayer_bg_s tiles, int x, int y);
static void         map_autotile_terrain(game_s *g, tilelayer_terrain_s tiles, int x, int y);
static bool32       map_property_get(map_property_s *props, int n_props, const char *name, map_property_s *prop);

void map_world_load(map_world_s *world, const char *worldfile)
{
    if (!world || !worldfile || worldfile[0] == '\0') return;

    FILEPATH_GEN(filepath, FILEPATH_MAP, worldfile);
    void *f = sys_file_open(filepath, SYS_FILE_R);
    sys_file_read(f, world, sizeof(map_world_s));
    sys_file_close(f);
}

map_worldroom_s *map_world_overlapped_room(map_world_s *world, rec_i32 r)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (room == world->roomcur) continue;
        rec_i32 rr = {room->x, room->y, room->w, room->h};
        if (overlap_rec(rr, r))
            return room;
    }
    return NULL;
}

map_worldroom_s *map_world_find_room(map_world_s *world, const char *mapfile)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (str_eq(room->filename, mapfile))
            return room;
    }
    return NULL;
}

void game_load_map(game_s *g, const char *mapfile)
{
    g->obj_ndelete = 0;
    g->obj_nbusy   = 0;
    g->obj_nfree   = NUM_OBJ;
    for (int n = 0; n < NUM_OBJ; n++) {
        obj_s *o             = &g->obj_raw[n];
        o->UID.index         = n;
        o->UID.gen           = 1;
        g->obj_free_stack[n] = o;
    }

    for (int n = 0; n < NUM_OBJ_TAGS; n++) {
        g->obj_tag[n] = NULL;
    }
    g->n_grass    = 0;
    g->n_ropes    = 0;
    g->n_decal_fg = 0;
    g->n_decal_bg = 0;
    marena_init(&g->arena, g->mem, sizeof(g->mem));

    g->ocean.ocean.particles      = (waterparticle_s *)marena_alloc(&g->arena, sizeof(waterparticle_s) * 1024);
    g->ocean.ocean.nparticles     = 1024;
    g->ocean.ocean.dampening_q12  = 4070;
    g->ocean.ocean.fneighbour_q16 = 8000;
    g->ocean.ocean.fzero_q16      = 20;
    g->ocean.ocean.loops          = 1;
    g->ocean.y                    = 300;

    g->ocean.water.particles      = (waterparticle_s *)marena_alloc(&g->arena, sizeof(waterparticle_s) * 4096);
    g->ocean.water.nparticles     = 4096;
    g->ocean.water.dampening_q12  = 4060;
    g->ocean.water.fneighbour_q16 = 2000;
    g->ocean.water.fzero_q16      = 100;
    g->ocean.water.loops          = 3;
    g->ocean.water.p              = (v2_i32){0};

    strcpy(g->areaname.filename, mapfile);
    g->map_world.roomcur = map_world_find_room(&g->map_world, mapfile);
    memset(g->tiles, 0, sizeof(g->tiles));
    memset(g->rtiles, 0, sizeof(g->rtiles));

    spm_push();

    FILEPATH_GEN(filepath, FILEPATH_MAP, mapfile);
    str_append(filepath, ".map");

    spm_push();
    void *mapf = sys_file_open(filepath, SYS_FILE_R);

    map_meta_s meta;
    sys_file_read(mapf, &meta, sizeof(meta));

    int w = meta.w;
    int h = meta.h;
    fade_start(&g->areaname.fade, 30, 300, 30, NULL, NULL, NULL);
    g->tiles_x = w;
    g->tiles_y = h;
    g->pixel_x = w << 4;
    g->pixel_y = h << 4;

    // PROPERTIES ==============================================================================
    spm_push();
    usize           propsize = (usize)(sizeof(map_property_s) * meta.n_properties);
    map_property_s *props    = (map_property_s *)spm_alloc(propsize);
    sys_file_read(mapf, props, propsize);
    // do stuff

    map_property_s prop;
    if (map_property_get(props, meta.n_properties, "music", &prop)) {
        mus_fade_to(mus_load(prop.v.s), 60, 60);
    }
    if (map_property_get(props, meta.n_properties, "name", &prop)) {
        mus_fade_to(mus_load(prop.v.s), 60, 60);
        str_cpy(g->areaname.label, prop.v.s);
    }

    spm_pop();

    // TERRAIN =================================================================================
    spm_push();
    tilelayer_terrain_s layer_terrain = {0};
    usize               terrainsize   = (usize)(sizeof(map_terraintile_s) * meta.w * meta.h);
    layer_terrain.tiles               = (map_terraintile_s *)spm_alloc(terrainsize);
    layer_terrain.w                   = w;
    layer_terrain.h                   = h;
    sys_file_read(mapf, layer_terrain.tiles, terrainsize);
    // do stuff

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            map_autotile_terrain(g, layer_terrain, x, y);
        }
    }

    spm_pop();

    // BACKGROUND ==============================================================================

    spm_push();
    tilelayer_bg_s layer_bg = {0};
    usize          bgsize   = (usize)(sizeof(u8) * meta.w * meta.h);
    layer_bg.tiles          = (u8 *)spm_alloc(bgsize);
    layer_bg.w              = w;
    layer_bg.h              = h;
    sys_file_read(mapf, layer_bg.tiles, bgsize);
    // do stuff

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            map_autotile_background(g, layer_bg, x, y);
        }
    }

    spm_pop();

    // OBJECTS =================================================================================
    for (int n = 0; n < meta.n_obj; n++) {
        spm_push();
        map_object_s obj = map_read_object(mapf, spm_alloc);

        spm_pop();
    }

    // PROPS BG ====================================================================================
    for (int n = 0; n < meta.n_props_bg; n++) {
        spm_push();
        map_object_s obj = map_read_object(mapf, spm_alloc);
        spm_pop();
    }

    // PROPS FG ====================================================================================
    for (int n = 0; n < meta.n_props_fg; n++) {
        spm_push();
        map_object_s obj = map_read_object(mapf, spm_alloc);

        if (str_len(obj.image) > 0) {
            tex_s tex;
            if (0 <= asset_tex_load(obj.image, &tex)) {
                decal_s *decal = &g->decal_fg[g->n_decal_fg++];
                decal->tex     = tex;
                decal->x       = obj.x;
                decal->y       = obj.y - tex.h;
            }
        }

        spm_pop();
    }

    sys_file_close(mapf);
    spm_pop();
}

static int autotile_bg_is(tilelayer_bg_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;
    return (tiles.tiles[u + v * tiles.w] > 0);
}

static int autotile_terrain_is(tilelayer_terrain_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;

    map_terraintile_s b = tiles.tiles[u + v * tiles.w];
    if (b.type == AUTOTILE_TYPE_NONE) return 0;

    switch (b.shape) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return (sy == -1 || sx == -1);
    case TILE_SLOPE_45_1: return (sy == +1 || sx == -1);
    case TILE_SLOPE_45_2: return (sy == -1 || sx == +1);
    case TILE_SLOPE_45_3: return (sy == +1 || sx == +1);
    case TILE_SLOPE_LO_0: return (sy == -1 && sx <= 0);
    case TILE_SLOPE_LO_1: return (sy == -1 && sx >= 0);
    case TILE_SLOPE_LO_2: return (sy == +1 && sx <= 0);
    case TILE_SLOPE_LO_3: return (sy == +1 && sx >= 0);
    case TILE_SLOPE_LO_4: return (sx == -1 && sy <= 0);
    case TILE_SLOPE_LO_5: return (sx == +1 && sy <= 0);
    case TILE_SLOPE_LO_6: return (sx == -1 && sy >= 0);
    case TILE_SLOPE_LO_7: return (sx == +1 && sy >= 0);
    case TILE_SLOPE_HI_0: return (sy <= 0 && (sx <= 0));
    case TILE_SLOPE_HI_1: return (sy <= 0 && (sx >= 0));
    case TILE_SLOPE_HI_2: return (sy >= 0 && (sx <= 0));
    case TILE_SLOPE_HI_3: return (sy >= 0 && (sx >= 0));
    case TILE_SLOPE_HI_4: return (sx <= 0 && (sy <= 0));
    case TILE_SLOPE_HI_5: return (sx >= 0 && (sy <= 0));
    case TILE_SLOPE_HI_6: return (sx <= 0 && (sy >= 0));
    case TILE_SLOPE_HI_7: return (sx >= 0 && (sy >= 0));
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

static void map_autotile_background(game_s *g, tilelayer_bg_s tiles, int x, int y)
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

    int xcoord = 0;
    int ycoord = (tile - 1) * 8;
    int coords = g_autotilemarch[march];

    xcoord = coords & 15;
    ycoord += coords >> 4;
    g->rtiles[index].layer[0] = rtile_pack(xcoord, ycoord);
}

static void map_autotile_terrain(game_s *g, tilelayer_terrain_s tiles, int x, int y)
{
    int               index = x + y * tiles.w;
    map_terraintile_s tile  = tiles.tiles[index];
    if (tile.type == 0) return;

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
    int ycoord = (tile.type - 1) * 8;
    int coords = g_autotilemarch[march];

    switch (tile.shape) {
    case TILE_BLOCK: {
        switch (march) {
        case 17: { // vertical
        } break;
        case 31: { // left border

        } break;
        case 199: { // bot border

        } break;
        case 241: { // right border

        } break;
        case 68: { // horizontal

        } break;
        case 124: { // top border

        } break;
        case 255: { // mid

        } break;
        }

        xcoord = coords & 15;
        ycoord += coords >> 4;
        g->tiles[index].collision = TILE_BLOCK;
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

        g->tiles[index].collision = tile.shape;
    } break;
    }
    g->rtiles[index].layer[1] = rtile_pack(xcoord, ycoord);
}

static bool32 map_property_get(map_property_s *props, int n_props, const char *name, map_property_s *prop)
{
    for (int n = 0; n < n_props; n++) {
        map_property_s p = props[n];
        if (str_eq(p.name, name)) {
            *prop = p;
            return 1;
        }
    }
    return 0;
}

static map_object_s map_read_object(void *f, void *(*allocf)(usize s))
{
    map_object_s obj = {0};
    sys_file_read(f, &obj, sizeof(map_obj_s));

    int n_poly = max_i(obj.n_polygon, obj.n_polyline);
    if (n_poly > 0) {
        usize polysize = sizeof(v2_i32) * n_poly;
        obj.poly       = (v2_i32 *)allocf(polysize);
        sys_file_read(f, obj.poly, polysize);
    }
    if (obj.n_properties > 0) {
        usize propsize = sizeof(map_property_s) * obj.n_properties;
        obj.properties = (map_property_s *)allocf(propsize);
        sys_file_read(f, obj.properties, propsize);
    }
    return obj;
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