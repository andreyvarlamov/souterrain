#include "sav_lib.h"
#include "souterrain.h"

run_state
RunState_MainMenu(game_state *GameState)
{
    BeginUIDraw(GameState);

    sav_font *TitleFont = GameState->TitleFont64;
    f32 TitleFontSize = 64.0f;

    f32 Width = 1920.0f;
    f32 Height = 1080.0f;

    rect TitleTexRect = Rect(0,0, 1000, 500);
    TitleTexRect.X = Width * 0.5f - TitleTexRect.Width * 0.5f;
    TitleTexRect.Y = 100.0f;

    DrawTexture(GameState->TitleTex, TitleTexRect, VA_WHITE);
    
    const char *Title = "SOUTERRAIN";
    vec2 TitleDim = GetStringDimensions(Title, TitleFont, TitleFontSize);
    f32 TitleX = Width * 0.5f - TitleDim.X * 0.5f;
    f32 TitleY = Height * 0.7f;
    f32 TitleHeight = 100.0f;
    
    DrawString(Title,
               TitleFont, TitleFontSize,
               TitleX, TitleY, 0,
               VA_WHITE, false, VA_WHITE,
               &GameState->ScratchArenaA);

    sav_font *ButtonFont = GameState->TitleFont;
    f32 ButtonFontSize = 32.0f;

    const char *PlayButton = "PLAY";
    vec2 ButtonDim = GetStringDimensions(PlayButton, ButtonFont, ButtonFontSize);
    f32 ButtonX = Width * 0.5f - ButtonDim.X * 0.5f;
    f32 ButtonY = TitleY + TitleHeight;
    f32 ButtonHeight = 50.0f;


    int PlayButtonClicked = GuiButtonRect(Rect(TitleX, ButtonY - 5.0f, TitleDim.X, ButtonHeight));

    DrawString(PlayButton,
               ButtonFont, ButtonFontSize,
               ButtonX, ButtonY, 0,
               VA_WHITE, false, VA_WHITE,
               &GameState->ScratchArenaA);

    const char *ExitButton = "EXIT";
    ButtonDim = GetStringDimensions(ExitButton, ButtonFont, ButtonFontSize);
    ButtonX = Width * 0.5f - ButtonDim.X * 0.5f;
    ButtonY += ButtonHeight;

    int ExitButtonClicked = GuiButtonRect(Rect(TitleX, ButtonY - 5.0f, TitleDim.X, ButtonHeight));

    DrawString(ExitButton,
               ButtonFont, ButtonFontSize,
               ButtonX, ButtonY, 0,
               VA_WHITE, false, VA_WHITE,
               &GameState->ScratchArenaA);

    const char *Signature = "MADE BY VARAND";
    DrawString(Signature,
               GameState->TitleFont, GameState->TitleFont->PointSize,
               1650.0f, 1030.0f, 0,
               VA_WHITE, false, VA_WHITE,
               &GameState->ScratchArenaA);
    
    EndUIDraw();

    if (PlayButtonClicked == SDL_BUTTON_LEFT)
    {
        return RUN_STATE_IN_GAME;
    }

    if (ExitButtonClicked == SDL_BUTTON_LEFT)
    {
        return RUN_STATE_QUIT;
    }

    return RUN_STATE_MAIN_MENU;
}
