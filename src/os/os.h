// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_H
#define OS_H

#define OS_SHOW_FPS    0
#define OS_SHOW_TIMING 0

enum {
        OS_FPS     = 50,
        OS_FPS_LOW = OS_FPS / 2,
};

#include "assets.h"
#include "os_fileio.h"
#include "os_math.h"
#include "os_types.h"
#include "util/array.h"
#include "util/memfunc.h"

// ASCII encoding of printable characters
// 32:   ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
// 64: @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
// 96: ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ DEL
enum fnt_glyph {
        FNT_GLYPH_NEWLINE  = 10,
        FNT_GLYPH_SPACE    = 32,
        FNT_GLYPH_A_U      = 65, // A upper case
        FNT_GLYPH_A_L      = 97, // a lower case
        FNT_GLYPH_BUTTON_A = 224,
        FNT_GLYPH_BUTTON_B = 225,
        FNT_GLYPH_DPAD     = 226,
        //
        NUM_FNT_GLYPHS     = 256
};

enum fnt_effect {
        FNT_EFFECT_NONE,
        FNT_EFFECT_WAVE,
        FNT_EFFECT_SHAKE,
};

enum inp_button {
#if defined(TARGET_PD)
        INP_LEFT  = kButtonLeft,
        INP_RIGHT = kButtonRight,
        INP_UP    = kButtonUp,
        INP_DOWN  = kButtonDown,
        INP_A     = kButtonA,
        INP_B     = kButtonB,
#elif defined(TARGET_DESKTOP)
        INP_LEFT  = 0x01,
        INP_RIGHT = 0x02,
        INP_UP    = 0x04,
        INP_DOWN  = 0x08,
        INP_A     = 0x10,
        INP_B     = 0x20,
#endif
};

enum inp_dpad_direction {
        INP_DPAD_NONE,
        INP_DPAD_N,
        INP_DPAD_S,
        INP_DPAD_E,
        INP_DPAD_W,
        INP_DPAD_NE,
        INP_DPAD_NW,
        INP_DPAD_SE,
        INP_DPAD_SW,
};

enum {
        SPRITE_CPY, // copy
        SPRITE_W_T, // white transparent
        SPRITE_B_T, // black transparent
        SPRITE_W_F, // fill white
        SPRITE_B_F, // fill black
        SPRITE_XOR, // xor
        SPRITE_NXR, // nxor
        SPRITE_INV, // invert
};

enum {
        SPRITE_FLIP_Y  = 1,
        SPRITE_FLIP_X  = 2,
        SPRITE_FLIP_XY = SPRITE_FLIP_X | SPRITE_FLIP_Y,
};

// primitive color modes, relevant for patterns
enum {
        GFX_PRIM_BW,                // 1 in pattern is black, 0 is white
        GFX_PRIM_BLACK_TRANSPARENT, // 1 in pattern is black, 0 is transparent
        GFX_PRIM_WHITE_TRANSPARENT, // 1 in pattern is white, 0 is transparent
};

enum {
        GFX_MODE_CPY, // copy
        GFX_MODE_W_T, // white transparent
        GFX_MODE_B_T, // black transparent
        GFX_MODE_W_F, // fill white
        GFX_MODE_B_F, // fill black
        GFX_MODE_XOR, // xor
        GFX_MODE_NXR, // nxor
        GFX_MODE_INV, // invert
};

enum timing_IDs {
        TIMING_UPDATE,
        TIMING_SOLID_UPDATE,
        TIMING_HERO_UPDATE,
        TIMING_HERO_MOVE,
        TIMING_HERO_HOOK,
        TIMING_ROPE,
        TIMING_DRAW,
        TIMING_DRAW_TILES,
        //
        NUM_TIMING
};

#if OS_SHOW_TIMING
#define TIMING_BEGIN(ID)    i_time_begin(ID)
#define TIMING_END          i_time_end
#define TIMING_DRAW_DIAGRAM i_time_draw
#define TIMING_TICK_DIAGRAM i_time_tick
void i_time_begin(int ID);
void i_time_end();
void i_time_draw();
void i_time_tick();
#else
#define TIMING_BEGIN(ID)
#define TIMING_END()
#define TIMING_DRAW_DIAGRAM()
#define TIMING_TICK_DIAGRAM()
#endif

typedef struct {
        u8 *px;
        u8 *mk;
        int w_word;
        int w_byte;
        int w;
        int h;
} tex_s;

extern tex_s g_tex_screen;
extern tex_s g_tex_layer_1;
extern tex_s g_tex_layer_2;
extern tex_s g_tex_layer_3;

typedef struct {
        tex_s   t;
        rec_i32 r;
} texregion_s;

typedef struct {
        float m[9];
} sprite_matrix_s;

sprite_matrix_s sprite_matrix_identity();
sprite_matrix_s sprite_matrix_add(sprite_matrix_s a, sprite_matrix_s b);
sprite_matrix_s sprite_matrix_sub(sprite_matrix_s a, sprite_matrix_s b);
sprite_matrix_s sprite_matrix_mul(sprite_matrix_s a, sprite_matrix_s b);
sprite_matrix_s sprite_matrix_rotate(float angle);
sprite_matrix_s sprite_matrix_scale(float scx, float scy);
sprite_matrix_s sprite_matrix_shear(float shx, float shy);
sprite_matrix_s sprite_matrix_offset(float x, float y);

typedef struct {
        u32 p[8];
} gfx_pattern_s;

typedef struct {
        tex_s         dst;
        tex_s         src;
        gfx_pattern_s pat;
        int           col; // color for primitive drawing
        int           sprmode;
} gfx_context_s;

enum {
        GFX_COL_WHITE = 0,
        GFX_COL_BLACK = 1,
        GFX_COL_CLEAR = 2,
        GFX_COL_NXOR  = 3,
};

enum {
        GFX_PATTERN_0,
        GFX_PATTERN_6,
        GFX_PATTERN_13,
        GFX_PATTERN_19,
        GFX_PATTERN_25,
        GFX_PATTERN_31,
        GFX_PATTERN_38,
        GFX_PATTERN_44,
        GFX_PATTERN_50,
        GFX_PATTERN_56,
        GFX_PATTERN_63,
        GFX_PATTERN_69,
        GFX_PATTERN_75,
        GFX_PATTERN_81,
        GFX_PATTERN_88,
        GFX_PATTERN_94,
        GFX_PATTERN_100,
        //
        NUM_GFX_PATTERN,
        GFX_PATTERN_NONE = GFX_PATTERN_0,
        GFX_PATTERN_FULL = GFX_PATTERN_100,
};

extern const gfx_pattern_s g_gfx_patterns[NUM_GFX_PATTERN];

#define gfx_pattern_get(ID) g_gfx_patterns[ID]

gfx_pattern_s gfx_pattern_set_8x8(int p0, int p1, int p2, int p3,
                                  int p4, int p5, int p6, int p7);

typedef struct {
        tex_s tex;
        int   gridw;
        int   gridh;
        int   lineheight;
        i8    glyph_widths[NUM_FNT_GLYPHS];
} fnt_s;

typedef struct {
        u8  glyphID;
        u8  effectID;
        u16 effecttick;
} fntchar_s;

typedef struct {
        fntchar_s *chars;
        int        n;
        int        c;
} fntstr_s;

typedef struct {
        i16 *data;
        int  len;
} snd_s;

enum {
        WAVE_TYPE_SINE,
        WAVE_TYPE_SQUARE,
        WAVE_TYPE_TRI,
};

fnt_s         fnt_put_load(int ID, const char *filename);
void          fnt_put(int ID, fnt_s f);
fnt_s         fnt_get(int ID);
fnt_s         fnt_load(const char *filename);
fntstr_s      fntstr_create(int numchars, void *(*allocfunc)(size_t));
int           fntlength_px(fnt_s *font, fntchar_s *chars, int l);
int           fntlength_px_ascii(fnt_s *font, const char *txt, int l);
fntchar_s     fntchar_from_glyphID(int glyphID);
bool32        fntstr_append_glyph(fntstr_s *f, int glyphID);
int           fntstr_append_ascii(fntstr_s *f, const char *txt);
int           fntstr_len(fntstr_s *f);
void          fntstr_apply_effect(fntstr_s *f, int from, int to,
                                  int effect, int tick);
//
tex_s         tex_put_load(int ID, const char *filename);
tex_s         tex_put(int ID, tex_s t);
tex_s         tex_get(int ID);
tex_s         tex_create(int w, int h, bool32 mask);
tex_s         tex_load(const char *filename);
tex_s         tex_new_screen_layer();
//
gfx_context_s gfx_context_create(tex_s dst);
void          gfx_tex_clr(tex_s t);
void          gfx_set_inverted(bool32 inv);
void          gfx_px(gfx_context_s ctx, int x, int y);
void          gfx_sprite_tile16(gfx_context_s ctx, v2_i32 pos, rec_i32 rs, int flags);
void          gfx_sprite(gfx_context_s ctx, v2_i32 pos, rec_i32 rs, int flags);
void          gfx_sprite_rotated_(gfx_context_s ctx, v2_i32 pos, rec_i32 r, v2_i32 origin, float angle);
void          gfx_sprite_rotated(gfx_context_s ctx, rec_i32 r, sprite_matrix_s mt);
void          gfx_rec_fill(gfx_context_s ctx, rec_i32 r);
void          gfx_tri_fill(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2);
void          gfx_tri(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2);
void          gfx_tri_thick(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2, int r);
void          gfx_line(gfx_context_s ctx, v2_i32 p0, v2_i32 p1);
void          gfx_line_thick(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, int r);
void          gfx_text_ascii(gfx_context_s ctx, fnt_s *font, const char *txt, v2_i32 p);
void          gfx_text(gfx_context_s ctx, fnt_s *font, fntstr_s *str, v2_i32 p);
void          gfx_text_glyphs(gfx_context_s ctx, fnt_s *font, fntchar_s *chars, int l, v2_i32 p);
//
snd_s         snd_put_load(int ID, const char *filename);
void          mus_play(const char *filename);
void          mus_close();
bool32        mus_playing();
void          mus_fade_out(int ticks);
void          mus_fade_in(const char *filename, int ticks);
void          mus_fade_to(const char *filename);
snd_s         snd_get(int ID);
void          snd_put(int ID, snd_s s);
snd_s         snd_load_wav(const char *filename);
void          snd_play_ext(snd_s s, float vol, float pitch);
void          snd_play(snd_s s);
//
i32           os_tick();
bool32        os_low_fps(); // if game is running slow
int           os_inp_raw();
int           os_inpp_raw();
void          os_inp_set_pressedp(int b); // deactivate just pressed for the current frame
int           os_inp_dpad_direction();
int           os_inp_dpad_x(); // returns -1 (left), 0 or +1 (right)
int           os_inp_dpad_y(); // returns -1 (up), 0 or +1 (down)
bool32        os_inp_pressed(int b);
bool32        os_inp_pressedp(int b);
bool32        os_inp_just_released(int b);
bool32        os_inp_just_pressed(int b);
int           os_inp_crank_change(); // crank angle [0, 65535]
int           os_inp_crank();        // crank angle [0, 65535]
int           os_inp_crankp();       // crank angle [0, 65535]
bool32        os_inp_crank_dockedp();
bool32        os_inp_crank_docked();
//
bool32        debug_inp_up();
bool32        debug_inp_down();
bool32        debug_inp_left();
bool32        debug_inp_right();
bool32        debug_inp_w();
bool32        debug_inp_a();
bool32        debug_inp_s();
bool32        debug_inp_d();
bool32        debug_inp_enter();
bool32        debug_inp_space();
//
// internal scratchpad memory stack
// just a fixed sized bump allocator
void          os_spmem_push(); // push the current state
void          os_spmem_pop();  // pop and restore previous state
void         *os_spmem_peek();
void          os_spmem_set(void *p);
void          os_spmem_clr();                    // reset bump allocator
void         *os_spmem_alloc(size_t size);       // allocate memory
void         *os_spmem_alloc_rems(size_t *size); // allocate remaining memory
void         *os_spmem_allocz(size_t size);      // allocate and zero memory
void         *os_spmem_allocz_rem(size_t *size); // allocate and zero remaining memory

// MEMORY REPLACEMENTS =========================================================
static inline void os_memset4(void *dst, int val, size_t l)
{
        ASSERT(((uintptr_t)dst & 3) == 0);
        ASSERT((l & 3) == 0);
        ASSERT(sizeof(int) == 4);

        char c[4] = {val, val, val, val};
        int  v    = *(int *)c;
        int *d    = (int *)dst;
        int  len  = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = v;
}

static inline void os_memclr4(void *dst, size_t l)
{
        ASSERT(((uintptr_t)dst & 3) == 0);
        ASSERT((l & 3) == 0);
        ASSERT(sizeof(int) == 4);

        int *d   = (int *)dst;
        int  len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = 0;
}

static inline void *os_memcpy4(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        ASSERT(((uintptr_t)dst & 3) == 0);
        ASSERT(((uintptr_t)src & 3) == 0);
        ASSERT((l & 3) == 0);
        ASSERT(sizeof(int) == 4);

        int       *d   = (int *)dst;
        const int *s   = (const int *)src;
        int        len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = *s++;
        return dst;
}

static inline void os_memset(void *dst, int val, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) *d++ = val;
}

static inline void *os_memcpy(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        char       *d   = (char *)dst;
        const char *s   = (const char *)src;
        int         len = (int)l;
        for (int n = 0; n < len; n++) *d++ = *s++;
        return dst;
}

static inline void *os_memmov(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        char       *d   = (char *)dst;
        const char *s   = (const char *)src;
        int         len = (int)l;
        if (d < s)
                for (int n = 0; n < len; n++) *d++ = *s++;
        else
                for (int n = len - 1; n >= 0; n--) *d-- = *s--;
        return dst;
}

static inline void os_memclr(void *dst, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) *d++ = 0;
}

#endif