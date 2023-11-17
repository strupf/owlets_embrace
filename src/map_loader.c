// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "util/str.h"

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html

typedef struct {
    u16 pxx; // position on map in pixels
    u16 pxy;
    u16 srcx; // position in tileset in pixels
    u16 srcy;
    u16 f; // f=1 (X flip only), f=2 (Y flip only), f=3 (both flips)
} ldtk_tile_s;

typedef struct {
    u8 type;
    u8 shape;
} autotile_s;

typedef struct {
    autotile_s *tiles;
    int         w;
    int         h;
} autotiling_s;

enum {
    AUTOTILE_TYPE_NONE,
    AUTOTILE_TYPE_BRICK,
};

static const u8    g_autotilemarch[256];
static void        autotile(game_s *g, autotiling_s tiling);
static ldtk_tile_s ldtk_tile_from_json(json_s jtile);
static obj_s      *ldtk_load_obj_from_json(game_s *g, json_s jobj);

void map_world_load(map_world_s *world, const char *filename)
{
    if (!world) return;

    spm_push();
    char *txt;
    txt_load(filename, spm_alloc, &txt);
    assert(txt);
    json_s jroot;
    json_root(txt, &jroot);

    for (json_each(jroot, "levels", jlevel)) {
        map_room_s *r = &world->rooms[world->n_rooms++];
        jsonk_str(jlevel, "identifier", r->filename, sizeof(r->filename));
        r->GUID = GUID_parse_str(jsonk_strp(jlevel, "iid", NULL));
        sys_printf("%s\n", r->filename);
        r->r.x = jsonk_i32(jlevel, "worldX");
        r->r.y = jsonk_i32(jlevel, "worldY");
        r->r.w = jsonk_i32(jlevel, "pxWid");
        r->r.h = jsonk_i32(jlevel, "pxHei");
    }

    spm_pop();
}

map_room_s *map_world_overlapped_room(map_world_s *world, rec_i32 r)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_room_s *room = &world->rooms[i];
        if (room == world->roomcur) continue;
        if (overlap_rec(room->r, r))
            return room;
    }
    return NULL;
}

map_room_s *map_world_find_room(map_world_s *world, const char *filename)
{

    int         len = str_len(filename);
    const char *c   = &filename[len];
    for (int i = len; i >= 0; i--) {
        if (*c == '/') {
            c++;
            break;
        }
        c--;
    }

    char  fname[64] = {0};
    char *fn        = fname;
    do {
        *fn++ = *c++;
    } while (*c != '.');

    for (int i = 0; i < world->n_rooms; i++) {
        map_room_s *room = &world->rooms[i];
        if (str_eq(room->filename, fname))
            return room;
    }
    return NULL;
}

void game_load_map(game_s *g, const char *filename)
{
    g->obj_ndelete = 0;
    g->obj_nbusy   = 0;
    g->obj_nfree   = NUM_OBJ;
    for (int n = 0; n < NUM_OBJ; n++) {
        obj_generic_s *o = &g->obj_raw[n];
        o->o.index       = n;
        o->o.gen         = 1;
#ifdef SYS_DEBUG
        o->magic = OBJ_GENERIC_MAGIC;
#endif
        g->obj_free_stack[n] = (obj_s *)o;
    }

    for (int n = 0; n < NUM_OBJ_TAGS; n++) {
        g->obj_tag[n] = NULL;
    }

    spm_push();
    char *txt;
    txt_load(filename, spm_alloc, &txt);
    assert(txt);
    json_s jroot;
    json_root(txt, &jroot);

    g->map_world.roomcur = map_world_find_room(&g->map_world, filename);

    g->pixel_x    = jsonk_u32(jroot, "pxWid");
    g->pixel_y    = jsonk_u32(jroot, "pxHei");
    g->tiles_x    = g->pixel_x >> 4;
    g->tiles_y    = g->pixel_y >> 4;
    int num_tiles = g->tiles_x * g->tiles_y;
    assert(num_tiles <= NUM_TILES);

    memset(g->tiles, 0, sizeof(g->tiles));
    memset(g->rtiles, 0, sizeof(g->rtiles));

    for (json_each(jroot, "layerInstances", jlayer)) {
        char name[64];
        jsonk_strs(jlayer, "__identifier", name);

        char *layertype = jsonk_strp(jlayer, "__type", NULL);
        spm_push();

        switch (*layertype) {
        case 'I': { // "IntGrid"
            int i = 0;
            for (json_each(jlayer, "intGridCsv", jtile)) {
                int t = json_u32(jtile);
                i++;
            }
            assert(i == num_tiles);
        } break;
        case 'T': { // "Tiles"

            if (0) {
            } else if (str_eq(name, "AUTOTILES")) {

                autotiling_s tiling;
                tiling.w     = g->tiles_x;
                tiling.h     = g->tiles_y;
                tiling.tiles = (autotile_s *)spm_alloc(sizeof(autotile_s) * num_tiles);
                for (json_each(jlayer, "gridTiles", jtile)) {
                    ldtk_tile_s tile = ldtk_tile_from_json(jtile);
                    int         tx   = tile.pxx >> 4;
                    int         ty   = tile.pxy >> 4;
                    int         sx   = tile.srcx >> 4;
                    int         sy   = tile.srcy >> 4;

                    int atype  = sy;
                    int ashape = 0;
                    switch (sx) {
                    case 0: // block
                        ashape = TILE_BLOCK;
                        break;
                    case 1: // slope 45
                        switch (tile.f) {
                        case 0: ashape = TILE_SLOPE_45_0; break;
                        case 1: ashape = TILE_SLOPE_45_2; break; // flipx
                        case 2: ashape = TILE_SLOPE_45_1; break; // flipy
                        case 3: ashape = TILE_SLOPE_45_3; break; // flipxy
                        }
                        break;
                    case 2: // slope lo
                        switch (tile.f) {
                        case 0: ashape = TILE_SLOPE_LO_0; break;
                        case 1: ashape = TILE_SLOPE_LO_1; break;
                        case 2: ashape = TILE_SLOPE_LO_2; break;
                        case 3: ashape = TILE_SLOPE_LO_3; break;
                        }
                        break;
                    case 4: // slope lo
                        switch (tile.f) {
                        case 0: ashape = TILE_SLOPE_LO_4; break;
                        case 1: ashape = TILE_SLOPE_LO_5; break;
                        case 2: ashape = TILE_SLOPE_LO_6; break;
                        case 3: ashape = TILE_SLOPE_LO_7; break;
                        }
                        break;
                    case 3: // slope hi
                        switch (tile.f) {
                        case 0: ashape = TILE_SLOPE_HI_0; break;
                        case 1: ashape = TILE_SLOPE_HI_1; break;
                        case 2: ashape = TILE_SLOPE_HI_2; break;
                        case 3: ashape = TILE_SLOPE_HI_3; break;
                        }
                        break;
                    case 5: // slope hi
                        switch (tile.f) {
                        case 0: ashape = TILE_SLOPE_HI_4; break;
                        case 1: ashape = TILE_SLOPE_HI_5; break;
                        case 2: ashape = TILE_SLOPE_HI_6; break;
                        case 3: ashape = TILE_SLOPE_HI_7; break;
                        }
                        break;
                    }

                    autotile_s at                      = {atype, ashape};
                    tiling.tiles[tx + ty * g->tiles_x] = at;
                }
                autotile(g, tiling);

            } else if (str_eq(name, "AUTO")) {
                u32 *tiles = (u32 *)spm_alloc(sizeof(u32) * num_tiles);
            }
        } break;
        case 'E': { // "Entities"
#if 1
            for (json_each(jlayer, "entityInstances", jobj)) {
                ldtk_load_obj_from_json(g, jobj);
            }
#endif
        } break;
        }

        spm_pop();
    }

    spm_pop();
}

// {
// "px":  [16,32],
// "src": [16,16],
// "f":   0,
// "t":   257,
// "d":   [33],
// "a":   1
// }
static ldtk_tile_s ldtk_tile_from_json(json_s jtile)
{
    ldtk_tile_s tile = {0};
    json_s      j;
    json_key(jtile, "px", &j);
    json_fchild(j, &j);
    tile.pxx = json_u32(j);
    json_next(j, &j);
    tile.pxy = json_u32(j);
    json_key(jtile, "src", &j);
    json_fchild(j, &j);
    tile.srcx = json_u32(j);
    json_next(j, &j);
    tile.srcy = json_u32(j);
    tile.f    = jsonk_u32(jtile, "f");
    return tile;
}

static obj_s *ldtk_load_obj_from_json(game_s *g, json_s jobj)
{
    obj_s *o = NULL;

    char oname[64] = {0};
    jsonk_strs(jobj, "__identifier", oname);

    json_s jpx;
    json_key(jobj, "px", &jpx);
    json_fchild(jpx, &jpx);
    int x = json_i32(jpx);
    json_next(jpx, &jpx);
    int y = json_i32(jpx);
    int w = jsonk_u32(jobj, "width");
    int h = jsonk_u32(jobj, "height");

    // type
    if (0) {
    } else if (str_eq(oname, "Solid")) {
        o        = obj_solid_create(g);
        o->pos.x = x;
        o->pos.y = y;
        o->w     = w;
        o->h     = h;
    } else if (str_eq(oname, "Sign")) {
        o     = obj_create(g);
        o->ID = OBJ_ID_SIGN;
        o->w  = 16;
        o->h  = 16;
        o->flags |= OBJ_FLAG_INTERACTABLE;
    }

    if (!o) return NULL;

    o->GUID  = GUID_parse_str(jsonk_strp(jobj, "iid", NULL));
    o->pos.x = x;
    o->pos.y = y;

    // properties
    for (json_each(jobj, "fieldInstances", jfield)) {
        char fieldname[64] = {0};
        jsonk_strs(jfield, "__identifier", fieldname);
        sys_printf("id: %s\n", fieldname);

        if (0) {
        } else if (str_eq(fieldname, "filename")) {
            jsonk_strs(jfield, "__value", o->filename);
            sys_printf("value: %s\n", o->filename);
        }
    }
    return o;
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

// returns true if tile looked at from x/y in direction sx/sy is "solid"
// x/y: tile currently looked at
// sx/sy direction we are looking in
// ai: current tile ID
// af: current tile flipping
static int autotile_is(autotiling_s t, int x, int y, int sx, int sy, autotile_s a)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || t.w <= u || v < 0 || t.h <= v) return 1;

    autotile_s b = t.tiles[u + v * t.w];
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

static flags32 autotile_march(autotiling_s t, int x, int y, autotile_s a)
{
    flags32 m = 0;
    if (autotile_is(t, x, y, +0, -1, a)) m |= AT_N;
    if (autotile_is(t, x, y, +1, +0, a)) m |= AT_E;
    if (autotile_is(t, x, y, +0, +1, a)) m |= AT_S;
    if (autotile_is(t, x, y, -1, +0, a)) m |= AT_W;
    if (autotile_is(t, x, y, +1, -1, a)) m |= AT_NE;
    if (autotile_is(t, x, y, +1, +1, a)) m |= AT_SE;
    if (autotile_is(t, x, y, -1, +1, a)) m |= AT_SW;
    if (autotile_is(t, x, y, -1, -1, a)) m |= AT_NW;
    return m;
}

static void autotile(game_s *g, autotiling_s tiling)
{
    for (int y = 0; y < tiling.h; y++) {
        for (int x = 0; x < tiling.w; x++) {
            int        n = x + y * tiling.w;
            autotile_s t = tiling.tiles[n];
            if (t.type == AUTOTILE_TYPE_NONE) {
                g->tiles[n].collision = TILE_EMPTY;
                continue;
            }

            flags32 m      = autotile_march(tiling, x, y, t);
            int     xcoord = 0;
            int     ycoord = 0;

            switch (t.shape) {
            case TILE_BLOCK: {
                int coords            = g_autotilemarch[m];
                xcoord                = coords & 15;
                ycoord                = coords >> 4;
                g->tiles[n].collision = TILE_BLOCK;
            } break;
            case TILE_SLOPE_45_0:
            case TILE_SLOPE_45_1:
            case TILE_SLOPE_45_2:
            case TILE_SLOPE_45_3: {
                static const int nmasks[4 * 3] = {AT_E, AT_S, AT_SE, // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
                                                  AT_E, AT_N, AT_NE,
                                                  AT_W, AT_S, AT_SW,
                                                  AT_W, AT_N, AT_NW};

                int *nm = &nmasks[(t.shape - TILE_SLOPE_45) * 3];
                int  xn = (m & nm[0]) != 0;                          // x neighbour
                int  yn = (m & nm[1]) != 0;                          // y neighbour
                int  cn = (m & nm[2]) != 0;                          // diagonal neighbour
                xcoord  = 8 + (xn && yn && cn ? 4 : xn | (yn << 1)); // slope image index
                ycoord  = (t.type - 1) * 8 + (t.shape - TILE_SLOPE_45);

                g->tiles[n].collision = t.shape;
            } break;
            }

            g->rtiles[n].layer[0] = rtile_pack(xcoord, ycoord);
        }
    }
}

// exported from autotiles_marching.xlsx
// maps marchID -> x + y * 16
static const u8 g_autotilemarch[256] = {
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x10, 0x37, 0x10, 0x37, 0x11, 0x20, 0x11, 0x60,
    0x10, 0x37, 0x10, 0x37, 0x55, 0x30, 0x55, 0x35,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x10, 0x37, 0x10, 0x37, 0x11, 0x20, 0x11, 0x60,
    0x10, 0x37, 0x10, 0x37, 0x55, 0x30, 0x55, 0x35,
    0x07, 0x77, 0x07, 0x77, 0x73, 0x75, 0x73, 0x45,
    0x07, 0x77, 0x07, 0x77, 0x73, 0x75, 0x73, 0x45,
    0x27, 0x57, 0x27, 0x57, 0x02, 0x12, 0x02, 0x65,
    0x27, 0x57, 0x27, 0x57, 0x03, 0x21, 0x03, 0x13,
    0x07, 0x77, 0x07, 0x77, 0x73, 0x75, 0x73, 0x45,
    0x07, 0x77, 0x07, 0x77, 0x73, 0x75, 0x73, 0x45,
    0x34, 0x54, 0x34, 0x54, 0x06, 0x56, 0x06, 0x23,
    0x34, 0x54, 0x34, 0x54, 0x04, 0x22, 0x04, 0x31,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x10, 0x37, 0x10, 0x37, 0x11, 0x20, 0x11, 0x60,
    0x10, 0x37, 0x10, 0x37, 0x55, 0x30, 0x55, 0x35,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x00, 0x70, 0x00, 0x70, 0x01, 0x72, 0x01, 0x43,
    0x10, 0x37, 0x10, 0x37, 0x11, 0x20, 0x11, 0x60,
    0x10, 0x37, 0x10, 0x37, 0x55, 0x30, 0x55, 0x35,
    0x07, 0x66, 0x07, 0x66, 0x73, 0x64, 0x73, 0x24,
    0x07, 0x66, 0x07, 0x66, 0x73, 0x64, 0x73, 0x24,
    0x27, 0x46, 0x27, 0x46, 0x02, 0x44, 0x02, 0x62,
    0x27, 0x46, 0x27, 0x46, 0x03, 0x33, 0x03, 0x25,
    0x07, 0x66, 0x07, 0x66, 0x73, 0x64, 0x73, 0x24,
    0x07, 0x66, 0x07, 0x66, 0x73, 0x64, 0x73, 0x24,
    0x34, 0x16, 0x34, 0x16, 0x06, 0x26, 0x06, 0x32,
    0x34, 0x16, 0x34, 0x16, 0x04, 0x52, 0x04, 0x14};