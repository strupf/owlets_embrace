#include "obj.h"
#include "game.h"

bool32 objhandle_is_valid(objhandle_s h)
{
        return (h.o && h.o->gen == h.gen);
}

bool32 objhandle_is_null(objhandle_s h)
{
        return (h.o == NULL);
}

obj_s *obj_from_objhandle(objhandle_s h)
{
        return (h.o && h.o->gen == h.gen ? h.o : NULL);
}

objhandle_s objhandle_from_obj(obj_s *o)
{
        objhandle_s h = {o->gen, o};
        return h;
}

obj_s *obj_create(game_s *g)
{
        NOT_IMPLEMENTED
        ASSERT(g->n_objfree > 0);
        obj_s      *o       = g->objfreestack[--g->n_objfree];
        int         index   = o->index;
        int         gen     = o->gen;
        const obj_s objzero = {0};
        *o                  = objzero;
        o->gen              = gen;
        o->index            = index;
        return o;
}

void obj_delete(game_s *g, obj_s *o)
{
        // test if object is already freed
        for (int n = 0; n < g->n_objfree; n++) {
                if (g->objfreestack[n] == o) {
                        ASSERT(0);
                }
        }
        o->gen++;
        g->objfreestack[g->n_objfree++] = o;
}

rec_i32 obj_aabb(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y, o->w, o->h};
        return r;
}