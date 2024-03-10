#include "sav_lib.h"
#include "souterrain.h"

b32
DrawLevelupUISection(const char *Label, const char *Description,
                     sav_font *TitleFont, f32 TitleFontSize,
                     sav_font *BodyFont, f32 BodyFontSize,
                     sav_texture Icon,
                     rect Section, memory_arena *ScratchArena)
{
    int ButtonClicked = GuiButtonRect(Section);

    f32 IconPadding = 80.0f;
    rect IconDest = Rect(0, 0, 128, 128);
    IconDest.X = Section.X + Section.Width * 0.5f - IconDest.Width * 0.5f;
    IconDest.Y = Section.Y + IconPadding;
        
    DrawTexture(Icon, IconDest, VA_WHITE);

    vec2 LabelDim = GetStringDimensions(Label, TitleFont, TitleFontSize);
    f32 LabelX = Section.X + Section.Width * 0.5f - LabelDim.X * 0.5f;
    f32 LabelY = Section.Y + Section.Height * 0.3f;
    DrawString(TextFormat("%s", Label),
               TitleFont, TitleFontSize,
               LabelX, LabelY, 0,
               VA_WHITE, false, VA_WHITE,
               ScratchArena);

    f32 DescriptionPadding = 13.0f;
    f32 DescriptionX = Section.X + DescriptionPadding;
    f32 DescriptionY = LabelY + 70.0f;
    
    DrawString(TextFormat("%s", Description),
               BodyFont, BodyFontSize,
               DescriptionX, DescriptionY, Section.Width - DescriptionPadding * 2.0f,
               VA_WHITE, false, VA_WHITE,
               ScratchArena);
    

    return (ButtonClicked == SDL_BUTTON_LEFT);
}

run_state
RunState_LevelupMenu(game_state *GameState)
{
    world *World = GameState->World;
    entity *Player = World->PlayerEntity;
    game_input *GameInput = &GameState->GameInput;

    DrawGame(GameState, World);

    BeginUIDraw(GameState);
    DrawDebugUI(GameState, Vec2I(0, 0));
    DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);

    f32 LevelupWidth = 1500.0f;
    f32 LevelupHeight = 900.0f;
    rect LevelupRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - LevelupWidth * 0.5f,
                            GameState->UiRenderTex.Texture.Height * 0.5f - LevelupHeight * 0.5f,
                            LevelupWidth,
                            LevelupHeight);
    DrawRect(LevelupRect, ColorAlpha(VA_BLACK, 240));

    int Options = 4;
    f32 SectionWidth = LevelupWidth / 4;

    sav_font *TitleFont = GameState->TitleFont;
    f32 TitleFontSize = TitleFont->PointSize;
    sav_font *BodyFont = GameState->BodyFont;
    f32 BodyFontSize = BodyFont->PointSize;

    const char *Title = "LEVEL UP!";
    vec2 TitleDim = GetStringDimensions(Title, TitleFont, TitleFontSize);
    f32 TitleX = LevelupRect.X + LevelupRect.Width * 0.5f - TitleDim.X * 0.5f;
    f32 TopPadding = 20.0f;
    f32 TitleY = LevelupRect.Y + TopPadding;
    f32 TitleHeight = 50.0f;
    
    DrawString(Title,
               TitleFont, TitleFontSize,
               TitleX, TitleY, 0,
               VA_WHITE, false, VA_WHITE,
               &GameState->ScratchArenaA);

    rect Section = Rect(LevelupRect.X, TitleY + TitleHeight, SectionWidth, LevelupRect.Height - TopPadding - TitleHeight);

    b32 StatChosen = false;
    const char *HaimaLabel = "HAIMA";
    const char *HaimaDescription = "Health points, melee attack damage, regen amount.";
    if (DrawLevelupUISection(HaimaLabel, HaimaDescription, TitleFont, TitleFontSize, BodyFont, BodyFontSize, GameState->HaimaTex, Section, &GameState->ScratchArenaA))
    {
        Player->Haima++;
        StatChosen = true;
    }
    Section.X += SectionWidth;

    const char *KitrinaLabel = "KITRINA";
    const char *KitrinaDescription = "Melee and ranged accuracy, ranged attack range, armor class.";
    if (DrawLevelupUISection(KitrinaLabel, KitrinaDescription, TitleFont, TitleFontSize, BodyFont, BodyFontSize, GameState->KitrinaTex, Section, &GameState->ScratchArenaA))
    {
        Player->Kitrina++;
        StatChosen = true;
    }
    Section.X += SectionWidth;

    const char *MelanaLabel = "MELANA";
    const char *MelanaDescription = "Fireball damage, fireball range, fireball area of effect, mana points.";
    if (DrawLevelupUISection(MelanaLabel, MelanaDescription, TitleFont, TitleFontSize, BodyFont, BodyFontSize, GameState->MelanaTex, Section, &GameState->ScratchArenaA))
    {
        Player->Melana++;
        StatChosen = true;
    }
    Section.X += SectionWidth;

    const char *SeraLabel = "SERA";
    const char *SeraDescription = "Rend mind damage, rend mind range.";
    if (DrawLevelupUISection(SeraLabel, SeraDescription, TitleFont, TitleFontSize, BodyFont, BodyFontSize,  GameState->SeraTex, Section, &GameState->ScratchArenaA))
    {
        Player->Sera++;
        StatChosen = true;
    }
    Section.X += SectionWidth;

    SetEntityStatsBasedOnAttributes(Player);
    
    EndUIDraw();

    return StatChosen ? RUN_STATE_IN_GAME : RUN_STATE_LEVELUP_MENU;
}
