// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STEERING_H
#define STEERING_H

#include "gamedef.h"

v2_i32 steer_seek(v2_i32 p,
                  v2_i32 v,
                  v2_i32 p_trg,
                  i32    vmax);
v2_i32 steer_flee(v2_i32 p,
                  v2_i32 v,
                  v2_i32 p_trg,
                  i32    vmax);
v2_i32 steer_arrival(v2_i32 p,
                     v2_i32 v,
                     v2_i32 p_trg,
                     i32    vmax,
                     i32    radius);

#endif