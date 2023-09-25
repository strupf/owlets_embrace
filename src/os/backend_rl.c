// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

#define RL_1080P 0 // used for capturing trailer footage
#define RL_SCALE 1

int main()
{
        os_prepare();
        while (!WindowShouldClose()) {
                os_do_tick();
        }

        os_backend_close();
        return 0;
}

static void audio_callback(void *buf, uint len);

void os_backend_init()
{
#if RL_1080P
        InitWindow(1920, 1080, "raylib");
#else
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(400 * RL_SCALE, 240 * RL_SCALE, "raylib");
#endif

        Image img = GenImageColor(416, 240, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(120);

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

        g_os.audiostream = LoadAudioStream(44100, 16, 1);
        SetAudioStreamCallback(g_os.audiostream, audio_callback);
        PlayAudioStream(g_os.audiostream);
}

void os_backend_close()
{
        mus_close();
        UnloadAudioStream(g_os.audiostream);
        CloseAudioDevice();
        UnloadTexture(g_os.tex);
        CloseWindow();
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
        BeginDrawing();
        ClearBackground(BLACK);
#if RL_1080P
        Rectangle rdst = {60, 0, 1800, 1080};
#else
        int       dstw = 400 * RL_SCALE;
        int       dsth = 240 * RL_SCALE;
        Rectangle rdst = {(float)((GetScreenWidth() - dstw) / 2),
                          (float)((GetScreenHeight() - dsth) / 2),
                          (float)dstw, (float)dsth};
#endif
        DrawTexturePro(g_os.tex, (Rectangle){0.f, 0.f, 400.f, 240.f}, rdst, (Vector2){0}, 0.f, WHITE);
        EndDrawing();
}

void os_backend_inp_update()
{
        g_os.buttonsp = g_os.buttons;
        g_os.buttons  = 0;

        if (IsKeyDown(KEY_A)) g_os.buttons |= INP_LEFT;
        if (IsKeyDown(KEY_D)) g_os.buttons |= INP_RIGHT;
        if (IsKeyDown(KEY_W)) g_os.buttons |= INP_UP;
        if (IsKeyDown(KEY_S)) g_os.buttons |= INP_DOWN;
        if (IsKeyDown(KEY_COMMA)) g_os.buttons |= INP_B;
        if (IsKeyDown(KEY_PERIOD)) g_os.buttons |= INP_A;

        /* only for testing purposes because emulating Playdate buttons
         * on a keyboard is weird */
        if (IsGamepadAvailable(0)) {
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -.1f)
                        g_os.buttons |= INP_LEFT;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > +.1f)
                        g_os.buttons |= INP_RIGHT;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) < -.1f)
                        g_os.buttons |= INP_UP;
                if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) > +.1f)
                        g_os.buttons |= INP_DOWN;
                if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                        g_os.buttons |= INP_B;
                if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                        g_os.buttons |= INP_A;
        }

        g_os.crankdockedp = g_os.crankdocked;
        g_os.crankdocked  = 0;
        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                if (GetMouseWheelMove() != 0.f) {
                        g_os.crank_q16 += (int)(5000.f * GetMouseWheelMove());
                } else {
                        if (IsKeyDown(KEY_E)) g_os.crank_q16 += 2048;
                        if (IsKeyDown(KEY_Q)) g_os.crank_q16 -= 2048;
                }

                g_os.crank_q16 &= 0xFFFF;
        }
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

bool32 debug_inp_up() { return IsKeyDown(KEY_UP); }
bool32 debug_inp_down() { return IsKeyDown(KEY_DOWN); }
bool32 debug_inp_left() { return IsKeyDown(KEY_LEFT); }
bool32 debug_inp_right() { return IsKeyDown(KEY_RIGHT); }
bool32 debug_inp_w() { return IsKeyDown(KEY_W); }
bool32 debug_inp_a() { return IsKeyDown(KEY_A); }
bool32 debug_inp_s() { return IsKeyDown(KEY_S); }
bool32 debug_inp_d() { return IsKeyDown(KEY_D); }
bool32 debug_inp_enter() { return IsKeyDown(KEY_ENTER); }
bool32 debug_inp_space() { return IsKeyDown(KEY_SPACE); }
