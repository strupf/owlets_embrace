// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_H
#define GAME_H

#include "area/area.h"
#include "boss/battleroom.h"
#include "boss/boss.h"
#include "cam.h"
#include "dialog.h"
#include "gamedef.h"
#include "gameover.h"
#include "hero/grapplinghook.h"
#include "hero/hero.h"
#include "hero_powerup.h"
#include "map_loader.h"
#include "maptransition.h"
#include "menu_screen.h"
#include "obj.h"
#include "particle.h"
#include "rope.h"
#include "save.h"
#include "settings.h"
#include "shop.h"
#include "tile_map.h"
#include "title.h"
#include "water.h"
#include "wiggle.h"

#define NUM_RESPAWNS        4
#define SAVE_TICKS          100
#define SAVE_TICKS_FADE_OUT 80
#define NUM_MAP_PINS        64

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_DEATH      = 1 << 2,
    EVENT_HERO_HOOK_PAUSE = 1 << 3,
};

// for (obj_each(g, o)) {}
#define obj_each(G, IT)           \
    obj_s *IT = G->obj_head_busy; \
    IT;                           \
    IT = IT->next

enum {
    SUBSTATE_NONE,
    SUBSTATE_TEXTBOX,
    SUBSTATE_MAPTRANSITION,
    SUBSTATE_GAMEOVER,
    SUBSTATE_POWERUP,
    SUBSTATE_MENUSCREEN
};

typedef struct {
    v2_i16   pos;
    texrec_s tr;
} foreground_prop_s;

#define NUM_FOREGROUND_PROPS 1024

void foreground_props_draw(g_s *g, v2_i32 cam);

typedef struct {
    u32 hash;
    i16 x; // relative to current room
    i16 y;
    u16 w;
    u16 h;
} map_neighbor_s;

struct g_s {
    i32               tick;
    u16               n_saveIDs;
    u16               saveIDs[NUM_SAVEIDS]; // unlocked things or events already happened
    u32               map_hash;
    map_pin_s         map_pins[NUM_MAP_PINS];
    v2_i32            cam_prev;
    v2_i32            cam_prev_world;
    map_neighbor_s    map_neighbors[NUM_MAP_NEIGHBORS];
    area_s            area;
    //
    gameover_s        gameover;
    maptransition_s   maptransition;
    menu_screen_s     menu_screen;
    hero_powerup_s    powerup;
    dialog_s          dialog;
    grapplinghook_s   ghook;
    u16               freeze_tick;
    u16               substate;
    cam_s             cam;
    u32               events_frame; // flags
    u32               hero_hurt_lp_tick;
    bool32            dark;
    u8                musicname[8];
    u8                mapname[32];
    //
    i32               tiles_x;
    i32               tiles_y;
    i32               pixel_x;
    i32               pixel_y;
    tile_s            tiles[NUM_TILES];
    u16               rtiles[NUM_TILELAYER][NUM_TILES];
    u8                fluid_streams[NUM_TILES];
    //
    obj_s            *obj_head_busy; // linked list
    obj_s            *obj_head_free; // linked list
    obj_s            *obj_tag[NUM_OBJ_TAGS];
    u32               obj_ndelete;
    obj_s            *obj_to_delete[NUM_OBJ];
    //
    bool16            objrender_dirty; // resort render list?
    u16               n_objrender;
    obj_s            *obj_render[NUM_OBJ]; // sorted render array
    obj_s             obj_raw[NUM_OBJ];
    //
    i32               n_foreground_props;
    foreground_prop_s foreground_props[NUM_FOREGROUND_PROPS];
    //
    boss_s            boss;
    battleroom_s      battleroom;
    u16               coins_added;
    u16               coins_added_ticks;
    u16               save_ticks;
    u32               n_grass;
    grass_s           grass[NUM_GRASS];
    u32               n_deco_verlet;
    deco_verlet_s     deco_verlet[NUM_DECO_VERLET];
    //
    grapplinghook_s   grapple;
    i32               grapple_tick;
    i32               save_slot;
    hero_s            hero;
    particles_s       particles;
    ocean_s           ocean;

    marena_s memarena;
    byte     mem[MKILOBYTE(512)];
};

void        game_init(g_s *g);
void        game_tick(g_s *g);
void        game_draw(g_s *g);
void        game_resume(g_s *g);
void        game_paused(g_s *g);
void       *game_alloc(g_s *g, usize s, usize alignment);
allocator_s game_allocator(g_s *g);
//
i32         gameplay_time(g_s *g);
i32         gameplay_time_since(g_s *g, i32 t);
void        game_load_savefile(g_s *g);
bool32      game_save_savefile(g_s *g);
void        game_on_trigger(g_s *g, i32 trigger);
void        game_on_solid_appear(g_s *g);
bool32      obj_game_enemy_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32      obj_game_player_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32      obj_game_player_attackbox(g_s *g, hitbox_s box);
void        objs_update(g_s *g);
void        objs_animate(g_s *g);
void        objs_trigger(g_s *g, i32 trigger);
void        obj_custom_draw(g_s *g, obj_s *o, v2_i32 cam);
void        obj_interact(g_s *g, obj_s *o, obj_s *ohero);
void        game_open_map(void *ctx, i32 opt);
void        game_unlock_map(g_s *g); // play cool cutscene and stuff later, too
i32         saveID_put(g_s *g, i32 ID);
bool32      saveID_has(g_s *g, i32 ID);

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