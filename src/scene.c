#include "scene.h"
#include "gamedef.h"

scene_s scene_create(void (*tick)(void *arg), void (*draw)(void *arg), void *arg)
{
    scene_s s = {0};
    s.tick    = tick;
    s.draw    = draw;
    s.arg     = arg;
    return s;
}

void scene_push(scene_stack_s *st, scene_s s, int fade_in_ticks)
{
    st->scenes[st->n++] = s;
    st->fade            = 0;
    st->fade_in         = fade_in_ticks;
}

void scene_pop(scene_stack_s *st, int fade_out_ticks)
{
    assert(0 < st->n);
    st->n--;
}

void scene_transition(scene_stack_s *st, scene_s s, void (*cb)(void *arg), void *arg)
{
    st->scenes[st->n - 1] = s;
    if (cb) {
        cb(arg);
    }
}

void scene_update(scene_stack_s *st)
{
    if (st->fade_in) {
        st->fade++;
        if (st->fade_in <= st->fade) {
            st->fade_in = 0;
            st->fade    = 0;
        }
    }

    for (int n = st->n - 1; 0 <= n; n--) {
        scene_s s = st->scenes[n];
        s.tick(s.arg);
    }
}

void scene_draw(scene_stack_s *st)
{
    tex_s dst = {0};

    for (int n = 0; n < st->n; n++) {
        scene_s s = st->scenes[n];
#if 0 // fill
        if (s.fillpat) {


            u32 *px = (u32 *)dst.px;
            for (int y = 0; y < SYS_DISPLAY_H; y++) {
                u32 p = s.pat[y & 3];
                for (int x = 0; x < SYS_DISPLAY_WWORDS; x++) {
                    *px |= p;
                    px++;
                }
            }
        }
#endif
        s.draw(s.arg);
    }
}
