// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_fileio.h"

char *txt_read_file_alloc(const char *file, void *(*allocfunc)(size_t))
{
        ASSERT(file);
        OS_FILE *f = os_fopen(file, "r");
        ASSERT(f);
        int e = os_fseek(f, 0, OS_SEEK_END);
        ASSERT(e == 0);
        int size  = (int)os_ftell(f);
        e         = os_fseek(f, 0, OS_SEEK_SET);
        char *buf = allocfunc((size_t)size + 2);
        ASSERT(e == 0 && buf);
        int read = (int)os_fread(buf, 1, size, f);
        e        = os_fclose(f);
        ASSERT(e == 0);
        buf[read] = '\0';
        PRINTF("Read file: %s\n", file);
        return buf;
}

int txt_read_file(const char *file, char *buf, size_t bufsize)
{
        ASSERT(file && buf && bufsize);
        OS_FILE *f = os_fopen(file, "r");
        ASSERT(f);
        int e = os_fseek(f, 0, OS_SEEK_END);
        ASSERT(e == 0);
        int size = (int)os_ftell(f);
        e        = os_fseek(f, 0, OS_SEEK_SET);
        ASSERT(e == 0 && 0 < size && size + 1 < (int)bufsize);
        int read = (int)os_fread(buf, 1, size, f);
        e        = os_fclose(f);
        ASSERT(e == 0);
        buf[read] = '\0';
        PRINTF("Read file: %s\n", file);
        return read;
}

bool32 jsn_root(const char *txt, jsn_s *jr)
{
        ASSERT(txt && jr);
        jsn_s j = {0};
        j.txt   = (char *)txt;
        j.st.i  = 1;
        int n   = 0;
        for (char c = txt[0]; c != '\0'; c = txt[++n]) {
                switch (c) {
                case '{':
                        j.i    = n;
                        j.type = JSN_OBJ;
                        *jr    = j;
                        return 1;
                }
        }
        return 0;
}

// at: position of first "
// skips a string and returns the location of the first character
// after ending quotation marks
static int jsn_skip_str(const char *txt, int at)
{
        ASSERT(txt[at] == '\"');
        int  n = at;
        char c = txt[n];
        while (c != '\0') {
                c = txt[++n];
                switch (c) {
                case '\\': c = txt[++n]; break;
                case '\"': return n + 1;
                }
        }
        return -1;
}

// skips through a number and returns the index of
// the next non-number token AFTER the number
static int jsn_skip_num(const char *txt, int at)
{
        int  n = at;
        char c = txt[n];
        int  s = c == '-' ? 0 : 2;
        while (1) {
                c = txt[++n];
                switch (s) {
                case 0:
                        s = char_digit_1_9(c) ? 2 : 1;
                        break;
                case 1:
                        if (c == '.') {
                                s = 3, n++;
                                break;
                        }

                        if (c_regex(c, "eE")) {
                                s = 4, n++;
                                break;
                        }
                        return n;
                case 2:
                        if (char_digit(c)) break;
                        if (c == '.') {
                                s = 3, n++;
                                break;
                        }
                        if (c_regex(c, "eE")) {
                                s = 4, n++;
                                break;
                        }
                        return n;
                case 3:
                        if (char_digit(c)) break;
                        if (c_regex(c, "eE")) {
                                s = 4, n++;
                                break;
                        }
                        return n;
                case 4:
                        if (char_digit(c)) break;
                        return n;
                }
        }
        ASSERT(0);
        return -1;
}

int jsn_typec(char c)
{
        if (char_digit(c)) return JSN_NUM;
        switch (c) {
        case '\"': return JSN_STR;
        case '{': return JSN_OBJ;
        case '[': return JSN_ARR;
        case 'f': return JSN_FLS;
        case 't': return JSN_TRU;
        case 'n': return JSN_NUL;
        case '+':
        case '-': return JSN_NUM;
        }
        ASSERT(0);
        return -1;
}

int jsn_typej(jsn_s j)
{
        return jsn_typec(j.txt[j.i]);
}

bool32 jsn_next(jsn_s j, jsn_s *jn)
{
        char      *txt = j.txt;
        int        i   = j.i;
        jsnstack_s st  = j.st;

        // immediately reduce depth if j is not an collection type
        // if (i == 0) return 0;
        // ASSERT(st.i > 0);
        if (i > 0 && st.s[st.i - 1] == 1) st.i--;

        switch (j.type) {
        case JSN_OBJ: i++; break;
        case JSN_ARR: i++; break;
        case JSN_NUM: i = jsn_skip_num(txt, i); break;
        case JSN_STR: i = jsn_skip_str(txt, i); break;
        case JSN_FLS: i += 5; break;
        case JSN_TRU: i += 4; break;
        case JSN_NUL: i += 4; break;
        }

        for (char c = txt[i]; 1; c = txt[++i]) {
                switch (c) {
                case '\0': return 0;
                case ',':
                case ' ':
                case '\t':
                case '\n':
                case '\r': break;
                case '}':
                case ']':
                        st.i--;
                        if (st.i == 0) return 0;
                        if (st.s[st.i - 1] == 1) {
                                st.i--;
                        }

                        break;
                case ':':
                        ASSERT(j.type == JSN_STR);
                        st.s[st.i++] = 1;

                        for (c = txt[++i]; c != '\0'; c = txt[++i]) {
                                switch (c) {
                                case '\0': return 0;
                                case ' ':
                                case '\t':
                                case '\n':
                                case '\r': break;
                                default: goto NESTEDBREAK;
                                }
                        }
                        break;
                default: goto NESTEDBREAK;
                }
        }

NESTEDBREAK:

        if (jn) {
                int tp    = jsn_typec(txt[i]);
                jn->st    = st;
                jn->i     = i;
                jn->depth = st.i;
                jn->txt   = txt;
                jn->type  = tp;
                if (tp <= 1) {
                        jn->st.s[jn->st.i++] = 0;
                }
        }

        return 1;
}

bool32 jsn_sibling(jsn_s j, jsn_s *js)
{
        jsn_s jj = j;
        js->type = -1;
        while (jsn_next(jj, &jj)) {
                if (jj.depth < j.depth) return 0;
                if (jj.depth == j.depth) {
                        *js = jj;
                        return 1;
                }
        }
        return 0;
}

bool32 jsn_fchild(jsn_s j, jsn_s *js)
{
        jsn_s jj = j;
        js->type = -1;
        while (jsn_next(jj, &jj)) {
                if (jj.depth <= j.depth) return 0;
                if (jj.depth > j.depth) {
                        *js = jj;
                        return 1;
                }
        }
        return 0;
}

bool32 jsn_key(jsn_s j, const char *key, jsn_s *jk)
{
        jsn_s jj = j;
        jsn_s jc;
        if (!jsn_fchild(j, &jj)) {
                PRINTF("no fchild\n");
                return 0;
        }

        do {
                if (jsn_typej(jj) != JSN_STR || !jsn_fchild(jj, &jc))
                        continue;
                int n = 0;
                for (char c = key[0]; c != '\0'; c = key[++n]) {
                        if (c != jj.txt[jj.i + n + 1]) {
                                goto CONTINUELOOP;
                        }
                }
                *jk = jc;
                return 1;
        CONTINUELOOP:;
        } while (jsn_sibling(jj, &jj));

        PRINTF("jsn: key not found - %s\n", key);
        return 0;
}

int jsn_num_children(jsn_s j)
{
        jsn_s c;
        if (!jsn_fchild(j, &c)) return 0;
        int n = 0;
        do {
                n++;
        } while (jsn_sibling(c, &c));
        return n;
}

int jsn_to_fchild(jsn_s *j)
{
        return jsn_fchild(*j, j);
}

int jsn_to_sibling(jsn_s *j)
{
        return jsn_sibling(*j, j);
}

int jsn_to_key(jsn_s *j, const char *key)
{
        return jsn_key(*j, key, j);
}

void jsn_print(jsn_s j)
{
        int i0 = j.i;
        int i1;
        int type = jsn_typej(j);
        if (type == -1) return;
        for (int n = 0; n < j.depth; n++) {
                PRINTF("  ");
        }
        switch (type) {
        case JSN_OBJ: PRINTF("{\n"); return;
        case JSN_ARR: PRINTF("[\n"); return;
        case JSN_NUM: i1 = jsn_skip_num(j.txt, i0); break;
        case JSN_STR: i1 = jsn_skip_str(j.txt, i0); break;
        case JSN_FLS: i1 = i0 + 5; break;
        case JSN_TRU: i1 = i0 + 4; break;
        case JSN_NUL: i1 = i0 + 4; break;
        default: ASSERT(0);
        }
        for (int n = i0; n < i1; n++) {
                PRINTF("%c", j.txt[n]);
        }
        PRINTF("\n");
}

i32 jsn_int(jsn_s j)
{
        int i1  = jsn_skip_num(j.txt, j.i);
        int n   = 0;
        i32 res = 0;
        for (int i = j.i; i < i1; i++, n++) {
                res *= 10;
                res += char_hex_to_int(j.txt[i]);
        }
        return res;
}

i32 jsn_intk(jsn_s j, const char *key)
{
        jsn_s jj;
        if (!jsn_key(j, key, &jj)) return 0;
        return jsn_int(jj);
}

u32 jsn_uint(jsn_s j)
{
        int i1  = jsn_skip_num(j.txt, j.i);
        int n   = 0;
        u32 res = 0;
        for (int i = j.i; i < i1; i++, n++) {
                res *= 10;
                res += char_hex_to_int(j.txt[i]);
        }
        return res;
}

u32 jsn_uintk(jsn_s j, const char *key)
{
        jsn_s jj;
        if (!jsn_key(j, key, &jj)) return 0;
        return jsn_int(jj);
}

char *jsn_strkptr(jsn_s j, const char *key)
{
        jsn_s jj;
        if (!jsn_key(j, key, &jj)) return NULL;
        ASSERT(jj.type == JSN_STR);
        return &j.txt[jj.i + 1];
}

char *jsn_str(jsn_s j, char *buf, size_t bufsize)
{
        ASSERT(j.type == JSN_STR);
        int i0 = j.i + 1;
        int i1 = jsn_skip_str(j.txt, j.i) - 1;
        int n  = 0;
        for (int i = i0; i < i1 && n < (int)bufsize; i++, n++) {
                buf[n] = j.txt[i];
        }
        buf[n] = '\0';
        return buf;
}

char *jsn_strk(jsn_s j, const char *key, char *buf, size_t bufsize)
{
        jsn_s jj;
        if (!jsn_key(j, key, &jj)) return 0;
        return jsn_str(jj, buf, bufsize);
}