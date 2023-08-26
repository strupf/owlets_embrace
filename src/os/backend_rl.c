// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "backend.h"

int main()
{
        os_prepare();
        while (!WindowShouldClose()) {
                os_do_tick();
        }

        os_backend_audio_close();
        os_backend_graphics_close();
        return 0;
}

void os_backend_graphics_init()
{
        InitWindow(400 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE, "raylib");
        Image img = GenImageColor(416, 240, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(60);
}

void os_backend_graphics_close()
{
        UnloadTexture(g_os.tex);
        CloseWindow();
}

void os_backend_graphics_begin()
{
        os_memclr4(g_os.framebuffer, sizeof(g_os.framebuffer));
}

void os_backend_graphics_end()
{
        static const Color t_rgb[2] = {0x31, 0x2F, 0x28, 0xFF,
                                       0xB1, 0xAF, 0xA8, 0xFF};

        for (int y = 0; y < 240; y++) {
                for (int x = 0; x < 400; x++) {
                        int i         = (x >> 3) + y * 52;
                        int k         = x + y * 416;
                        int byt       = g_os.framebuffer[i];
                        int bit       = (byt & (0x80 >> (x & 7))) > 0;
                        g_os.texpx[k] = t_rgb[g_os.inverted ? !bit : bit];
                }
        }
        UpdateTexture(g_os.tex, g_os.texpx);
}

void os_backend_graphics_flip()
{
        static const Rectangle rsrc =
            {0, 0, 416, 240};
        static const Rectangle rdst =
            {0, 0, 416 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE};
        static const Vector2 vorg =
            {0, 0};
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(g_os.tex, rsrc, rdst, vorg, 0.f, WHITE);
        EndDrawing();
}

static void audio_callback(void *buf, uint len)
{
        static i16 audiomem_local[0x4000];

        if (os_audio_cb(NULL, audiomem_local, NULL, len)) {
                os_memcpy(buf, audiomem_local, sizeof(i16) * len);
        } else {
                os_memclr(buf, sizeof(i16) * len);
        }
}

void os_backend_audio_init()
{
        InitAudioDevice();

        /*
        snd_s            snd = snd_load_wav("assets/snd/sample.wav");
        audio_channel_s *ch  = &g_os.audiochannels[0];
        ch->playback_type    = PLAYBACK_TYPE_GEN;
        ch->wavedata         = snd.data;
        ch->wavelen          = snd.len;
        ch->gentype          = WAVE_TYPE_SINE;
        ch->genfreq          = 300;
        ch->sinincr          = (ch->genfreq * 0x40000) / 44100;
        ch->squarelen        = 44100 / ch->genfreq;
        */

        mus_play("assets/snd/pink.wav");
        // snd_s pinks = snd_load_wav("assets/snd/pink.wav");
        // snd_play(pinks);
        g_os.audiostream = LoadAudioStream(44100, 16, 1);
        SetAudioStreamCallback(g_os.audiostream, audio_callback);
        PlayAudioStream(g_os.audiostream);
}

void os_backend_audio_close()
{
        UnloadAudioStream(g_os.audiostream);
        mus_close();
        CloseAudioDevice();
}