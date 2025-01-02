// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 textlen;
    u8  text[128];
} sign_popup_s;

obj_s *sign_popup_create(g_s *g)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJ_ID_SIGN_POPUP;
    o->flags           = OBJ_FLAG_SPRITE;
    o->render_priority = 1;
    o->n_sprites       = 1;
    obj_sprite_s *spr  = &o->sprites[0];
    spr->trec          = asset_texrec(TEXID_MISCOBJ, 64, 0, 32, 32);
    spr->offs.x        = -8;
    spr->offs.y        = -8;
    o->w               = 16;
    o->h               = 16;

    return o;
}

void sign_popup_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = sign_popup_create(g);
}

void sign_popup_on_update(g_s *g, obj_s *o)
{
    sign_popup_s *s     = (sign_popup_s *)o->mem;
    obj_s        *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        v2_i32 p1 = obj_pos_center(ohero);
        v2_i32 p2 = obj_pos_center(o);
        u32    dt = v2_distancesq(p1, p2);
        if (dt < 1000) {
            o->timer++;
            return;
        }
    }
    o->timer--;
}

void sign_popup_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    if (o->timer <= 0) return;

    sign_popup_s *s   = (sign_popup_s *)o->mem;
    int           t   = clamp_i32(o->timer, 0, 100);
    gfx_ctx_s     ctx = gfx_ctx_display();
    ctx.pat           = gfx_pattern_interpolate(t, 100);
    fnt_s fnt         = asset_fnt(FNTID_SMALL);

    v2_i32 pos = {0};
    fnt_draw_ascii(ctx, fnt, v2_add(pos, cam), NULL, 0);
}

void sign_on_interact(g_s *g, obj_s *o)
{
    // textbox_load_dialog(g, o->filename);
}

// interactable sign

obj_s *sign_create(g_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SIGN;
    o->flags = OBJ_FLAG_INTERACTABLE |
               OBJ_FLAG_SPRITE;
    o->on_interact     = sign_on_interact;
    o->render_priority = -10;
    o->n_sprites       = 1;
    obj_sprite_s *spr  = &o->sprites[0];
    spr->trec          = asset_texrec(TEXID_MISCOBJ, 48, 0, 32, 32);
    spr->offs.x        = -8;
    spr->offs.y        = -16;

    o->w = 16;
    o->h = 16;
    return o;
}

void sign_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = sign_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    map_obj_strs(mo, "Dialogfile", o->filename);
}
