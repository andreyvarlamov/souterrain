#include "sav_lib.h"

#include "va_util.h"
#include "va_types.h"

#include <sdl2/SDL_scancode.h>

#include <cstdio>
#include <cstdlib>

#include "sou_templates.h"

void UpdateAndRender(b32 *Quit, b32 Reloaded, game_memory GameMemory);

int main(int argc, char **argv)
{
    InitWindow("Souterrain", 1920, 1080);

    InitAudio();

    game_memory GameMemory = AllocGameMemory(Megabytes(4));

    SetTargetFPS(60.0);

    b32 ShouldQuit = false;
    while (!ShouldQuit)
    {
        BeginFrameTiming();

        PollEvents(&ShouldQuit);

        UpdateAndRender(&ShouldQuit, false, GameMemory);

        EndFrameTiming();
    }

    Quit();

    return 0;
}
