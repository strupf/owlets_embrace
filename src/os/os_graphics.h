#ifndef OS_GRAPHICS_H
#define OS_GRAPHICS_H

#include "os_math.h"

enum {
        TEXID_DISPLAY,
        TEXID_FONT_DEFAULT,
        //
        NUM_TEXID
};

enum {
        FNTID_DEFAULT,
        //
        NUM_FNTID
};

typedef struct {
        u8 *px;
        int w_byte;
        int w;
        int h;
} tex_s;

tex_s tex_create(int w, int h);
tex_s tex_load(const char *filename);
void  tex_put(int ID, tex_s t);
tex_s tex_get(int ID);

// ASCII encoding of printable characters
// 32:   ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
// 64: @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
// 96: ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ DEL
enum {
        FNT_GLYPH_NEWLINE = 10,
        FNT_GLYPH_SPACE   = 32,
        FNT_GLYPH_A_U     = 65, // A upper case
        FNT_GLYPH_A_L     = 97, // a lower case
        //
        NUM_FNT_GLYPHS    = 256
};

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

fntstr_s fntstr_create(int numchars, void *(*allocfunc)(size_t));
void     fnt_put(int ID, fnt_s f);
fnt_s    fnt_get(int ID);
void     fntstr_append_glyph(fntstr_s *f, int glyphID);
void     fntstr_append_ascii(fntstr_s *f, const char *txt);
int      fntstr_len(fntstr_s *f);
void     fntstr_apply_effect(fntstr_s *f, int from, int to,
                             int effect, int tick);

void gfx_sprite(tex_s src, v2_i32 pos, rec_i32 rs, int flags);
void gfx_draw_to(tex_s tex);
void gfx_rec_fill(rec_i32 r, int col);
void gfx_line(int x0, int y0, int x1, int y1, int col);
void gfx_text_ascii(fnt_s *font, const char *txt, int x, int y);
void gfx_text(fnt_s *font, fntstr_s *str, int x, int y);

#endif