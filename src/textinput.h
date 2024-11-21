// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TEXTINPUT_H
#define TEXTINPUT_H

#include "pltf/pltf.h"

#define NUM_TEXTINPUT_CHARS 64

typedef struct {
    char (*on_char_add)(char c); // return char - if '\0' returned: not added
    u16  cap;
    u16  n;
    char c[NUM_TEXTINPUT_CHARS];
} textinput_s;

bool32 textinput_active();
void   textinput_activate(textinput_s *txt, bool32 only_letters);
void   textinput_deactivate();
void   textinput_update();
void   textinput_draw();
i32    textinput_draw_offs();

#endif