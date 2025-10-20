// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OBJ_H
#define OBJ_H

#include "gamedef.h"
#include "objdef.h"
#include "owl/grapplinghook.h"
#include "particle.h"
#include "wire.h"

#define NUM_OBJ       256
#define OBJ_MEM_BYTES 512

// internal flags by engine (separate field)
#define OBJ_IFLAG_TO_DELETE        ((u32)1 << 0)
//
// user flags
#define OBJ_FLAG_DONT_UPDATE       ((u32)1 << 0)
#define OBJ_FLAG_DONT_SHOW         ((u32)1 << 1)
#define OBJ_FLAG_INTERACTABLE      ((u32)1 << 2)
#define OBJ_FLAG_PLATFORM          ((u32)1 << 3)
#define OBJ_FLAG_OWL_PLATFORM      ((u32)1 << 4) // platform for the hero only
#define OBJ_FLAG_OWL_STOMPABLE     ((u32)1 << 5) // platform for the hero only while stomping
#define OBJ_FLAG_OWL_JUMPABLE      ((u32)1 << 6) // platform for the hero only while falling
#define OBJ_FLAG_KILL_OFFSCREEN    ((u32)1 << 7)
#define OBJ_FLAG_HOOKABLE          ((u32)1 << 8)
#define OBJ_FLAG_ENEMY             ((u32)1 << 9)
#define OBJ_FLAG_HURT_ON_TOUCH     ((u32)1 << 10)
#define OBJ_FLAG_DESTROY_HOOK      ((u32)1 << 11)
#define OBJ_FLAG_CLAMP_ROOM_X      ((u32)1 << 12)
#define OBJ_FLAG_CLAMP_ROOM_Y      ((u32)1 << 13)
#define OBJ_FLAG_CLIMBABLE         ((u32)1 << 14)
#define OBJ_FLAG_LIGHT             ((u32)1 << 15)
#define OBJ_FLAG_GRAB              ((u32)1 << 16)
#define OBJ_FLAG_PUSHABLE          ((u32)1 << 17)
#define OBJ_FLAG_HITTABLE_BY_OWL   ((u32)1 << 18)
#define OBJ_FLAG_HITTABLE_BY_ENEMY ((u32)1 << 19)
#define OBJ_FLAG_ACTOR             ((u32)1 << 30)
#define OBJ_FLAG_SOLID             ((u32)1 << 31)

#define OBJ_FLAG_PUSHABLE_SOLID    (OBJ_FLAG_PUSHABLE | OBJ_FLAG_SOLID)
#define OBJ_FLAG_DONT_SHOW_UPDATE  (OBJ_FLAG_DONT_UPDATE | OBJ_FLAG_DONT_SHOW)
#define OBJ_FLAG_GRABBABLE_SOLID   (OBJ_FLAG_SOLID | OBJ_FLAG_GRAB)
#define OBJ_FLAG_HOOKABLE_SOLID    (OBJ_FLAG_HOOKABLE | OBJ_FLAG_SOLID)
#define OBJ_FLAG_HOOKABLE_ACTOR    (OBJ_FLAG_HOOKABLE | OBJ_FLAG_ACTOR)
#define OBJ_FLAG_PLATFORM_ANY      (OBJ_FLAG_PLATFORM | OBJ_FLAG_OWL_PLATFORM)
#define OBJ_FLAG_OWL_JUMPSTOMPABLE (OBJ_FLAG_OWL_STOMPABLE | OBJ_FLAG_OWL_JUMPABLE)
#define OBJ_FLAG_CLAMP_TO_ROOM     (OBJ_FLAG_CLAMP_ROOM_X | OBJ_FLAG_CLAMP_ROOM_Y)

enum {
    OBJ_BUMP_X_NEG     = 1 << 0,
    OBJ_BUMP_X_POS     = 1 << 1,
    OBJ_BUMP_Y_NEG     = 1 << 2,
    OBJ_BUMP_Y_POS     = 1 << 3,
    OBJ_BUMP_SQUISH    = 1 << 4,
    OBJ_BUMP_ON_HEAD   = 1 << 5,
    OBJ_BUMP_X_BOUNDS  = 1 << 6,
    OBJ_BUMP_Y_BOUNDS  = 1 << 7,
    OBJ_BUMP_JUMPED_ON = 1 << 8,
    OBJ_BUMP_X         = OBJ_BUMP_X_NEG | OBJ_BUMP_X_POS,
    OBJ_BUMP_Y         = OBJ_BUMP_Y_NEG | OBJ_BUMP_Y_POS,
    OBJ_BUMP_XY        = OBJ_BUMP_X | OBJ_BUMP_Y
};

static inline i32 obj_bump_x_flag(i32 x)
{
    if (0 < x) return OBJ_BUMP_X_POS;
    if (x < 0) return OBJ_BUMP_X_NEG;
    return 0;
}

static inline i32 obj_bump_y_flag(i32 y)
{
    if (0 < y) return OBJ_BUMP_Y_POS;
    if (y < 0) return OBJ_BUMP_Y_NEG;
    return 0;
}

enum obj_mover_e {
    OBJ_MOVER_GLUE_GROUND        = 1 << 0,
    OBJ_MOVER_ONE_WAY_PLAT       = 1 << 1,
    OBJ_MOVER_SLIDE_Y_POS        = 1 << 2,
    OBJ_MOVER_SLIDE_Y_NEG        = 1 << 3,
    OBJ_MOVER_SLIDE_X_POS        = 1 << 4,
    OBJ_MOVER_SLIDE_X_NEG        = 1 << 5,
    OBJ_MOVER_AVOID_HEADBUMP     = 1 << 6,
    OBJ_MOVER_TERRAIN_COLLISIONS = 1 << 7,
};

typedef struct {
    ALIGNAS(32)
    texrec_s trec;
    v2_i16   offs;
    i16      flip;
} obj_sprite_s;

#define OBJ_NUM_HITBOXID 8

typedef void (*obj_action_f)(g_s *g, obj_s *o);
typedef void (*obj_value_f)(g_s *g, obj_s *o, i32 v);
typedef void (*obj_enemy_hurt_f)(g_s *g, obj_s *o, i32 dmg);
typedef void (*obj_draw_f)(g_s *g, obj_s *o, v2_i32 cam);
typedef void (*obj_pushpull_f)(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
typedef bool32 (*obj_pushpull_blocked_f)(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
typedef void (*obj_impulse_f)(g_s *g, obj_s *o, i32 x_q8, i32 y_q8);
typedef void (*obj_pushed_by_solid_f)(g_s *g, obj_s *o, obj_s *osolid, i32 sx, i32 sy);
typedef void (*obj_on_hitbox_f)(g_s *g, obj_s *o, hitbox_res_s res);

typedef struct enemy_s {
    obj_action_f on_hurt; // void f(g_s *g, obj_s *o);
    b8           hurt_on_jump;
    b8           explode_on_death;
    b8           coins_on_death;
    u8           sndID_hurt;
    u8           sndID_die;
    i8           hurt_tick; // 0< hurt, <0 die
    u8           flash_tick;
    u8           die_tick_max;
    u8           hurt_tick_max;
    u8           hero_hitID;
} enemy_s;

#define OBJ_MAGIC U32_C(0xABABABAB)

struct obj_s {
    ALIGNAS(32)
    obj_s *next;           // 4 - 4  main linked list
    obj_s *prev;           // 4 - 8  main linked list
    u32    flags_internal; // 4 - 12
    u32    flags;          // 4 - 16
    u16    ID;             // 2 - 18 type of object
    u16    subID;          // 2 - 20 subtype of object
    u32    generation;     // 4 - 24 how often this object was instantiated
    u32    editorUID;      // 4 - 28 unique map editorID

    obj_action_f           on_update;           // void f(g_s *g, obj_s *o);
    obj_action_f           on_animate;          // void f(g_s *g, obj_s *o);
    obj_draw_f             on_draw;             // void f(g_s *g, obj_s *o, v2_i32 cam);
    obj_value_f            on_trigger;          // void f(g_s *g, obj_s *o, i32 trigger);
    obj_pushpull_f         on_pushpull;         // void f(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
    obj_pushpull_blocked_f on_pushpull_blocked; // bool32 f(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
    obj_action_f           on_squish;           // void f(g_s *g, obj_s *o);
    obj_action_f           on_touchhurt_hero;   // void f(g_s *g, obj_s *o);
    obj_action_f           on_interact;         // void f(g_s *g, obj_s *o);
    obj_value_f            on_hook;             // void f(g_s *g, obj_s *o, i32 state);
    obj_value_f            on_grab;             // void f(g_s *g, obj_s *o, i32 state);
    obj_impulse_f          on_impulse;          // void f(g_s *g, obj_s *o, i32 x_q8, i32 y_q8);
    obj_pushed_by_solid_f  on_pushed_by_solid;  // void f(g_s *g, obj_s *o, obj_s *osolid, i32 sx, i32 sy);
    obj_action_f           on_carried_removed;  // void f(g_s *g, obj_s *o);

    ALIGNAS(32)
    v2_i32 pos;  // position in pixels
    v2_i32 ppos; // position in pixels
    i16    w;
    i16    h;
    v2_i32 subpos_q12; // subpixel used for movement; more like an accumulator
    v2_i32 v_q12;

    i32 steer_v_max_q12;

    // some generic behaviour fields
    u16              bumpflags; // has to be cleared manually
    u16              moverflags;
    i16              state;
    i16              substate;
    i16              action;
    i32              animation;
    i32              timer;
    i32              subtimer;
    i16              cam_attract_r;
    i8               facing; // -1 left, +1 right
    u8               render_priority;
    i16              health;
    i16              health_max;
    u8               light_radius;
    u8               light_strength;
    b8               blinking;
    u8               pushable_weight;
    v2_i8            offs_interact_ui; // offset of interaction UI
    v2_i8            offs_interact;    // offset of interaction anchor (logic) from center
    enemy_s          enemy;
    obj_handle_s     linked_solid;
    particle_emit_s *emitter;

    ALIGNAS(32)
    u8           n_sprites;
    obj_sprite_s sprites[4];

    ALIGNAS(32)
    u32             hitboxUID_registered[OBJ_NUM_HITBOXID];
    u16             hitbox_flags_group;
    u16             n_hitboxUID_registered;
    obj_on_hitbox_f on_hitbox;

    ALIGNAS(8)
    wirenode_s *wirenode;
    wire_s     *wire;

    ALIGNAS(32)
    u8           n_ignored_solids;
    obj_handle_s ignored_solids[4];

    void *heap;
    ALIGNAS(32)
    byte mem[OBJ_MEM_BYTES];
    u32  magic;
};

#define obj_get_owl(G)  obj_get_tagged(G, OBJ_TAG_OWL)
#define obj_get_comp(G) obj_get_tagged(G, OBJ_TAG_COMPANION)

obj_handle_s handle_from_obj(obj_s *o);
obj_s       *obj_from_handle(obj_handle_s h);
bool32       obj_try_from_handle(obj_handle_s h, obj_s **o_out);
bool32       obj_handle_valid(obj_handle_s h);
obj_s       *obj_create(g_s *g);
void         obj_delete(g_s *g, obj_s *o); // only flags for deletion -> deleted at end of frame
void         obj_handle_delete(g_s *g, obj_handle_s h);
bool32       obj_tag(g_s *g, obj_s *o, i32 tag);
bool32       obj_untag(g_s *g, obj_s *o, i32 tag);
obj_s       *obj_get_tagged(g_s *g, i32 tag);
void         objs_cull_to_delete(g_s *g); // removes all flagged objects
bool32       overlap_obj(obj_s *a, obj_s *b);
rec_i32      obj_aabb(obj_s *o);
rec_i32      obj_rec_left(obj_s *o);
rec_i32      obj_rec_right(obj_s *o);
rec_i32      obj_rec_bottom(obj_s *o);
rec_i32      obj_rec_top(obj_s *o);
rec_i32      obj_rec_x_leading(obj_s *o, i32 dx);
rec_i32      obj_rec_y_leading(obj_s *o, i32 dy);
v2_i32       obj_pos_bottom_center(obj_s *o);
v2_i32       obj_pos_center(obj_s *o);
void         obj_move(g_s *g, obj_s *o, i32 dx, i32 dy);
bool32       obj_step_is_clamped(g_s *g, obj_s *o, i32 sx, i32 sy);
void         obj_move_by_v_q12(g_s *g, obj_s *o);
void         obj_step_solid(g_s *g, obj_s *o, i32 sx, i32 sy);
b32          obj_step_actor(g_s *g, obj_s *o, i32 sx, i32 sy);
bool32       obj_try_wiggle(g_s *g, obj_s *o);
bool32       obj_grounded(g_s *g, obj_s *o);
bool32       obj_grounded_at_offs(g_s *g, obj_s *o, v2_i32 offs);
bool32       obj_grounded_at_offs_xy(g_s *g, obj_s *o, i32 ox, i32 oy);
bool32       obj_try_move_grounded_sideways_without_falling(g_s *g, obj_s *o, i32 sx, bool32 slopes);
bool32       obj_would_fall_down_next(g_s *g, obj_s *o, i32 xdir); // not on ground returns false
void         squish_delete(g_s *g, obj_s *o);
void         obj_move_by_q12(g_s *g, obj_s *o, i32 dx_q8, i32 dy_q8);
void         obj_v_q8_mul(obj_s *o, i32 mx_q8, i32 my_q8);
void         obj_vx_q8_mul(obj_s *o, i32 mx_q8);
void         obj_vy_q8_mul(obj_s *o, i32 my_q8);
void         obj_v_q12_mul(obj_s *o, i32 mx_q12, i32 my_q12);
void         obj_vx_q12_mul(obj_s *o, i32 mx_q12);
void         obj_vy_q12_mul(obj_s *o, i32 my_q12);
bool32       obj_on_platform(g_s *g, obj_s *o, i32 x, i32 y, i32 w);
bool32       blocked_excl_offs(g_s *g, rec_i32 r, obj_s *o, i32 dx, i32 dy);
bool32       blocked_excl(g_s *g, rec_i32 r, obj_s *o);
bool32       obj_ignores_solid(obj_s *oactor, obj_s *osolid, i32 *index);
enemy_s      enemy_default();
void         enemy_hurt(g_s *g, obj_s *o, i32 dmg);
i32          obj_distsq(obj_s *a, obj_s *b);    // between centers
i32          obj_dist_appr(obj_s *a, obj_s *b); // between centers
v2_i32       obj_solid_align_pos_for_render(g_s *g, v2_i32 p);
bool32       obj_pushpull_blocked_default(g_s *g, obj_s *o, i32 dt_x, i32 dt_y);
void         enemy_on_update_die(g_s *g, obj_s *o);
obj_s       *obj_find_ID_subID(g_s *g, i32 ID, i32 subID, obj_s *o_from); // find obj with ID, and optionally subID; o_from == null to start from the beginning, or start one after o_from
// whether one object is standing on top of another
// offx/y added to position of o_plat
bool32       obj_standing_on(obj_s *o_standing_on, obj_s *o_plat, i32 offx, i32 offy);

bool32 obj_hit_by_cir(obj_s *o, i32 cx, i32 cy, i32 cr);
bool32 obj_hit_by_rec(obj_s *o, i32 rx, i32 ry, i32 rw, i32 rh);
bool32 obj_hit_by_ray(obj_s *o, i32 p0x, i32 p0y, i32 p1x, i32 p1y, i32 cr);

typedef struct {
    v2_i32 f;
    i32    w;
} obj_steer_force_s;

typedef struct obj_steering_s {
    obj_s            *o;
    i32               m_q8;
    i32               v_q12_max;
    i32               n_forces;
    obj_steer_force_s forces[8];
} obj_steering_s;

#if 0
void obj_steering_init(obj_steering_s* s, obj_s* o, i32 vmax_q12, i32 m_q8) {
    mclr(s, sizeof(obj_steering_s));
    s->o = o;
    s->m_q8 = m_q8;
    s->v_q12_max = vmax_q12;
}

void obj_steering_seek(obj_steering_s* s, v2_i32 p) {
    v2_i32 v = steer_seek(obj_pos_center(s->o), s->o->v_q12, p, s->v_q12_max);
    s->forces[s->n_forces++].f = v;
}

void obj_steering_arrive(obj_steering_s *s, v2_i32 p, i32 r)
{
    v2_i32 v                   = steer_arrival(obj_pos_center(s->o), s->o->v_q12, p, s->v_q12_max, r);
    s->forces[s->n_forces++].f = v;
}

void obj_steering_apply(g_s *g, obj_steering_s *s)
{
    v2_i32 steer   = {0};
    i32    w_total = 0;
    for (i32 n = 0; n < s->n_forces; n++) {
        steer = v2_i32_add(steer, s->forces[n].f);
    }
    s->n_forces = 0;

    // f = m * a
    // a = f / m

    v2_i32 a = {div_rounded_i32(steer.x * s->m_q8, 256),
                div_rounded_i32(steer.y * s->m_q8, 256)};
    s->o->v_q12.x += a.x;
    s->o->v_q12.y += a.y;
    obj_move_by_v_q12(g, s->o);

}
#endif
#endif