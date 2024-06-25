// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "area/area.h"
#include "cam.h"
#include "gamedef.h"
#include "gameover.h"
#include "hero/hero.h"
#include "lighting.h"
#include "map_loader.h"
#include "maptransition.h"
#include "menu_screen.h"
#include "obj.h"
#include "obj/behaviour.h"
#include "particle.h"
#include "rope.h"
#include "save.h"
#include "settings.h"
#include "textbox.h"
#include "tile_map.h"
#include "title.h"
#include "water.h"
#include "wiggle.h"

#define INTERACTABLE_DIST   32
#define NUM_RESPAWNS        8
#define SAVE_TICKS          100
#define SAVE_TICKS_FADE_OUT 80

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_HOOK_PAUSE = 1 << 2,
};

#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    IT;                           \
    IT = IT->next

enum {
    SUBSTATE_NONE,
    SUBSTATE_TEXTBOX,
    SUBSTATE_MAPTRANSITION,
    SUBSTATE_GAMEOVER,
    SUBSTATE_HEROUPGRADE,
    SUBSTATE_MENUSCREEN
};

typedef struct {
    u16 y;
    u16 x1;
    u16 x2;
} weather_surface_s;

#define NUM_WEATHER_SURFACES 1024

struct game_s {
    u32               gameplay_tick;
    u32               state;
    //
    map_world_s       map_world; // layout of all map files globally
    map_worldroom_s  *map_worldroom;
    area_s            area;
    //
    gameover_s        gameover;
    maptransition_s   maptransition;
    textbox_s         textbox;
    menu_screen_s     menu_screen;
    u16               freeze_tick;
    u16               substate;
    cam_s             cam;
    flags32           events_frame;
    u32               aud_lowpass;
    bool32            hook_mode;
    //
    u16               tiles_x;
    u16               tiles_y;
    u16               pixel_x;
    u16               pixel_y;
    tile_s            tiles[NUM_TILES];
    rtile_s           rtiles[NUM_TILELAYER][NUM_TILES];
    obj_s            *obj_head_busy; // linked list
    obj_s            *obj_head_free; // linked list
    obj_s            *obj_tag[NUM_OBJ_TAGS];
    u32               obj_ndelete;
    obj_s            *obj_to_delete[NUM_OBJ];
    bool32            objrender_dirty; // resort render list?
    u32               n_objrender;
    obj_s            *obj_render[NUM_OBJ]; // sorted render array
    obj_s             obj_raw[NUM_OBJ];
    //
    i32               n_weather_surfaces;
    weather_surface_s weather_surfaces[NUM_WEATHER_SURFACES];
    //
    u16               coins_added;
    u16               coins_added_ticks;
    u16               save_ticks;
    u32               n_respawns;
    v2_i32            respawns[NUM_RESPAWNS];
    u32               n_grass;
    grass_s           grass[NUM_GRASS];
    u32               n_coinparticles;
    coinparticle_s    coinparticles[NUM_COINPARTICLE];
    u32               n_deco_verlet;
    deco_verlet_s     deco_verlet[NUM_DECO_VERLET];
    //
    i32               save_slot;
    save_s            save;
    hero_s            hero_mem;
    rope_s            rope;
    i32               n_ropes;
    rope_s            ropes[8];
    particles_s       particles;
    ocean_s           ocean;
#if LIGHTING_ENABLED
    lighting_s lighting;
#endif

    struct areaname {
        char filename[LEN_AREA_FILENAME];
        char label[64];
        i32  fadeticks;
    } areaname;
};

void   game_init(game_s *g);
void   game_tick(game_s *g);
void   game_draw(game_s *g);
void   game_resume(game_s *g);
void   game_paused(game_s *g);
//
i32    gameplay_time(game_s *g);
i32    gameplay_time_since(game_s *g, i32 t);
void   game_load_savefile(game_s *g);
bool32 game_save_savefile(game_s *g);
void   game_on_trigger(game_s *g, i32 trigger);
void   game_on_solid_appear(game_s *g);
bool32 obj_game_player_attackboxes(game_s *g, hitbox_s *boxes, i32 nb);
bool32 obj_game_player_attackbox(game_s *g, hitbox_s box);

// returns a number [0, n_frames-1]
// tick is the time variable
// freqticks is how many ticks are needed for one loop
i32 tick_to_index_freq(i32 tick, i32 n_frames, i32 freqticks);

#define NUM_FRAME_TICKS 64

typedef struct {
    u8 ticks[NUM_FRAME_TICKS];
} frame_ticks_s;

static i32 anim_frame_from_ticks(i32 ticks, frame_ticks_s *f)
{
    i32 a = 0;
    for (i32 i = 0; i < NUM_FRAME_TICKS; i++) {
        i32 t = f->ticks[i];
        if (t == 0) return (i - 1);
        a += t;
        if (ticks <= a) return i;
    }
    return 0;
}

static i32 anim_total_ticks(frame_ticks_s *f)
{
    i32 time = 0;
    for (i32 i = 0; i < NUM_FRAME_TICKS; i++) {
        i32 t = f->ticks[i];
        if (t == 0) break;
        time += t;
    }
    return time;
}

#endif