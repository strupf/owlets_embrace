#include "os.h"
#include <stdio.h>

int main()
{
        InitWindow(400, 240, "rl");
        InitAudioDevice();
        while (!WindowShouldClose()) {
                int z = 1;
                printf("Hello world %i\n", z);
        }

        CloseAudioDevice();
        CloseWindow();
        return 1;
}