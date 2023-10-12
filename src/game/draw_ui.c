// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "draw.h"
#include "game.h"

static void draw_textbox(textbox_s *tb, v2_i32 camp);

static void item_selection_redraw(hero_s *h)
{
        // interpolator based on crank position
        int ii = -((ITEM_SIZE * os_inp_crank()) >> 16);
        if (os_inp_crank() >= 0x8000) {
                ii += ITEM_SIZE;
        }

        int itemIDs[3] = {h->selected_item_prev,
                          h->selected_item,
                          h->selected_item_next};

        tex_s         texcache = tex_get(TEXID_ITEM_SELECT_CACHE);
        gfx_context_s ctx      = gfx_context_create(texcache);
        ctx.src                = tex_get(TEXID_ITEMS);
        gfx_tex_clr(texcache);

        for (int y = -ITEM_BARREL_R; y <= +ITEM_BARREL_R; y++) {
                int     a_q16   = (y << 16) / ITEM_BARREL_R;
                int     arccos  = (acos_q16(a_q16) * ITEM_SIZE) >> (16 + 1);
                int     loc     = arccos + ITEM_SIZE - ii;
                int     itemi   = loc / ITEM_SIZE;
                int     yy      = ITEM_SIZE * itemIDs[itemi] + loc % ITEM_SIZE;
                int     uu      = ITEM_BARREL_R - y + ITEM_Y_OFFS;
                rec_i32 itemrow = {ITEM_SIZE, yy, ITEM_SIZE, 1};
                gfx_sprite(ctx, (v2_i32){ITEM_X_OFFS, uu}, itemrow, 0);
        }

        gfx_sprite(ctx, (v2_i32){0, 0}, (rec_i32){64, 0, 64, 64}, 0);
}

void draw_UI(game_s *g, v2_i32 camp)
{
        gfx_context_s ctx = gfx_context_create(tex_get(0));
        ctx.col           = 1;
        hero_s *h         = (hero_s *)obj_get_tagged(g, OBJ_TAG_HERO);

        if (h->aquired_items > 0) {
                if (h->itemselection_dirty) {
                        item_selection_redraw(h);
                        h->itemselection_dirty = 0;
                }
                ctx.src = tex_get(TEXID_ITEM_SELECT_CACHE);
                gfx_sprite(ctx,
                           (v2_i32){400 - ITEM_FRAME_SIZE + 16,
                                    -16},
                           (rec_i32){0, 0,
                                     ITEM_FRAME_SIZE,
                                     ITEM_FRAME_SIZE},
                           SPRITE_CPY);
        }

        obj_s *ohero = (obj_s *)h;
        if (h && h->caninteract) {
                v2_i32 heroc = obj_aabb_center(ohero);

                obj_s *interactable = obj_closest_interactable(g, heroc);

                if (interactable) {
                        int    type = interactable->interactable_type;
                        v2_i32 pp   = obj_aabb_center(interactable);

                        switch (type) {
                        case INTERACTABLE_TYPE_READ:
                        case INTERACTABLE_TYPE_SPEAK:
                        case INTERACTABLE_TYPE_DEFAULT: {
                                pp = v2_add(pp, camp);
                                pp.y -= 48;
                                pp.x -= 8;
                                ctx.src = tex_get(TEXID_UI);

                                int yy = (os_tick() / 30) % 2;
                                gfx_sprite(ctx, pp, (rec_i32){32 * 2, yy * 32, 32, 32}, 0);
                        } break;
                        }
                }
        }

        if (textbox_state(&g->textbox) != TEXTBOX_STATE_INACTIVE)
                draw_textbox(&g->textbox, camp);

        if (g->area_name_ticks > 0) {
                fnt_s areafont  = fnt_get(FNTID_DEFAULT);
                int   fadeticks = min_i(g->area_name_ticks, AREA_NAME_FADE_TICKS);
                int   patternID = lerp_fast_i32(GFX_PATTERN_0,
                                                GFX_PATTERN_100,
                                                fadeticks,
                                                AREA_NAME_FADE_TICKS);
                ctx.pat         = gfx_pattern_get(patternID);
                gfx_text_ascii(ctx, &areafont, g->area_name, (v2_i32){10, 10});
                ctx.pat = gfx_pattern_get(GFX_PATTERN_100);
        }
}

static void draw_textbox(textbox_s *tb, v2_i32 camp)
{
        gfx_context_s ctx   = gfx_context_create(tex_get(0));
        int           state = textbox_state(tb);
        switch (state) {
        case TEXTBOX_STATE_OPENING: {
                int patternID = lerp_i32(GFX_PATTERN_0,
                                         GFX_PATTERN_100,
                                         tb->animationticks,
                                         TEXTBOX_ANIMATION_TICKS);
                ctx.pat       = gfx_pattern_get(patternID);
        } break;
        case TEXTBOX_STATE_CLOSING: {
                int patternID = lerp_i32(GFX_PATTERN_100,
                                         GFX_PATTERN_0,
                                         tb->animationticks,
                                         TEXTBOX_ANIMATION_TICKS);
                ctx.pat       = gfx_pattern_get(patternID);
        } break;
        }

        fnt_s font  = fnt_get(FNTID_DEFAULT);
        int   textx = 0;
        int   texty = 0;

        // TODO
        tb->type = 0;

        switch (tb->type) {
        case TEXTBOX_TYPE_STATIC_BOX: {
                rec_i32 r = (rec_i32){0, 0, 400, 240};
                ctx.src   = tex_get(TEXID_TEXTBOX);
                gfx_sprite(ctx, (v2_i32){0, 0}, r, 0);
                textx = 20;
                texty = 165;
        } break;
        case TEXTBOX_TYPE_SPEECH_BUBBLE: {
                textx     = 50;
                texty     = 10;
                ctx.col   = 0;
                rec_i32 r = (rec_i32){0, 240, 400, 240};
                ctx.src   = tex_get(TEXID_TEXTBOX);
                gfx_sprite(ctx, (v2_i32){0, 0}, r, 0);
                ctx.col = 1;
                v2_i32 tr0;
                v2_i32 tr1;
                v2_i32 tr2;
                gfx_tri_fill(ctx, (v2_i32){10, 10}, (v2_i32){40, 0}, (v2_i32){0, 50});
        } break;
        }
        ctx.col = 1;

        for (int l = 0; l < TEXTBOX_LINES; l++) {
                textboxline_s *line = &tb->lines[l];
                gfx_text_glyphs(ctx, &font, line->chars, line->n_shown, (v2_i32){textx, texty + 21 * l});
        }

        if (state != TEXTBOX_STATE_WAITING) return;

        ctx.col = 1;
        if (tb->n_choices) {
                rec_i32 rr               = (rec_i32){0, 128, 200, 128};
                ctx.src                  = tex_get(TEXID_TEXTBOX);
                // gfx_sprite(ctx, (v2_i32){243, 48}, rr, 0);
                //  draw choices if any
                static const int spacing = 21;
                for (int n = 0; n < tb->n_choices; n++) {
                        textboxchoice_s *tc = &tb->choices[n];

                        gfx_text_glyphs(ctx, &font, tc->label, tc->labellen,
                                        (v2_i32){264, 70 + spacing * n});
                }

                gfx_rec_fill(ctx, (rec_i32){250, 73 + spacing * tb->cur_choice, 10, 10});
        } else {
                //  "next page" animated marker in corner
                int animstate = (os_tick() * 10000) & (Q16_ANGLE_TURN - 1);
                int yy        = sin_q16(animstate) >> 14;
                gfx_rec_fill(ctx, (rec_i32){370, 205 + yy, 10, 10});
        }
}
