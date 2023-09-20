// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "draw.h"

void sprite_anim_update(sprite_anim_s *a)
{
        if (++a->time < a->times[a->frame]) return;
        a->time = 0;
        switch (a->mode) {
        case SPRITE_ANIM_MODE_ONCE: {
                int f = a->frame + a->dir;
                if (0 <= f && f < a->nframes) {
                        a->frame = f;
                }
        } break;
        case SPRITE_ANIM_MODE_LOOP: {
                a->frame = (a->frame + a->dir + a->nframes) % a->nframes;
        } break;
        case SPRITE_ANIM_MODE_PINGPONG: {
                int f = a->frame + a->dir;
                if (0 <= f && f < a->nframes) {
                        a->frame = f;
                } else {
                        a->frame -= a->dir;
                        a->dir = -a->dir;
                }
        } break;
        case SPRITE_ANIM_MODE_RNG: {
                int f    = rng_fast_u16() % a->nframes;
                a->frame = f == a->frame ? rng_fast_u16() % a->nframes : f;
        } break;
        }
}

void sprite_anim_set(sprite_anim_s *a, int frame, int time)
{
        a->frame = frame;
        a->time  = time;
}

/*
texregion_s sprite_anim_get(sprite_anim_s *a)
{
        texregion_s t = {a->tex,
                         a->sx + a->frame * a->dx * a->sw,
                         a->sy + a->frame * a->dy * a->sh,
                         a->sw,
                         a->sh};
        return t;
}
*/