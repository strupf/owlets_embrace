// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#ifdef OS_PLAYDATE

static LCDBitmap *menubm;

void menufunction(void *arg)
{
        PRINTF("print\n");
}
#endif

void game_init(game_s *g)
{
#ifdef OS_PLAYDATE
        menubm = PD->graphics->newBitmap(400, 240, kColorWhite);
        PD->system->addMenuItem("Option", menufunction, NULL);
        PD->system->setMenuImage(menubm, 0);
#endif
        gfx_set_inverted(1);

        for (int n = 0; n < ARRLEN(g_tileIDs); n++) {
                g_tileIDs[n] = n;
        }

        assets_load();
        world_def_init();

        g->mainmenu.state       = MAINMENU_STATE_TITLE;
        g->mainmenu.num_options = 1;
        g->state                = GAMESTATE_TITLE;
        g->rng                  = 213;
        g->cam.w                = 400;
        g->cam.h                = 240;
        g->cam.wh               = g->cam.w / 2;
        g->cam.hh               = g->cam.h / 2;

        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ALIVE];
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ACTOR];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_ACTOR;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_SOLID];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_SOLID;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_NEW_AREA_COLLIDER];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_NEW_AREA_COLLIDER;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_PICKUP];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_PICKUP;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_INTERACT];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_INTERACT;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_MOVABLE];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_MOVABLE;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_THINK_1];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_THINK_1;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_THINK_2];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_THINK_2;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_HURTABLE];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_HURTABLE;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_KILL_OFFSCREEN];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_KILL_OFFSCREEN;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_HURTS_PLAYER];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_HURTS_PLAYER;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_CAM_ATTRACTOR];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_CAM_ATTRACTOR;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_RENDERABLE];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_RENDERABLE;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ANIMATE];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_ANIMATE;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ENEMY];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_ENEMY;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_HURTS_ENEMIES];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_HURTS_ENEMIES;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_HITBOX];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = OBJ_FLAG_HITBOX;
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
}