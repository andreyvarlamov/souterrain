#include "sav_lib.h"
#include "souterrain.h"

#include "va_util.h"
#include "va_types.h"
#include "va_memarena.h"
#include "va_linmath.h"
#include "va_colors.h"

#include <sdl2/SDL_scancode.h>
#include <sdl2/SDL_mouse.h>

#define GLAD_GLAPI_EXPORT
#include <glad/glad.h>

#include <cstdio>

GAME_API void
UpdateAndRender(b32 *Quit, b32 Reloaded, game_memory GameMemory) 
{
    game_state *GameState = (game_state *) GameMemory.Data;

    // SECTION: INIT
    
    if (!GameState->IsInitialized)
    {
        Assert(sizeof(game_state) < Megabytes(16));
        u8 *RootArenaBase = (u8 *) GameMemory.Data + Megabytes(16);
        size_t RootArenaSize = GameMemory.Size - Megabytes(16);
        GameState->RootArena = MemoryArena(RootArenaBase, RootArenaSize);

        GameState->WorldArena = MemoryArenaNested(&GameState->RootArena, Megabytes(32));
        GameState->ResourceArena = MemoryArenaNested(&GameState->RootArena, Megabytes(16));
        GameState->TrArenaA = MemoryArenaNested(&GameState->RootArena, Megabytes(16));
        GameState->TrArenaB = MemoryArenaNested(&GameState->RootArena, Megabytes(16));

        TraceLog("Root arena used mem: %zu/%zu MB", GameState->RootArena.Used / 1024 / 1024, GameState->RootArena.Size / 1024 / 1024);

        GameState->TestTex = SavLoadTexture("res/Test.png");

        GameState->IsInitialized = true;
    }

    if (Reloaded)
    {
    }

    // SECTION: GAME LOGIC PRE UPDATE

    if (KeyPressed(SDL_SCANCODE_B))
    {
        Breakpoint;
    }

    // SECTION: CHECK INPUTS

    if (KeyPressed(SDL_SCANCODE_F11)) ToggleWindowBorderless();
    
    // SECTION: GAME LOGIC UPDATE

    // SECTION: GAME LOGIC POST UPDATE
 
    // SECTION: RENDER
        
    BeginDraw();
    {
        ClearBackground(VA_BLACK);
     
        DrawTexture(GameState->TestTex, 100, 100);
    }
    EndDraw();

    SavSwapBuffers();
}
