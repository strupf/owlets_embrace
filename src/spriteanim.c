// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "spriteanim.h"
#include "gamedef.h"

int ase_anim_parse(ase_anim_s *a, const char *file, void *(*allocf)(usize s))
{
    spm_push();
    char *txt;
    if (txt_load(file, spm_alloc, &txt) != TXT_SUCCESS) {
        spm_pop();
        return ASE_ERR_FILE;
    }

    json_s jroot, jframes, jmeta, jarrtags;
    if (json_root(txt, &jroot) != JSON_SUCCESS) {
        spm_pop();
        return ASE_ERR_JSON;
    }

    json_key(jroot, "frames", &jframes);
    json_key(jroot, "meta", &jmeta);
    json_key(jmeta, "frameTags", &jarrtags);

    int n_frames = json_num_children(jframes);
    int n_tags   = json_num_children(jarrtags);
    for (json_each(jmeta, "frameTags", jtag)) {
        char tagname[16];
        jsonk_str(jtag, "name", tagname, sizeof(tagname));
    }

    usize sf   = sizeof(ase_frame_s) * n_frames;
    usize st   = sizeof(ase_tagged_anim_s) * n_tags;
    usize size = sf + st;
    void *mem  = allocf(size);
    if (!mem) {
        spm_pop();
        return ASE_ERR_ALLOC;
    }

    ase_frame_s       *frame = (ase_frame_s *)mem;
    ase_tagged_anim_s *anim  = (ase_tagged_anim_s *)((char *)mem + sf);

    memset(frame, 0, sf);
    memset(anim, 0, st);

    int i_anim       = 0;
    a->frames        = frame;
    a->tagged_anim   = anim;
    a->n_frames      = n_frames;
    a->n_tagged_anim = n_tags;

    for (json_each(jroot, "frames", jframe)) {
        json_s ja, jb, jc;
        json_key(jframe, "sourceSize", &ja);
        json_key(jframe, "spriteSourceSize", &jb);
        json_key(jframe, "frame", &jc);
        frame->duration           = jsonk_u32(jframe, "duration");
        frame->sourcesize.w       = jsonk_u32(ja, "w");
        frame->sourcesize.h       = jsonk_u32(ja, "h");
        frame->frame.x            = jsonk_u32(jc, "x");
        frame->frame.y            = jsonk_u32(jc, "y");
        frame->frame.w            = jsonk_u32(jc, "w");
        frame->frame.h            = jsonk_u32(jc, "h");
        frame->spritesourcesize.x = jsonk_u32(jb, "x");
        frame->spritesourcesize.y = jsonk_u32(jb, "y");
        frame->spritesourcesize.w = jsonk_u32(jb, "w");
        frame->spritesourcesize.h = jsonk_u32(jb, "h");
        frame->rotated            = jsonk_bool(jframe, "rotated");
        frame->trimmed            = jsonk_bool(jframe, "trimmed");
        char *fn                  = jsonk_strp(jframe, "filename", NULL);

        while (*fn != '#') // start of tag name
            fn++;
        fn++;

        char  tag[16];
        char *ct = &tag[0];
        while (*fn != ' ') // splitting token between tag and frame
            *ct++ = *fn++;
        *ct = '\0';

        if (anim->tag[0] == '\0' || !str_eq(anim->tag, tag)) {
            anim = &a->tagged_anim[i_anim++];
            str_cpy(anim->tag, tag);
            anim->frames = frame;
        }

        anim->n_frames = u32_from_str(fn) + 1;
        frame++;
    }
    spm_pop;
    return ASE_SUCCESS;
}

bool32 ase_anim_get_tag(ase_anim_s a, const char *tag, ase_tagged_anim_s *t)
{
    for (int i = 0; i < a.n_tagged_anim; i++) {
        ase_tagged_anim_s u = a.tagged_anim[i];
        if (str_eq(u.tag, tag)) {
            if (t) *t = u;
            return 1;
        }
    }
    return 0;
}

void spriteanim_update(spriteanim_s *a)
{
    if (a->mode == SPRITEANIM_MODE_NONE) return;
    spriteanimdata_s *d = &a->data;
    a->tick++;
    if (a->tick < d->frames[a->frame].ticks) return;

    int f   = a->frame;
    a->tick = 0;
    switch (a->mode) {
    case SPRITEANIM_MODE_LOOP:
        a->frame = (f + 1) % d->n_frames;
        break;
    case SPRITEANIM_MODE_LOOP_REV:
        a->frame = (f - 1 + d->n_frames) % d->n_frames;
        break;
    case SPRITEANIM_MODE_LOOP_PINGPONG:
        break;
    case SPRITEANIM_MODE_ONCE:
        a->frame++;
        if (a->frame == d->n_frames) {
            a->frame = d->n_frames - 1;
            a->mode  = SPRITEANIM_MODE_NONE;
            if (a->cb_finished) {
                a->cb_finished(a, a->cb_arg);
            }
        }
        break;
    case SPRITEANIM_MODE_ONCE_REV:
        a->frame--;
        if (a->frame == -1) {
            a->frame = 0;
            a->mode  = SPRITEANIM_MODE_NONE;
            if (a->cb_finished) {
                a->cb_finished(a, a->cb_arg);
            }
        }
        break;
    }

    if (a->frame != f && a->cb_framechanged) {
        a->cb_framechanged(a, a->cb_arg);
    }
}

texrec_s spriteanim_frame(spriteanim_s *a)
{
    spriteanimdata_s *d = &a->data;
    spriteanimframe_s f = d->frames[a->frame];
    texrec_s          t = {0};
    t.t                 = d->tex;
    t.r.x               = f.x;
    t.r.y               = f.y;
    t.r.w               = d->w;
    t.r.h               = d->h;
    return t;
}