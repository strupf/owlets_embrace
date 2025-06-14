// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_TYPES_H
#define TILE_TYPES_H

#include "gamedef.h"

enum {
    TILE_TYPE_NONE                     = 0,
    TILE_TYPE_INVISIBLE_NON_CONNECTING = 1,
    TILE_TYPE_INVISIBLE_CONNECTING     = 2,
    //
    TILE_TYPE_DARK_LEAVES              = 3,
    TILE_TYPE_DARK_STONE               = 4,
    TILE_TYPE_DARK_STONE_PEBBLE        = 5,
    TILE_TYPE_DARK_OBSIDIAN            = 6,
    TILE_TYPE_DARK_2                   = 7,
    TILE_TYPE_DARK_3                   = 8,
    TILE_TYPE_DARK_4                   = 9,
    TILE_TYPE_DARK_5                   = 10,
    //
    TILE_TYPE_BRIGHT_STONE             = 11,
    TILE_TYPE_BRIGHT_SNOW              = 12,
    TILE_TYPE_BRIGHT_BREAKING          = 13,
    TILE_TYPE_BRIGHT_STOMP             = 15,
    TILE_TYPE_BRIGHT_STOMP_2           = 16,
    TILE_TYPE_BRIGHT_STOMP_HOR         = 17,
    //
    TILE_TYPE_THORNS                   = 19,
    TILE_TYPE_CRUMBLE                  = 22,
    //
    NUM_TILE_TYPES                     = 24
};

enum {
    TILE_TYPE_COLOR_BRIGHT,
    TILE_TYPE_COLOR_DARK,
    TILE_TYPE_COLOR_BLACK
};

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

#endif