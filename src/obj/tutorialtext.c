// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void tutorialtext_on_animate(g_s *g, obj_s *o);
void tutorialtext_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void tutorialtext_load(g_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJID_TUTORIALTEXT;
    o->on_draw         = tutorialtext_on_draw;
    o->w               = 32;
    o->h               = 32;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->state           = map_obj_i32(mo, "I");
    o->render_priority = RENDER_PRIO_BEHIND_TERRAIN_LAYER;
    o->on_animate      = tutorialtext_on_animate;
}

void tutorialtext_on_animate(g_s *g, obj_s *o)
{
    o->timer++;
}

void tutorialtext_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    fnt_s     f   = asset_fnt(FNTID_MEDIUM);
    v2_i32    p   = v2_i32_add(o->pos, cam);
    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  trb = asset_texrec(TEXID_BUTTONS, 0, 0, 32, 32);

    switch (o->state) {
    default:
    case 1: {
        i32 tt = (o->timer >> 5);
        i32 t  = (tt % 3);
        trb.w  = 48;
        trb.h  = 48;

        if (t == 0) {
            trb.x = 1 * 48;
            trb.y = 1 * 48;
        } else {
            switch ((tt / 3) % 8) {
            case 0: trb.x = 0 * 48, trb.y = 0 * 48; break;
            case 1: trb.x = 1 * 48, trb.y = 0 * 48; break;
            case 2: trb.x = 2 * 48, trb.y = 0 * 48; break;
            case 3: trb.x = 2 * 48, trb.y = 1 * 48; break;
            case 4: trb.x = 2 * 48, trb.y = 2 * 48; break;
            case 5: trb.x = 1 * 48, trb.y = 2 * 48; break;
            case 6: trb.x = 0 * 48, trb.y = 2 * 48; break;
            case 7: trb.x = 0 * 48, trb.y = 1 * 48; break;
            }
        }

        gfx_spr(ctx, trb, p, 0, 0);

        p.x += 40;
        p.y += 4;
        trb.w = 32;
        trb.h = 32;
        trb.x = (5 + (2 <= t)) * 32;
        trb.y = (0 * 32);

        gfx_spr(ctx, trb, p, 0, 0);

        v2_i32 ptext = {p.x, p.y + 36};
        fnt_draw_outline_style(ctx, f, ptext, "to throw the ", 1, 1);
        ptext.y += 22;
        fnt_draw_outline_style(ctx, f, ptext, "grappling hook", 1, 1);

        break;
    }
    case 2: {
        i32 t        = ((o->timer >> 6) & 1);
        trb.x        = (5 + t) * 32;
        trb.y        = 1 * 32;
        v2_i32 ptext = {p.x + 16, p.y + 36};
        fnt_draw_outline_style(ctx, f, ptext, "to air jump", 1, 1);
        ptext.y += 22;
        fnt_draw_outline_style(ctx, f, ptext, "(multiple times)", 1, 1);
        gfx_spr(ctx, trb, p, 0, 0);
        break;
    }
    case 3: {
        i32 tt = (o->timer >> 5);
        i32 t  = (tt % 3);
        trb.w  = 48;
        trb.h  = 48;
        trb.x  = 1 * 48;
        trb.y  = 1 * 48;
        gfx_spr(ctx, trb, p, 0, 0);

        p.x += 40;
        p.y += 4;
        trb.w = 32;
        trb.h = 32;
        trb.x = (5 + (1 <= t)) * 32;
        trb.y = (0 * 32);
        gfx_spr(ctx, trb, p, 0, 0);

        v2_i32 ptext = {p.x, p.y + 36};
        fnt_draw_outline_style(ctx, f, ptext, "Hold B to swap between", 1, 1);
        ptext.y += 22;
        fnt_draw_outline_style(ctx, f, ptext, "attack & grappling hook", 1, 1);

        break;
    }
    case 4: {
        i32 t        = ((o->timer >> 6) & 1);
        trb.x        = (5 + t) * 32;
        trb.y        = 0 * 32;
        v2_i32 ptext = {p.x + 16, p.y + 36};
        fnt_draw_outline_style(ctx, f, ptext, "to attack", 1, 1);
        gfx_spr(ctx, trb, p, 0, 0);
        break;
    }
    }
}