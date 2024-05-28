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
#include "item_select.h"
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
#include "shop.h"
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
    SUBSTATE_MENUSCREEN,
};

struct game_s {
    i32              gameplay_tick;
    settings_s       settings;
    title_s          title;
    i32              state;
    //
    map_world_s      map_world; // layout of all map files globally
    map_worldroom_s *map_worldroom;
    area_s           area;
    //
    shop_s           shop;
    gameover_s       gameover;
    maptransition_s  maptransition;
    textbox_s        textbox;
    menu_screen_s    menu_screen;
    u16              freeze_tick;
    u16              substate;
    //
    cam_s            cam;
    flags32          events_frame;
    //
    i32              tiles_x;
    i32              tiles_y;
    i32              pixel_x;
    i32              pixel_y;
    tile_s           tiles[NUM_TILES];
    rtile_s          rtiles[NUM_TILELAYER][NUM_TILES];
    u16              obj_ndelete;
    u16              n_objrender;
    bool32           objrender_dirty;
    obj_s           *obj_head_busy; // linked list
    obj_s           *obj_head_free; // linked list
    obj_s           *obj_tag[NUM_OBJ_TAGS];
    obj_s           *obj_to_delete[NUM_OBJ];
    obj_s           *obj_render[NUM_OBJ]; // sorted render array
    obj_s            obj_raw[NUM_OBJ];
    //
    i32              coins_added;
    i32              coins_added_ticks;
    i32              save_ticks;
    //
    i32              n_respawns;
    v2_i32           respawns[NUM_RESPAWNS];
    //
    i32              n_grass;
    grass_s          grass[NUM_GRASS];
    //
    i32              n_wiggle_deco;
    wiggle_deco_s    wiggle_deco[NUM_WIGGLE];
    //
    i32              n_coinparticles;
    coinparticle_s   coinparticles[NUM_COINPARTICLE];
    //
    save_s           save;
    hero_s           hero_mem;
    hero_jump_ui_s   jump_ui;
    particles_s      particles;
    ocean_s          ocean;
    lighting_s       lighting;

    item_select_s item_select;

    struct {
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
bool32 game_load_savefile(game_s *g);
bool32 game_save_savefile(game_s *g);
void   game_on_trigger(game_s *g, i32 trigger);
void   game_on_solid_appear(game_s *g);
void   obj_game_player_attackboxes(game_s *g, hitbox_s *boxes, i32 nb);
void   obj_game_player_attackbox(game_s *g, hitbox_s box);

// returns a number [0, n_frames-1]
// tick is the time variable
// freqticks is how many ticks are needed for one loop
i32    tick_to_index_freq(i32 tick, i32 n_frames, i32 freqticks);
obj_s *obj_closest_interactable(game_s *g, v2_i32 pos);

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