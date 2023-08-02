// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_H
#define OS_H

#include "os_fileio.h"
#include "os_math.h"
#include "os_types.h"
#include "util/array.h"

enum tex_id {
        TEXID_DISPLAY,
        TEXID_FONT_DEFAULT,
        TEXID_TILESET,
        //
        NUM_TEXID
};

enum snd_id {
        SNDID_DEFAULT = 0,
        //
        NUM_SNDID
};

enum fnt_id {
        FNTID_DEFAULT,
        //
        NUM_FNTID
};

// ASCII encoding of printable characters
// 32:   ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
// 64: @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
// 96: ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ DEL
enum fnt_glyph {
        FNT_GLYPH_NEWLINE = 10,
        FNT_GLYPH_SPACE   = 32,
        FNT_GLYPH_A_U     = 65, // A upper case
        FNT_GLYPH_A_L     = 97, // a lower case
        //
        NUM_FNT_GLYPHS    = 256
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

typedef struct {
        u8 *px;
        int w_byte;
        int w;
        int h;
} tex_s;

typedef struct {
        u16 px[16];
} tileimg_s;

typedef struct {
        tex_s tex;
        int   gridw;
        int   gridh;
        int   lineheight;
        i8    glyph_widths[NUM_FNT_GLYPHS];
} fnt_s;

typedef struct {
        u8 glyphID;
        u8 effectID;
} fntchar_s;

typedef struct {
        fntchar_s *chars;
        int        n;
        int        c;
} fntstr_s;

typedef struct {
        int x;
} snd_s;

typedef struct {
        char *p;
        char *pr;
        char *mem;
} memarena_s;

void     fnt_put(int ID, fnt_s f);
fnt_s    fnt_get(int ID);
fnt_s    fnt_load(const char *filename);
fntstr_s fntstr_create(int numchars, void *(*allocfunc)(size_t));
void     fntstr_append_glyph(fntstr_s *f, int glyphID);
void     fntstr_append_ascii(fntstr_s *f, const char *txt);
int      fntstr_len(fntstr_s *f);
void     fntstr_apply_effect(fntstr_s *f, int from, int to,
                             int effect, int tick);
//
void     tex_put(int ID, tex_s t);
tex_s    tex_get(int ID);
tex_s    tex_create(int w, int h);
tex_s    tex_load(const char *filename);
//
void     gfx_set_inverted(bool32 inv);
void     gfx_tile(tileimg_s t, v2_i32 pos, int flags);
void     gfx_sprite(tex_s src, v2_i32 pos, rec_i32 rs, int flags);
void     gfx_draw_to(tex_s tex);
void     gfx_rec_fill(rec_i32 r, int col);
void     gfx_line(int x0, int y0, int x1, int y1, int col);
void     gfx_line_thick(int x0, int y0, int x1, int y1, int r, int col);
void     gfx_text_ascii(fnt_s *font, const char *txt, int x, int y);
void     gfx_text(fnt_s *font, fntstr_s *str, int x, int y);
//
snd_s    snd_get(int ID);
//
bool32   os_inp_pressed(int b);
bool32   os_inp_pressedp(int b);
bool32   os_inp_just_released(int b);
bool32   os_inp_just_pressed(int b);
int      os_inp_crank_change();
int      os_inp_crank();
int      os_inp_crankp();
bool32   os_inp_crank_dockedp();
bool32   os_inp_crank_docked();
//
bool32   debug_inp_up();
bool32   debug_inp_down();
bool32   debug_inp_left();
bool32   debug_inp_right();
bool32   debug_inp_w();
bool32   debug_inp_a();
bool32   debug_inp_s();
bool32   debug_inp_d();
bool32   debug_inp_enter();
bool32   debug_inp_space();
//
// internal scratchpad memory stack
// just a fixed sized bump allocator
void     os_spmem_push(); // push the current state
void     os_spmem_pop();  // pop and restore previous state
void    *os_spmem_peek();
void     os_spmem_set(void *p);
void     os_spmem_clr();                    // reset bump allocator
void    *os_spmem_alloc(size_t size);       // allocate memory
void    *os_spmem_alloc_rems(size_t *size); // allocate remaining memory
void    *os_spmem_allocz(size_t size);      // allocate and zero memory
void    *os_spmem_allocz_rem(size_t *size); // allocate and zero remaining memory
//
void     memarena_init(memarena_s *m, void *buf, size_t bufsize);
void    *memarena_alloc(memarena_s *m, size_t s);
void    *memarena_alloc_rem(memarena_s *m, size_t *s);
void    *memarena_allocz(memarena_s *m, size_t s);
void    *memarena_allocz_rem(memarena_s *m, size_t *s);
void    *memarena_peek(memarena_s *m);
void     memarena_set(memarena_s *m, void *p);
void     memarena_clr(memarena_s *m);

// MEMORY REPLACEMENTS =========================================================
static inline void os_memset(void *dst, int val, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) *d++ = (char)val;
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
        for (int n = 0; n < len; n++) d[n] = 0;
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
#endif