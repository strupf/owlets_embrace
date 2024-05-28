// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf.h"

typedef struct pltf_s {
    u32 tick;
    f32 lasttime;
    f32 ups_timeacc;
    f32 fps_timeacc;
    i32 fps_counter;
    i32 fps; // updates per second
} pltf_s;

pltf_s g_pltf;

void pltf_internal_init()
{
    g_pltf.fps      = PLTF_UPS;
    g_pltf.lasttime = pltf_seconds();
    app_init();
}

i32 pltf_internal_update()
{
    f32 time        = pltf_seconds();
    f32 timedt      = time - g_pltf.lasttime;
    g_pltf.lasttime = time;
    g_pltf.ups_timeacc += timedt;
    if (PLTF_UPS_DT_CAP < g_pltf.ups_timeacc) {
        g_pltf.ups_timeacc = PLTF_UPS_DT_CAP;
    }

    bool32 updated = 0;
    while (PLTF_UPS_DT_TEST <= g_pltf.ups_timeacc) {
        g_pltf.ups_timeacc -= PLTF_UPS_DT;
        g_pltf.tick++;
        updated = 1;
        app_tick();
    }

    if (updated) {
        app_draw();
        g_pltf.fps_counter++;
    }

    g_pltf.fps_timeacc += timedt;
    if (1.f <= g_pltf.fps_timeacc) {
        g_pltf.fps_timeacc -= 1.f;
        g_pltf.fps         = g_pltf.fps_counter;
        g_pltf.fps_counter = 0;
    }
    return updated;
}

void pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    app_audio(lbuf, rbuf, len);
}

void pltf_internal_close()
{
    app_close();
}

void pltf_internal_pause()
{
    app_pause();
}

void pltf_internal_resume()
{
    app_resume();
}

#if PLTF_ENGINE_ONLY
void app_init()
{
}

void app_tick()
{
}

void app_draw()
{
}

void app_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
}

void app_close()
{
}

void app_pause()
{
}

void app_resume()
{
}

#endif