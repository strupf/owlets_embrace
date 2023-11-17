// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SYS_BACKEND_H
#define SYS_BACKEND_H

#include "sys_types.h"

void  sys_init();
int   sys_tick(void *arg);
int   sys_audio_cb(void *context, i16 *lbuf, i16 *rbuf, int len);
void  sys_close();
void  sys_pause();
void  sys_resume();
//
void  backend_display_row_updated(int a, int b);
f32   backend_seconds();
u8   *backend_framebuffer();
int   backend_inp();
int   backend_debug_space();
f32   backend_crank();
int   backend_crank_docked();
void *backend_file_open(const char *path, int mode);
int   backend_file_close(void *f);
int   backend_file_read(void *f, void *buf, usize bufsize);
int   backend_file_write(void *f, const void *buf, usize bufsize);
int   backend_file_tell(void *f);
int   backend_file_seek(void *f, int pos, int origin);
int   backend_file_remove(const char *path);
#endif