// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_FILEIO_H
#define OS_FILEIO_H

#include "os_types.h"

#if defined(TARGET_DESKTOP)
// RAYLIB ======================================================================
typedef FILE OS_FILE;
#define os_fopen  fopen
#define os_fseek  fseek
#define os_ftell  ftell
#define os_fread  fread
#define os_fclose fclose
//
#elif defined(TARGET_PD)
// PLAYDATE ====================================================================
typedef SDFile OS_FILE;

static inline OS_FILE *os_fopen(const char *file, const char *mode)
{
        if (!file || !mode) return NULL;
        switch (mode[0]) {
        case 'r': return PD->file->open(file, kFileRead);
        }
        return NULL;
}
#define os_fseek                   PD->file->seek
#define os_ftell                   PD->file->tell
#define os_fread(PTR, S, NUM, FIL) PD->file->read(FIL, PTR, NUM)
#define os_fclose                  PD->file->close
//
#endif // playdate
// =============================================================================

enum {
        OS_SEEK_END = SEEK_END,
        OS_SEEK_SET = SEEK_SET,
};

// reads a text file into the given buffer and
// returns the length of the string including \0
int   txt_read_file(const char *file, char *buf, size_t bufsize);
char *txt_read_file_alloc(const char *file, void *(*allocfunc)(size_t));

enum {
        JSN_OBJ,
        JSN_ARR,
        JSN_STR,
        JSN_NUM,
        JSN_TRU,
        JSN_FLS,
        JSN_NUL,
};

typedef struct {
        char s[16]; // depth stack indicating key-value
        int  i;
} jsnstack_s;

typedef struct {
        char      *txt;
        int        type;
        int        i;
        int        depth;
        jsnstack_s st;
} jsn_s;

bool32 jsn_root(const char *txt, jsn_s *jr);
bool32 jsn_next(jsn_s j, jsn_s *jn);
bool32 jsn_sibling(jsn_s j, jsn_s *js);
bool32 jsn_fchild(jsn_s j, jsn_s *js);
bool32 jsn_key(jsn_s j, const char *key, jsn_s *jk);
int    jsn_num_children(jsn_s j);
int    jsn_to_fchild(jsn_s *j);
int    jsn_to_sibling(jsn_s *j);
int    jsn_to_key(jsn_s *j, const char *key);
i32    jsn_int(jsn_s j);
i32    jsn_intk(jsn_s j, const char *key);
u32    jsn_uint(jsn_s j);
u32    jsn_uintk(jsn_s j, const char *key);
char  *jsn_strkptr(jsn_s j, const char *key);
char  *jsn_str(jsn_s j, char *buf, size_t bufsize);
char  *jsn_strk(jsn_s j, const char *key, char *buf, size_t bufsize);
void   jsn_print(jsn_s j);

static jsn_s i_jsn_begin(jsn_s jroot)
{
        jsn_s j = {0};
        jsn_fchild(jroot, &j);
        return j;
}

static jsn_s i_jsn_begink(jsn_s jroot, const char *key)
{
        jsn_s k;
        jsn_s j = {0};
        if (jsn_key(jroot, key, &k)) {
                jsn_fchild(k, &j);
        } else {
                j.type = -1;
        }
        return j;
}

#define foreach_jsn_child(JROOT, NAME) \
        for (jsn_s NAME = i_jsn_begin(JROOT); NAME.type >= 0; jsn_sibling(NAME, &NAME))
#define foreach_jsn_childk(JROOT, KEY, NAME) \
        for (jsn_s NAME = i_jsn_begink(JROOT, KEY); NAME.type >= 0; jsn_sibling(NAME, &NAME))

static bool32 c_regex(char c, const char *chars)
{
        for (int i = 0; chars[i] != '\0'; i++) {
                if (chars[i] == c) return 1;
        }
        return 0;
}

#endif