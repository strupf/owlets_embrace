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
#include "foreground_prop.h"
#include "gamedef.h"
#include "gameover.h"
#include "hero/grapplinghook.h"
#include "hero/hero.h"
#include "hero/hero_hook.h"
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

#define INTERACTABLE_DIST   32
#define NUM_RESPAWNS        8
#define SAVE_TICKS          100
#define SAVE_TICKS_FADE_OUT 80

enum {
    EVENT_HIT_ENEMY       = 1 << 0,
    EVENT_HERO_DAMAGE     = 1 << 1,
    EVENT_HERO_HOOK_PAUSE = 1 << 2,
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

struct g_s {
    i32               gameplay_tick;
    i32               state;
    v2_i32            cam_prev;
    v2_i32            cam_prev_world;
    //
    map_world_s       map_world; // layout of all map files globally
    map_worldroom_s  *map_worldroom;
    area_s            area;
    //
    gameover_s        gameover;
    maptransition_s   maptransition;
    menu_screen_s     menu_screen;
    hero_powerup_s    powerup;
    dialog_s          dialog;
    u16               freeze_tick;
    u16               substate;
    cam_s             cam;
    u32               events_frame; // flags
    u32               hero_hurt_lowpass_tick;
    bool32            dark;
    //
    i32               tiles_x;
    i32               tiles_y;
    i32               pixel_x;
    i32               pixel_y;
    tile_s            tiles[NUM_TILES];
    rtile_s           rtiles[NUM_TILELAYER][NUM_TILES];
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
    u32               n_respawns;
    v2_i16            respawns[NUM_RESPAWNS];
    u32               n_grass;
    grass_s           grass[NUM_GRASS];
    u32               n_deco_verlet;
    deco_verlet_s     deco_verlet[NUM_DECO_VERLET];
    u32               n_ladders;
    //
    grapplinghook_s   grapple;
    i32               save_slot;
    save_s            save;
    hero_s            hero_mem;
    rope_s            rope;
    particles_s       particles;
    ocean_s           ocean;

    struct areaname {
        char filename[LEN_AREA_FILENAME];
        char label[64];
        i32  fadeticks;
    } areaname;

    marena_s        memarena;
    ALIGNAS(4) byte mem[MKILOBYTE(1024)];
};

void   game_init(g_s *g);
void   game_tick(g_s *g);
void   game_draw(g_s *g);
void   game_resume(g_s *g);
void   game_paused(g_s *g);
// allocate static memory (per level)
void  *game_alloc_aligned(g_s *g, usize s, usize alignment);
void  *game_alloc(g_s *g, usize s);
void   game_allocator_reset(g_s *g);
//
i32    gameplay_time(g_s *g);
i32    gameplay_time_since(g_s *g, i32 t);
void   game_load_savefile(g_s *g);
bool32 game_save_savefile(g_s *g);
void   game_on_trigger(g_s *g, i32 trigger);
void   game_on_solid_appear(g_s *g);
bool32 obj_game_enemy_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32 obj_game_player_attackboxes(g_s *g, hitbox_s *boxes, i32 nb);
bool32 obj_game_player_attackbox(g_s *g, hitbox_s box);
void   objs_update(g_s *g);
void   objs_animate(g_s *g);
void   objs_trigger(g_s *g, i32 trigger);
void   obj_custom_draw(g_s *g, obj_s *o, v2_i32 cam);
void   obj_interact(g_s *g, obj_s *o, obj_s *ohero);
void   game_open_map(void *ctx, i32 opt);
void   game_unlock_map(g_s *g); // play cool cutscene and stuff later, too

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