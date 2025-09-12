// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_TYPES_H
#define TILE_TYPES_H

#include "gamedef.h"

#define TILE_TYPE_ID_BY_XY(X, Y) ((X) + (Y) * 4)

enum {
    TILE_TYPE_NONE                     = TILE_TYPE_ID_BY_XY(0, 0),
    TILE_TYPE_INVISIBLE_NON_CONNECTING = TILE_TYPE_ID_BY_XY(1, 0),
    TILE_TYPE_INVISIBLE_CONNECTING     = TILE_TYPE_ID_BY_XY(2, 0),
    TILE_TYPE_UNUSED                   = TILE_TYPE_ID_BY_XY(3, 0),
    //
    TILE_TYPE_DARK_LEAVES              = TILE_TYPE_ID_BY_XY(0, 1),
    TILE_TYPE_DARK_STONE               = TILE_TYPE_ID_BY_XY(1, 1),
    TILE_TYPE_DARK_STONE_PEBBLE        = TILE_TYPE_ID_BY_XY(2, 1),
    TILE_TYPE_DARK_OBSIDIAN            = TILE_TYPE_ID_BY_XY(0, 4),
    TILE_TYPE_BRIGHT_STONE             = TILE_TYPE_ID_BY_XY(0, 2),
    TILE_TYPE_BRIGHT_SNOW              = TILE_TYPE_ID_BY_XY(1, 2),
    TILE_TYPE_BRIGHT_MOUNTAIN          = TILE_TYPE_ID_BY_XY(2, 2),
    TILE_TYPE_THORNS                   = TILE_TYPE_ID_BY_XY(0, 5),
    TILE_TYPE_SPIKES                   = TILE_TYPE_ID_BY_XY(2, 5),
    TILE_TYPE_BRIGHT_STOMP             = TILE_TYPE_ID_BY_XY(0, 7),
    TILE_TYPE_BRIGHT_STOMP_2           = TILE_TYPE_ID_BY_XY(1, 7),
    TILE_TYPE_BRIGHT_STOMP_HOR         = TILE_TYPE_ID_BY_XY(3, 7),
    TILE_TYPE_MUSHROOMS                = TILE_TYPE_ID_BY_XY(0, 8),
    TILE_TYPE_MUSHROOMS_HALF           = TILE_TYPE_ID_BY_XY(1, 8),
    TILE_TYPE_MUSHROOMS_GONE           = TILE_TYPE_ID_BY_XY(2, 8),
    TILE_TYPE_BRIGHT_BREAKING          = TILE_TYPE_ID_BY_XY(3, 8),
    TILE_TYPE_CRUMBLE                  = TILE_TYPE_ID_BY_XY(3, 8),
    //
    NUM_TILE_TYPES                     = 24
};

enum {
    TILE_TYPE_COLOR_BRIGHT,
    TILE_TYPE_COLOR_DARK,
    TILE_TYPE_COLOR_BLACK
};

typedef struct tile_s {
    ALIGNAS(4)
    u16 ty;
    u8  type;  // 6 bits for type
    u8  shape; // collision shape
} tile_s;

#define TILE_TYPE_FLAG_INNER_GRADIENT (1 << 7)
#define TILE_TYPE_MASK                B8(00111111)

static i32 tile_type_color(i32 t)
{
    switch (t) {
    case TILE_TYPE_DARK_STONE:
    case TILE_TYPE_DARK_STONE_PEBBLE:
    case TILE_TYPE_DARK_OBSIDIAN:
    case TILE_TYPE_DARK_LEAVES:
        return TILE_TYPE_COLOR_BLACK;
        //
    case TILE_TYPE_BRIGHT_STONE:
    case TILE_TYPE_BRIGHT_BREAKING:
    case TILE_TYPE_BRIGHT_STOMP:
    case TILE_TYPE_BRIGHT_STOMP_2:
    case TILE_TYPE_BRIGHT_STOMP_HOR:
        return TILE_TYPE_COLOR_DARK;
        //
    case TILE_TYPE_BRIGHT_SNOW:
        return TILE_TYPE_COLOR_BRIGHT;
    }

    return 0;
}

// higher = gets rendered on top
static i32 tile_type_render_priority(i32 t)
{
    switch (t) {
    case TILE_TYPE_DARK_OBSIDIAN: return 1;
    case TILE_TYPE_DARK_STONE: return 4;
    case TILE_TYPE_DARK_STONE_PEBBLE: return 5;
    case TILE_TYPE_DARK_LEAVES: return 6;
    case TILE_TYPE_BRIGHT_STONE: return 7;
    case TILE_TYPE_BRIGHT_BREAKING: return 8;
    case TILE_TYPE_BRIGHT_STOMP: return 9;
    case TILE_TYPE_BRIGHT_STOMP_2: return 10;
    case TILE_TYPE_BRIGHT_STOMP_HOR: return 11;
    case TILE_TYPE_BRIGHT_SNOW: return 2;
    }

    return t;
}

static void tile_set(tile_s *t, i32 type, i32 shape)
{
    t->type  = type;
    t->shape = shape;
}

static inline i32 tile_get_type(tile_s *t)
{
    return (t->type & TILE_TYPE_MASK);
}

static inline i32 tile_get_shape(tile_s *t)
{
    return t->shape;
}

#endif