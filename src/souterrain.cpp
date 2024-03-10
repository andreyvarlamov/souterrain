#include "sav_lib.h"
#include "souterrain.h"
#include "sou_templates.h"

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

#include "sou_world.cpp"

sav_texture
GenerateVignette(memory_arena *TrArenaA)
{
    int VigDim = 512;
    MemoryArena_Freeze(TrArenaA);
    u32 *VigData = MemoryArena_PushArray(TrArenaA, VigDim*VigDim, u32);
    f32 FadeOutEndR = VigDim / 2.0f;
    f32 FadeOutStartR = FadeOutEndR - 256.0f;
    for (int i = 0; i < VigDim*VigDim; i++)
    {
        vec2i P = IdxToXY(i, VigDim);
            
        u32 *Pixel = VigData + i;

        vec2 PFromCenter = (Vec2(P) + Vec2(0.5f)) - Vec2(VigDim / 2.0f);

        f32 R = SqrtF(PFromCenter.X * PFromCenter.X + PFromCenter.Y * PFromCenter.Y);

        color C;
        if (R > FadeOutEndR)
        {
            C = ColorAlpha(VA_BLACK, 0);
        }
        else if (R > FadeOutStartR && R <= FadeOutEndR)
        {
            f32 T = 1.0f - (R - FadeOutStartR) / (FadeOutEndR - FadeOutStartR);
            f32 A = EaseOutQuad(T);
            C = ColorAlpha(VA_BLACK, (u8) (A * 255.0f));
        }
        else
        {
            C = ColorAlpha(VA_BLACK, 255);
        }
            
        *Pixel = C.C32;
    }
    // SavSaveImage("temp/vig.png", VigData, VigDim, VigDim, false, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    sav_texture Tex = SavLoadTextureFromData(VigData, VigDim, VigDim);
    SavSetTextureWrapMode(Tex, SAV_CLAMP_TO_EDGE);
    
    MemoryArena_Unfreeze(TrArenaA);

    return Tex;
}

void
DrawGround(game_state *GameState, world *World, memory_arena *ScratchArena)
{
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    BeginDraw();
    {
        BeginCameraMode(&GameState->Camera);
        {
            glStencilMask(0xFF);

            ClearBackground(VA_BLACK);
            
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            for (int GroundVariant = 1; GroundVariant <= 3; GroundVariant++)
            {
                MemoryArena_Freeze(ScratchArena);
                
                int TileCount = World->Width * World->Height;
                vec3 *VertPositions = MemoryArena_PushArray(ScratchArena, TileCount * 4, vec3);
                vec4 *VertTexCoords = MemoryArena_PushArray(ScratchArena, TileCount * 4, vec4);
                vec4 *VertColors = MemoryArena_PushArray(ScratchArena, TileCount * 4, vec4);
                int CurrentVert = 0;
                u32 *VertIndices = MemoryArena_PushArray(ScratchArena, TileCount * 6, u32);
                int CurrentIndex = 0;

                for (int WorldI = 0; WorldI < World->Width * World->Height; WorldI++)
                {
                    b32 TileInitialized = World->TilesInitialized[WorldI];
                    u8 DarknessLevel = World->DarknessLevels[WorldI];
                    if (!TileInitialized || DarknessLevel == DARKNESS_UNSEEN) continue;
                    
                    switch (World->Tiles[WorldI])
                    {
                        case TILE_GRASS:
                        {
                            if (GroundVariant != 1) continue;
                        } break;

                        case TILE_WATER:
                        {
                            if (GroundVariant != 3) continue;
                        } break;
                        
                        case TILE_STONE:
                        default:
                        {
                            if (GroundVariant != 2) continue;
                        } break;
                    }
                        
                    vec2i WorldP = IdxToXY(WorldI, World->Width);
                    rect Dest = GetWorldDestRect(World, WorldP);

                    vec3 Positions[4];
                    RectGetPoints(Dest, Positions);
                    u32 Indices[] = { 0, 1, 2, 2, 3, 0 };

                    int BaseVert = CurrentVert;

                    for (int i = 0; i < ArrayCount(Positions); i++)
                    {
                        VertPositions[CurrentVert++] = Positions[i];
                    }
                
                    for (int i = 0; i < ArrayCount(Indices); i++)
                    {
                        VertIndices[CurrentIndex++] = Indices[i] + BaseVert;
                    }
                }

                glStencilFunc(GL_ALWAYS, GroundVariant, 0xFF);

                if (CurrentVert > 0 && CurrentIndex > 0)
                {
                    DrawVertices(VertPositions, VertTexCoords, VertColors, VertIndices, CurrentVert, CurrentIndex);
                }

                MemoryArena_Unfreeze(ScratchArena);
            }

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
        EndCameraMode();
    }
    EndDraw();

    BeginShaderMode(GameState->GroundShader);
    {
        BeginDraw();
        {
            BeginCameraMode(&GameState->Camera);
            {
                glStencilMask(0x00);
                
                BindTextureSlot(1, GameState->GroundBrushTex);
                BindTextureSlot(2, GameState->VigTex);
                
                for (int GroundVariant = 1; GroundVariant <= 3; GroundVariant++)
                {
                    glStencilFunc(GL_EQUAL, GroundVariant, 0xFF);

                    MemoryArena_Freeze(ScratchArena);
                    
                    vec3 *VertPositions = MemoryArena_PushArray(ScratchArena, World->GroundSplatCount * 4, vec3);
                    vec4 *VertTexCoords = MemoryArena_PushArray(ScratchArena, World->GroundSplatCount * 4, vec4);
                    vec4 *VertColors = MemoryArena_PushArray(ScratchArena, World->GroundSplatCount * 4, vec4);
                    vec4 C = ColorV4(VA_WHITE);
                    for (int VertColorI = 0; VertColorI < World->GroundSplatCount * 4; VertColorI++)
                    {
                        VertColors[VertColorI] = C;
                    }
                    int CurrentVert = 0;
                    u32 *VertIndices = MemoryArena_PushArray(ScratchArena, World->GroundSplatCount * 6, u32);
                    int CurrentIndex = 0;

                    ground_splat *GroundSplat = World->GroundSplats;
                    for (int SplatI = 0; SplatI < World->GroundSplatCount; SplatI++, GroundSplat++)
                    {
                        vec2 P = GroundSplat->Position;
                        
                        rect Dest = Rect(P.X, P.Y,
                                         GameState->GroundBrushRect.Width * GroundSplat->Scale,
                                         GameState->GroundBrushRect.Height * GroundSplat->Scale);
                        Dest.X -= Dest.Width / 2.0f;
                        Dest.Y -= Dest.Height / 2.0f;
    
                        vec3 Positions[4];
                        RectGetPoints(Dest, Positions);
    
                        vec2 TexCoords[4];
                        rect Source = Rect(0.0f, (GroundVariant-1) * GameState->GroundBrushRect.Height,
                                           GameState->GroundBrushRect.Width, GameState->GroundBrushRect.Height);
                        RectGetPoints(Source, TexCoords);
                        Rotate4PointsAroundOrigin(TexCoords, RectGetMid(Source), GroundSplat->Rotation);
                        NormalizeTexCoords(GameState->GroundBrushTex, TexCoords);
                        FlipTexCoords(TexCoords);

                        vec2 VigTexCoords[4];
                        rect VigSource = Rect(GameState->VigTex.Width, GameState->VigTex.Height);
                        RectGetPoints(VigSource, VigTexCoords);
                        NormalizeTexCoords(GameState->VigTex, VigTexCoords);
                        FlipTexCoords(VigTexCoords);

                        vec4 TexCoordsV4[4];
                        for (int i = 0; i < ArrayCount(TexCoords); i++)
                        {
                            TexCoordsV4[i] = Vec4(TexCoords[i].X, TexCoords[i].Y, VigTexCoords[i].X, VigTexCoords[i].Y);
                        }

                        u32 Indices[] = { 0, 1, 2, 2, 3, 0 };

                        int BaseVert = CurrentVert;

                        for (int i = 0; i < ArrayCount(Positions); i++)
                        {
                            VertPositions[CurrentVert] = Positions[i];
                            VertTexCoords[CurrentVert] = TexCoordsV4[i];
                            CurrentVert++;
                        }
                
                        for (int i = 0; i < ArrayCount(Indices); i++)
                        {
                            VertIndices[CurrentIndex++] = Indices[i] + BaseVert;
                        }
                    }

                    DrawVertices(VertPositions, VertTexCoords, VertColors, VertIndices, CurrentVert, CurrentIndex);

                    MemoryArena_Unfreeze(ScratchArena);
                }

                glStencilMask(0xFF); // NOTE: So that stencil mask can be cleared glClear
            }
            EndCameraMode();
        }
        EndDraw();
    }
    EndShaderMode();

    glDisable(GL_STENCIL_TEST);
}

void
DrawLighting(game_state *GameState)
{
    BeginTextureMode(GameState->LightingRenderTex, Rect(0)); BeginCameraMode(&GameState->Camera); 
    {
        ClearBackground(ColorAlpha(VA_WHITE, 0));

        MemoryArena_Freeze(&GameState->ScratchArenaA);
        
        int TileCount = GameState->World->Width * GameState->World->Height;
        vec3 *VertPositions = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec3);
        vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec4);
        vec4 *VertColors = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec4);
        int CurrentVert = 0;
        u32 *VertIndices = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 6, u32);
        int CurrentIndex = 0;
        
        for (int WorldI = 0; WorldI < TileCount; WorldI++)
        {
            u8 DarknessLevel = GameState->World->DarknessLevels[WorldI];
            if (GameState->World->DarknessLevels[WorldI] == DARKNESS_SEEN)
            {
                vec2i WorldP = IdxToXY(WorldI, GameState->World->Width);
                rect Dest = GetWorldDestRect(GameState->World, WorldP);

                vec3 Positions[4];
                RectGetPoints(Dest, Positions);
            
                vec4 ColV = ColorV4(ColorAlpha(VA_BLACK, DarknessLevel));

                int BaseVert = CurrentVert;
                for (int i = 0; i < ArrayCount(Positions); i++)
                {
                    VertPositions[CurrentVert] = Positions[i];
                    VertColors[CurrentVert] = ColV;
                    CurrentVert++;
                }

                u32 Indices[] = { 0, 1, 2, 2, 3, 0 };
                
                for (int i = 0; i < ArrayCount(Indices); i++)
                {
                    VertIndices[CurrentIndex++] = Indices[i] + BaseVert;
                }
            }
        }

        if (CurrentVert > 0 && CurrentIndex > 0)
        {
            DrawVertices(VertPositions, VertTexCoords, VertColors, VertIndices, CurrentVert, CurrentIndex);
        }

        MemoryArena_Unfreeze(&GameState->ScratchArenaA);
    }
    EndCameraMode(); EndTextureMode();
}

void
DrawEntities(game_state *GameState, vec3 LightPosition)
{
    static_i b32 WillDrawRoomGround = false;

    if (KeyPressed(SDL_SCANCODE_G))
    {
        WillDrawRoomGround = !WillDrawRoomGround;
    }
    
    BeginShaderMode(GameState->Glyph3DShader); BeginDraw();
    {
        SetUniformVec3("lightPos", &LightPosition.E[0]);
        
        // NOTE: Draw entities
        BeginCameraMode(&GameState->Camera);
        {
            MemoryArena_Freeze(&GameState->ScratchArenaA);

            int TileCount = GameState->World->Width * GameState->World->Height;
            vec3 *VertPositions = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec3);
            vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec4);
            vec4 *VertColors = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 4, vec4);
            int CurrentVert = 0;
            u32 *VertIndices = MemoryArena_PushArray(&GameState->ScratchArenaA, TileCount * 6, u32);
            int CurrentIndex = 0;

            for (int WorldI = 0; WorldI < TileCount; WorldI++)
            {
                u8 DarknessLevel = GameState->World->DarknessLevels[WorldI];
                if (DarknessLevel != DARKNESS_UNSEEN)
                {
                    entity *Entity = GameState->World->SpatialEntities[WorldI];

                    entity *EntityCursor = Entity;
                    while (EntityCursor)
                    {
                        if (EntityCursor->Type == ENTITY_NPC || EntityCursor->Type == ENTITY_PLAYER)
                        {
                            Entity  = EntityCursor;
                            break;
                        }
                        else if (Entity->Type != ENTITY_ITEM_PICKUP && EntityCursor->Type == ENTITY_ITEM_PICKUP)
                        {
                            Entity = EntityCursor;
                        }

                        EntityCursor = EntityCursor->Next;
                    }
                    
                    if (Entity)
                    {
                        if (DarknessLevel == DARKNESS_IN_VIEW || (Entity->Type != ENTITY_NPC))
                        {
                            vec2i WorldP = IdxToXY(WorldI, GameState->World->Width);
                            rect Dest = GetWorldDestRect(GameState->World, WorldP);
                            rect Source = GetGlyphSourceRect(GameState->GlyphAtlas, Entity->Glyph);

                            vec3 Positions[4];
                            RectGetPoints(Dest, Positions);
                            vec2 TexCoords[4];
                            GetTexCoordsForTex(GameState->GlyphAtlas.T, Source, TexCoords);
                            vec4 TexCoordsV4[4];
                            for (int i = 0; i < ArrayCount(TexCoords); i++)
                            {
                                TexCoordsV4[i] = Vec4(TexCoords[i], 0, 0);
                            }
                            vec4 ColV = ColorV4(Entity->Color);

                            int BaseVert = CurrentVert;
                            for (int i = 0; i < ArrayCount(Positions); i++)
                            {
                                VertPositions[CurrentVert] = Positions[i];
                                VertTexCoords[CurrentVert] = TexCoordsV4[i];
                                VertColors[CurrentVert] = ColV;
                                CurrentVert++;
                            }

                            u32 Indices[] = { 0, 1, 2, 2, 3, 0 };
                
                            for (int i = 0; i < ArrayCount(Indices); i++)
                            {
                                VertIndices[CurrentIndex++] = Indices[i] + BaseVert;
                            }
                        }
                    }
                }
            }

            if (CurrentVert > 0 && CurrentIndex > 0)
            {
                BindTextureSlot(1, GameState->GlyphAtlas.T);
                BindTextureSlot(2, GameState->GlyphAtlasNormalTex);

                DrawVertices(VertPositions, VertTexCoords, VertColors, VertIndices, CurrentVert, CurrentIndex);
            }

            MemoryArena_Unfreeze(&GameState->ScratchArenaA);
        }
        EndCameraMode();
    }
    EndShaderMode(); EndDraw();
}

void
DrawDebugUI(game_state *GameState, vec2i MouseTileP)
{
    world *World = GameState->World;
    DrawString(TextFormat("%0.3f FPS", GetFPSAvg(), GetDeltaAvg()),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               10, 10, 0,
               VA_MAROON,
               true, ColorAlpha(VA_BLACK, 128),
               &GameState->ScratchArenaA);

    DrawString(TextFormat("Tile: %d, %d", MouseTileP.X, MouseTileP.Y, GameState->World->DarknessLevels[XYToIdx(MouseTileP.X, MouseTileP.Y, GameState->World->Width)]),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               10, 60, 0,
               VA_MAROON,
               true, ColorAlpha(VA_BLACK, 128),
               &GameState->ScratchArenaA);

    DrawString(TextFormat("Darkness Level: %d", GameState->World->DarknessLevels[XYToIdx(MouseTileP.X, MouseTileP.Y, GameState->World->Width)]),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               10, 110, 0,
               VA_MAROON,
               true, ColorAlpha(VA_BLACK, 128),
               &GameState->ScratchArenaA);

    DrawString(TextFormat("Turn: %lld", World->CurrentTurn),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               10, 160, 0,
               VA_MAROON,
               true, ColorAlpha(VA_BLACK, 128),
               &GameState->ScratchArenaA);
}

void
DrawPlayerStatsUI(game_state *GameState, entity *Player, vec2 MouseWorldPxP)
{
    world *World = GameState->World;
    rect PlayerStatsRect = Rect(0, 780, 600, 300);
    DrawRect(PlayerStatsRect, ColorAlpha(VA_BLACK, 240));

    f32 PaddingX = 20.0f;
    f32 PaddingY = 20.0f;

    f32 TopY = PlayerStatsRect.Y + PaddingY;
    f32 LineX = PlayerStatsRect.X + PaddingX;
    f32 LineY = TopY;

    color PortraitColor = (((f32) Player->Health / (f32) Player->MaxHealth > 0.3f) ? VA_WHITE : Color(0xAE4642FF));

    f32 PortraitWidth = 160.0f;
    f32 PortraitHeight = 160.0f;
    f32 PortraitX = LineX;
    f32 PortraitY = PlayerStatsRect.Y + PlayerStatsRect.Height * 0.5f - PortraitHeight * 0.5f;

    DrawTexture(GameState->PlayerPortraitTex, Rect(PortraitX, PortraitY, PortraitWidth, PortraitHeight), PortraitColor);
    if (Player->Health > 0)
    {
        vec2 PlayerPxP = Vec2(Player->Pos.X * World->TilePxW + World->TilePxW * 0.5f,
                              Player->Pos.Y * World->TilePxH + World->TilePxH * 0.5f);
        vec2 EyesOffset = MouseWorldPxP - PlayerPxP;
        f32 MaxAwayFromPlayer = 200.0f;
        EyesOffset = (VecLengthSq(EyesOffset) > Square(MaxAwayFromPlayer) ?
                      VecNormalize(EyesOffset) * MaxAwayFromPlayer :
                      EyesOffset);
        EyesOffset *= 0.05f;
        DrawTexture(GameState->PlayerPortraitEyesTex, Rect(PortraitX + EyesOffset.X, PortraitY, PortraitWidth, PortraitHeight), PortraitColor);
    }

    LineX += PortraitWidth + PaddingX;

    f32 LineHeight = 37.0f;
    f32 MaxWidth = 180.0f;

    DrawString(TextFormat("Level: %d", Player->Level),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    DrawString(TextFormat("HP: %d/%d", Player->Health, Player->MaxHealth),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;
            
    DrawString(TextFormat("AC: %d", Player->ArmorClass),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    DrawString(TextFormat("Haima: %d", Player->Haima),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    DrawString(TextFormat("Kitrina: %d", Player->Kitrina),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    DrawString(TextFormat("Melana: %d", Player->Melana),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    DrawString(TextFormat("Sera: %d", Player->Sera),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;

    LineY = TopY;
    LineX += MaxWidth;

    DrawString(TextFormat("XP: %d", Player->XP),
               GameState->TitleFont,
               GameState->TitleFont->PointSize,
               LineX, LineY, 0,
               VA_WHITE,
               false, VA_BLACK,
               &GameState->ScratchArenaA);
    LineY += LineHeight;
            
}

void
DrawInventoryUI(game_state *GameState)
{
    entity *Player = GameState->World->PlayerEntity;
    f32 InventoryWidth = 500.0f;
    f32 InventoryHeight = 900.0f;
    rect InventoryRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - InventoryWidth * 0.5f,
                              GameState->UiRenderTex.Texture.Height * 0.5f - InventoryHeight * 0.5f,
                              InventoryWidth,
                              InventoryHeight);
    DrawRect(InventoryRect, ColorAlpha(VA_BLACK, 240));

    item *InventoryItem = Player->Inventory;
    f32 LineX = InventoryRect.X + 10.0f;
    f32 LineY = InventoryRect.Y + 10.0f;
    for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, InventoryItem++)
    {
        if (InventoryItem->ItemType != ITEM_NONE)
        {
            int ButtonPressed = GuiButtonRect(Rect(LineX, LineY, InventoryWidth - 20.0f, 40.0f));
            switch (ButtonPressed)
            {
                case SDL_BUTTON_LEFT:
                { 
                    SetItemToInspect(&GameState->InspectState, InventoryItem, NULL, INSPECT_ITEM_TO_DROP);
                } break;

                case SDL_BUTTON_RIGHT:
                { 
                    GameState->PlayerRequestedDropItem = InventoryItem;
                } break;

                default: break;
            }

            DrawString(TextFormat("%s", InventoryItem->Name),
                       GameState->BodyFont,
                       GameState->BodyFont->PointSize,
                       LineX, LineY, 0,
                       VA_WHITE,
                       false, VA_BLACK,
                       &GameState->ScratchArenaA);

            LineY += 40.0f;
        }
    }

}

void
DrawPickupUI(game_state *GameState)
{
    world *World = GameState->World;
    entity *Player = World->PlayerEntity;
    f32 InventoryWidth = 500.0f;
    f32 InventoryHeight = 900.0f;
    rect InventoryRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - InventoryWidth * 0.5f,
                              GameState->UiRenderTex.Texture.Height * 0.5f - InventoryHeight * 0.5f,
                              InventoryWidth,
                              InventoryHeight);
    DrawRect(InventoryRect, ColorAlpha(VA_BLACK, 240));

    f32 LineX = InventoryRect.X + 10.0f;
    f32 LineY = InventoryRect.Y + 10.0f;
            
    entity *ItemPickup = GetEntitiesAt(Player->Pos, World);
    while (ItemPickup)
    {
        if (ItemPickup->Type == ENTITY_ITEM_PICKUP)
        {
            item *ItemFromPickup = ItemPickup->Inventory;
            for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromPickup++)
            {
                if (ItemFromPickup->ItemType != ITEM_NONE)
                {
                    int ButtonPressed = GuiButtonRect(Rect(LineX, LineY, InventoryWidth - 20.0f, 40.0f));
                    switch (ButtonPressed)
                    {
                        case SDL_BUTTON_LEFT:
                        {
                            SetItemToInspect(&GameState->InspectState, ItemFromPickup, ItemPickup, INSPECT_ITEM_TO_PICKUP);
                        } break;

                        case SDL_BUTTON_RIGHT:
                        {
                            GameState->PlayerRequestedPickupItem = ItemFromPickup;
                            GameState->PlayerRequestedPickupItemItemPickup = ItemPickup;
                        } break;

                        default: break;
                    }

                    DrawString(TextFormat("%s", ItemFromPickup->Name),
                               GameState->BodyFont,
                               GameState->BodyFont->PointSize,
                               LineX, LineY, 0,
                               VA_WHITE,
                               false, VA_BLACK,
                               &GameState->ScratchArenaA);

                    LineY += 40.0f;
                }
            }
        }

        ItemPickup = ItemPickup->Next;
    }
}

void
DrawLevelupUISection(const char *Label, sav_font *Font, f32 FontSize,
                     rect Section, memory_arena *ScratchArena)
{
    vec2 LabelDim = GetStringDimensions(Label, Font, FontSize);
    
    f32 LabelX = Section.X + Section.Width * 0.5f - LabelDim.X * 0.5f;
    f32 LabelY = Section.Height * 0.33f;

    DrawString(TextFormat("%s", Label),
               Font, FontSize,
               LabelX, LabelY, 0,
               VA_WHITE, false, VA_WHITE,
               ScratchArena);
}

void
DrawLevelupUI(game_state *GameState)
{
    f32 InventoryWidth = 1000.0f;
    f32 InventoryHeight = 900.0f;
    rect InventoryRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - InventoryWidth * 0.5f,
                              GameState->UiRenderTex.Texture.Height * 0.5f - InventoryHeight * 0.5f,
                              InventoryWidth,
                              InventoryHeight);
    DrawRect(InventoryRect, ColorAlpha(VA_BLACK, 240));

    int Options = 4;
    f32 SectionWidth = InventoryWidth / 4;

    sav_font *Font = GameState->TitleFont;
    f32 FontSize = Font->PointSize;
    

    rect Section = Rect(InventoryRect.X, InventoryRect.Y, SectionWidth, InventoryRect.Height);

    DrawLevelupUISection("Haima", Font, FontSize, Section, &GameState->ScratchArenaA);
    Section.X += SectionWidth;

    DrawLevelupUISection("Kitrina", Font, FontSize, Section, &GameState->ScratchArenaA);
    Section.X += SectionWidth;

    DrawLevelupUISection("Melana", Font, FontSize, Section, &GameState->ScratchArenaA);
    Section.X += SectionWidth;

    DrawLevelupUISection("Sera", Font, FontSize, Section, &GameState->ScratchArenaA);
    Section.X += SectionWidth;
}

void
DrawInspectUI(game_state *GameState)
{
    world *World = GameState->World;
    b32 InspectMenuInternalButtonPressed = false;
    rect InspectRect = Rect(1500, 0, 420, 1080);
    switch (GameState->InspectState.T)
    {
        case INSPECT_ENTITY:
        {
            entity *EntityToInspect = GameState->InspectState.IS_Entity.EntityToInspect;
                
            DrawRect(InspectRect, ColorAlpha(VA_BLACK, 240));

            f32 LineX = 1510.0f;
            f32 LineY = 10.0f;
            f32 IncY = 50.0f;

            if (EntityToInspect->Name)
            {
                DrawString(TextFormat("%s (%d)", EntityToInspect->Name, EntityToInspect->DebugID),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           LineX, LineY, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
                LineY += IncY;
            }

            if (EntityToInspect->MaxHealth > 0)
            {
                DrawString(TextFormat("HP: %d/%d", EntityToInspect->Health, EntityToInspect->MaxHealth),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           LineX, LineY, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
                LineY += IncY;
                    
                DrawString(TextFormat("AC: %d", EntityToInspect->ArmorClass),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           LineX, LineY, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
                LineY += IncY;
                    
                DrawString(TextFormat("Damage: 1d%d", EntityToInspect->Damage),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           LineX, LineY, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
                LineY += IncY;
            }

            if (EntityToInspect->Type == ENTITY_NPC)
            {
                const char *Action;
                switch (EntityToInspect->NpcState)
                {
                    case NPC_STATE_IDLE:
                    {
                        Action = "Idle";
                    } break;
                        
                    case NPC_STATE_HUNTING:
                    {
                        Action = "Hunting";
                    } break;
                        
                    case NPC_STATE_SEARCHING:
                    {
                        Action = "Searching";
                    } break;

                    default:
                    {
                        Action = "Unknown";
                    } break;
                }

                DrawString(TextFormat("Action: %s", Action),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           LineX, LineY, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
                LineY += IncY;
            }
            
            if (EntityToInspect->Description)
            {
                DrawString(EntityToInspect->Description,
                           GameState->BodyFont,
                           GameState->BodyFont->PointSize,
                           LineX, LineY, 400,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);
            }
        } break;

        case INSPECT_ITEM_TO_PICKUP:
        case INSPECT_ITEM_TO_DROP:
        {
            item *ItemToInspect = GameState->InspectState.IS_Item.ItemToInspect;
            entity *ItemPickup = GameState->InspectState.IS_Item.ItemPickup;
                
            DrawRect(InspectRect, ColorAlpha(VA_BLACK, 240));

            if (ItemToInspect->Name)
            {
                DrawString(TextFormat("%s", ItemToInspect->Name),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           1510, 10, 0,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);

                DrawString(TextFormat("%s", ItemToInspect->Description),
                           GameState->BodyFont,
                           GameState->BodyFont->PointSize,
                           1510, 60, 400,
                           VA_WHITE,
                           false, VA_BLACK,
                           &GameState->ScratchArenaA);

                if (ItemToInspect->HaimaBonus > 0)
                {
                    DrawString(TextFormat("Haima Bonus: %d", ItemToInspect->HaimaBonus),
                               GameState->BodyFont,
                               GameState->BodyFont->PointSize,
                               1510, 100, 0,
                               VA_WHITE,
                               false, VA_BLACK,
                               &GameState->ScratchArenaA);
                }

                switch (GameState->InspectState.T)
                {
                    case INSPECT_ITEM_TO_PICKUP:
                    {
                        int ButtonPressed = GuiButtonRect(Rect(1510.0f, 1040.0f, InspectRect.Width - 20.0f, 40.0f));
                        if (ButtonPressed == SDL_BUTTON_LEFT)
                        {
                            GameState->PlayerRequestedPickupItem = ItemToInspect;
                            GameState->PlayerRequestedPickupItemItemPickup = ItemPickup;
                            InspectMenuInternalButtonPressed = true;
                        }

                        DrawString("PICK UP",
                                   GameState->TitleFont,
                                   GameState->TitleFont->PointSize,
                                   1510, 1040, 0,
                                   VA_WHITE,
                                   false, VA_BLACK,
                                   &GameState->ScratchArenaA);

                    } break;
                        
                    case INSPECT_ITEM_TO_DROP:
                    {
                        int ButtonPressed = GuiButtonRect(Rect(1510.0f, 1040.0f, InspectRect.Width - 20.0f, 40.0f));
                        if (ButtonPressed == SDL_BUTTON_LEFT)
                        {
                            GameState->PlayerRequestedDropItem = ItemToInspect;
                            InspectMenuInternalButtonPressed = true;
                        }

                        DrawString("DROP",
                                   GameState->TitleFont,
                                   GameState->TitleFont->PointSize,
                                   1510, 1040, 0,
                                   VA_WHITE,
                                   false, VA_BLACK,
                                   &GameState->ScratchArenaA);
                    } break;

                    default: break;
                }
            }
        } break;

        default: break;
    }

    if (GameState->InspectState.T != INSPECT_NONE && !GameState->InspectState.JustOpened && !InspectMenuInternalButtonPressed)
    {
        if (MouseReleased(SDL_BUTTON_LEFT) || MouseReleased(SDL_BUTTON_RIGHT) || KeyPressed(SDL_SCANCODE_ESCAPE))
        {
            ResetInspectMenu(&GameState->InspectState);
        }
    }
    else if (GameState->InspectState.JustOpened)
    {
        GameState->InspectState.JustOpened = false;
    }
}

void
ProcessInputs(game_state *GameState)
{
    game_input *GameInput = &GameState->GameInput;
    GameInput->MouseP = GetMousePos();
    GameInput->MouseWorldPxP = CameraScreenToWorld(&GameState->Camera, GetMousePos());
    GameInput->MouseWorldTileP = GetTilePFromPxP(GameState->World, GameInput->MouseWorldPxP);
}

void
ProcessPlayerInputs(game_state *GameState)
{
    req_action *Action = &GameState->PlayerReqAction; *Action = {};
    if (KeyPressedOrRepeat(SDL_SCANCODE_Q)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_W)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 0, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_E)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_A)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1,  0); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_X)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 0,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_D)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1,  0); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_Z)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_C)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_S)) Action->T = ACTION_SKIP_TURN;
    else if (KeyPressed(SDL_SCANCODE_R)) Action->T = ACTION_ITEM_DROP;
    else if (KeyPressed(SDL_SCANCODE_T)) Action->T = ACTION_TELEPORT;
    else if (KeyPressed(SDL_SCANCODE_I)) Action->T = ACTION_OPEN_INVENTORY;
    else if (KeyPressed(SDL_SCANCODE_G)) Action->T = ACTION_OPEN_PICKUP;
    else if (KeyPressed(SDL_SCANCODE_F)) Action->T = ACTION_OPEN_RANGED_ATTACK;
    else if (KeyPressed(SDL_SCANCODE_PERIOD) && (KeyDown(SDL_SCANCODE_LSHIFT) || KeyDown(SDL_SCANCODE_RSHIFT))) Action->T = ACTION_NEXT_LEVEL;
    else if (KeyPressed(SDL_SCANCODE_COMMA) && (KeyDown(SDL_SCANCODE_LSHIFT) || KeyDown(SDL_SCANCODE_RSHIFT))) Action->T = ACTION_PREV_LEVEL;
}


void
ProcessSwarms(world *World, i64 TurnsPassed)
{
    swarm *Swarm = World->Swarms;
    for (int i = 0; i < World->SwarmCount; i++, Swarm++)
    {
        if (Swarm->Cooldown > 0)
        {
            Swarm->Cooldown -= TurnsPassed;
        }
    }
}

void
ProcessPlayerFOV(world *World, b32 IgnoreFieldOfView)
{
    if (IgnoreFieldOfView)
    {
        memset(World->PlayerEntity->FieldOfView, 0x1, World->Width * World->Height * sizeof(World->PlayerEntity->FieldOfView[0]));
    }
    else
    {
        memset(World->PlayerEntity->FieldOfView, 0, World->Width * World->Height * sizeof(World->PlayerEntity->FieldOfView[0]));
        CalculateExhaustiveFOV(World, World->PlayerEntity->Pos, World->PlayerEntity->FieldOfView, World->PlayerEntity->ViewRange);
    }

    for (int i = 0; i < World->Width * World->Height; i++)
    {
        if (World->DarknessLevels[i] == DARKNESS_IN_VIEW)
        {
            World->DarknessLevels[i] = DARKNESS_SEEN;
        }

        if (IgnoreFieldOfView || World->PlayerEntity->FieldOfView == 0 || World->PlayerEntity->FieldOfView[i] == 1)
        {
            World->DarknessLevels[i] = DARKNESS_IN_VIEW;
        }
    }
}

void
DeleteDeadEntities(world *World)
{
    entity *Entity = World->Entities;
    for (int i = 0; i < World->EntityUsedCount; i++, Entity++)
    {
        if (EntityExists(Entity) && EntityIsDead(Entity))
        {
            if (Entity->ActionCost > 0)
            {
                EntityTurnQueueDelete(World, Entity);
            }
            if (Entity->Type == ENTITY_PLAYER)
            {
                ProcessPlayerFOV(World, true);
            }
            DeleteEntity(World, Entity);
        }
    }
}

void
ProcessMinedWalls(world *World)
{
    entity *Entity = World->Entities;
    for (int i = 0; i < World->EntityUsedCount; i++, Entity++)
    {
        if (Entity->Type == ENTITY_WALL && EntityIsDead(Entity))
        {
            vec2i EntityP = Entity->Pos;
            for (int Dir = 0; Dir < 8; Dir++)
            {
                vec2i NeighborP = EntityP + DIRECTIONS[Dir];
                if (IsPInBounds(NeighborP, World) && !IsPInitialized(NeighborP, World))
                {
                    InitializeP(NeighborP, World);
                    entity WallTemplate = Template_PumiceWall();
                    AddEntity(World, NeighborP, &WallTemplate);
                }
            }
        }
    }
}

GAME_API void
UpdateAndRender(b32 *Quit, b32 Reloaded, game_memory GameMemory) 
{
    game_state *GameState = (game_state *) GameMemory.Data;

    // SECTION: INIT
    
    if (!GameState->IsInitialized)
    {
        Assert(sizeof(game_state) < Megabytes(GameMemory.Size));

        GameState->ResourceArena = AllocArena(Megabytes(16));
        GameState->ScratchArenaA = AllocArena(Megabytes(16));
        GameState->ScratchArenaB = AllocArena(Megabytes(16));

        // NOTE: Shaders
        GameState->GroundShader = BuildCustomShader("res/ground.vs", "res/ground.fs");
        BeginShaderMode(GameState->GroundShader);
        {
            SetUniformI("sprite", 1);
            SetUniformI("vig", 2);
        }
        EndShaderMode();
        GameState->Glyph3DShader = BuildCustomShader("res/glyph3d.vs", "res/glyph3d.fs");
        BeginShaderMode(GameState->Glyph3DShader);
        {
            SetUniformI("diffuse", 1);
            SetUniformI("normal", 2);
        }
        EndShaderMode();

        // NOTE: Fonts
        GameState->TitleFont = SavLoadFont(&GameState->ResourceArena, "res/ProtestStrike-Regular.ttf", 32);
        GameState->BodyFont = SavLoadFont(&GameState->ResourceArena, "res/GildaDisplay-Regular.ttf", 28);
        // GameState->BodyFont = SavLoadFont(&GameState->ResourceArena, "res/ProtestStrike-Regular.ttf", 28);

        // NOTE: Textures
        GameState->GlyphAtlas.T = SavLoadTexture("res/NewFontWhite2.png");
        GameState->GlyphAtlas.GlyphsPerRow = 16;
        GameState->GlyphAtlas.GlyphPadX = 1;
        GameState->GlyphAtlas.GlyphPadY = 1;
        GameState->GlyphAtlas.GlyphPxW = 42;
        GameState->GlyphAtlas.GlyphPxH = 54;
        GameState->GlyphAtlas.GlyphPxWPad = GameState->GlyphAtlas.GlyphPxW + 2 * GameState->GlyphAtlas.GlyphPadX;
        GameState->GlyphAtlas.GlyphPxHPad = GameState->GlyphAtlas.GlyphPxH + 2 * GameState->GlyphAtlas.GlyphPadY;
        GameState->GlyphAtlasNormalTex = SavLoadTexture("res/NewFontNormals5.png");
        GameState->VigTex = GenerateVignette(&GameState->ScratchArenaA);
        GameState->GroundBrushTex = SavLoadTexture("res/GroundBrushes5.png");
        GameState->GroundBrushRect = Rect(GameState->GroundBrushTex.Width, GameState->GroundBrushTex.Width);
        GameState->PlayerPortraitTex = SavLoadTexture("res/PlayerPortraitCursed.png");
        GameState->PlayerPortraitEyesTex = SavLoadTexture("res/PlayerPortraitEyes.png");

        // NOTE: Render textures
        GameState->UiRect = Rect(GetWindowSize());
        GameState->UiRenderTex = SavLoadRenderTexture((int) GetWindowOrigSize().X, (int) GetWindowOrigSize().Y, false);
        GameState->LightingRenderTex = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);
        GameState->DebugOverlay = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);

        // NOTE: Audio
        // GameState->BackgroundMusic = LoadMusicStream("res/20240204 Calling.mp3");

        int WorldW = 100;
        int WorldH = 100;
        GameState->CurrentWorld = 0;
        GameState->World = GameState->OtherWorlds[GameState->CurrentWorld] = GenerateWorld(WorldW, WorldH,
                                                                                           GameState->GlyphAtlas.GlyphPxW,
                                                                                           GameState->GlyphAtlas.GlyphPxH,
                                                                                           &GameState->ScratchArenaA);

        ProcessPlayerFOV(GameState->World, false);
        
        TraceLog("Generated world #%d. World arena used size: %zu KB. Each entity is %zu bytes.",
                 GameState->CurrentWorld, GameState->World->Arena.Used, sizeof(*GameState->World->Entities));

        GameState->Camera.Rotation = 0.0f;
        CameraInitLogZoomSteps(&GameState->Camera, 0.2f, 5.0f, 5);
        GameState->Camera.Offset = GetWindowSize() / 2.0f;
        GameState->Camera.Target = GetPxPFromTileP(GameState->World, GameState->World->PlayerEntity->Pos);

        // NOTE: Misc
        // PlayMusicStream(GameState->BackgroundMusic);

        GameState->IsInitialized = true;
    }

    if (Reloaded)
    {
        DeleteShader(&GameState->GroundShader);
        GameState->GroundShader = BuildCustomShader("res/ground.vs", "res/ground.fs");
        BeginShaderMode(GameState->GroundShader);
        {
            SetUniformI("sprite", 1);
            SetUniformI("vig", 2);
        }
        EndShaderMode();
        DeleteShader(&GameState->Glyph3DShader);
        GameState->Glyph3DShader = BuildCustomShader("res/glyph3d.vs", "res/glyph3d.fs");
        BeginShaderMode(GameState->Glyph3DShader);
        {
            SetUniformI("diffuse", 1);
            SetUniformI("normal", 2);
        }
        EndShaderMode();
    }

    // SECTION: GAME LOGIC PRE UPDATE
    MemoryArena_Reset(&GameState->ScratchArenaA);

    world *World = GameState->World;
    entity *Player = World->PlayerEntity;

    if (WindowSizeChanged())
    {
        GameState->Camera.Offset = GetWindowSize() / 2.0f;
        GameState->UiRect = Rect(GetWindowSize());
        SavDeleteRenderTexture(&GameState->LightingRenderTex);
        GameState->LightingRenderTex = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);
        SavDeleteRenderTexture(&GameState->DebugOverlay);
        GameState->DebugOverlay = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);
    }

#ifdef SAV_DEBUG  
    if (KeyPressed(SDL_SCANCODE_B))
    {
        Breakpoint;

        entity **DebugEntities;
        int Count;
        GetAllEntitiesOfType(ENTITY_ITEM_PICKUP, World, &GameState->ScratchArenaA, &DebugEntities, &Count);

        GetAllEntitiesOfType(ENTITY_NPC, World, &GameState->ScratchArenaA, &DebugEntities, &Count);

        Noop;
    }
#endif

    // SECTION: CHECK INPUTS
    game_input *GameInput = &GameState->GameInput;
    ProcessInputs(GameState);
    if (KeyPressed(SDL_SCANCODE_F11)) ToggleWindowBorderless();
    if (MouseWheel() != 0) CameraIncreaseLogZoomSteps(&GameState->Camera, MouseWheel());
    if (MouseDown(SDL_BUTTON_MIDDLE)) GameState->Camera.Target -= CameraScreenToWorldRel(&GameState->Camera, GetMouseRelPos());
    if (MouseReleased(SDL_BUTTON_RIGHT))
    {
        if (IsInFOV(World, Player->FieldOfView, GameInput->MouseWorldTileP))
        {
            entity *EntityToInspect = GetEntitiesAt(GameInput->MouseWorldTileP, World);
            if (EntityToInspect)
            {
                SetEntityToInspect(&GameState->InspectState, EntityToInspect);
            }
            else
            {
                ResetInspectMenu(&GameState->InspectState);
            }
        }
    }
    if (KeyPressed(SDL_SCANCODE_F1))
    {
        GameState->IgnoreFieldOfView = !GameState->IgnoreFieldOfView;
        ProcessPlayerFOV(World, GameState->IgnoreFieldOfView);
    }
    if (KeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (GameState->InspectState.T == INSPECT_NONE)
        {
            *Quit = true;
        }
    }

    // SECTION: GAME LOGIC UPDATE
    switch (GameState->RunState)
    {
        case RUN_STATE_NONE:
        {
            GameState->RunState = RUN_STATE_IN_GAME;
        } break;

        case RUN_STATE_LOAD_WORLD:
        {
            Assert(GameState->CurrentWorld < MAX_WORLDS);
                
            if (GameState->OtherWorlds[GameState->CurrentWorld] == NULL)
            {
                int WorldW = 100;
                int WorldH = 100;
                world *NewWorld = GenerateWorld(WorldW, WorldH,
                                                GameState->GlyphAtlas.GlyphPxW,
                                                GameState->GlyphAtlas.GlyphPxH,
                                                &GameState->ScratchArenaA);

                TraceLog("Generated world #%d. World arena used size: %zu KB. Each entity is %zu bytes.",
                         GameState->CurrentWorld, NewWorld->Arena.Used, sizeof(*GameState->World->Entities));

                GameState->OtherWorlds[GameState->CurrentWorld] = NewWorld;
            }

            GameState->World = GameState->OtherWorlds[GameState->CurrentWorld];
            
            World = GameState->World;
            entity *OldPlayer = Player;
            Player = World->PlayerEntity;

            if (OldPlayer != Player)
            {
                CopyEntity(OldPlayer, Player, World);
            }

            GameState->Camera.Target = GetPxPFromTileP(World, Player->Pos);

            ProcessPlayerFOV(World, GameState->IgnoreFieldOfView);

            TraceLog("Loaded world #%d.", GameState->CurrentWorld);
                
            GameState->RunState = RUN_STATE_IN_GAME;
        } break;

        case RUN_STATE_IN_GAME:
        {
            i64 StartTurn = World->CurrentTurn;
            
            entity *ActiveEntity = EntityTurnQueuePeek(World);
            if (ActiveEntity == Player)
            {
                ProcessPlayerInputs(GameState);
                b32 TurnUsed = UpdatePlayer(Player, World, &GameState->Camera, &GameState->PlayerReqAction, GameState);
                if (TurnUsed)
                {
                    World->CurrentTurn += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
                    EntityRegen(Player, World);
                    ActiveEntity = EntityTurnQueuePeek(World);
                    ProcessPlayerFOV(World, GameState->IgnoreFieldOfView);
                }
            }

            b32 FirstEntity = true;
            entity *StartingEntity = ActiveEntity;
            while (ActiveEntity != Player && (FirstEntity || ActiveEntity != StartingEntity))
            {
                switch (ActiveEntity->Type)
                {
                    case ENTITY_NPC:
                    {
                        b32 TurnUsed = UpdateNpc(GameState, World, ActiveEntity);
                        if (TurnUsed)
                        {
                            World->CurrentTurn += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
                            EntityRegen(ActiveEntity, World);
                        }
                    } break;

                    default:
                    {
                        EntityTurnQueuePopWithoutConsumingCost(World);
                    } break;
                }

                ActiveEntity = EntityTurnQueuePeek(World);

                FirstEntity = false;
            }

            i64 TurnsPassed = World->CurrentTurn - StartTurn;

            ProcessSwarms(World, TurnsPassed);
 
            ProcessMinedWalls(World);

            DeleteDeadEntities(World);
        } break;

        case RUN_STATE_INVENTORY_MENU:
        {
            if (KeyPressed(SDL_SCANCODE_I) || KeyPressed(SDL_SCANCODE_ESCAPE))
            {
                ResetInspectMenu(&GameState->InspectState);
                GameState->RunState = RUN_STATE_IN_GAME;
            }

            if (GameState->PlayerRequestedDropItem && GameState->PlayerRequestedDropItem->ItemType != ITEM_NONE)
            {
                entity ItemPickupTestTemplate = {};
                ItemPickupTestTemplate.Type = ENTITY_ITEM_PICKUP;
                entity *ItemPickup = AddEntity(World, Player->Pos, &ItemPickupTestTemplate);
                Assert(ItemPickup->Inventory);

                if (AddItemToEntityInventory(GameState->PlayerRequestedDropItem, ItemPickup))
                {
                    RemoveItemFromEntityInventory(GameState->PlayerRequestedDropItem, Player);
                }

                GameState->PlayerRequestedDropItem = NULL;

                ResetInspectMenu(&GameState->InspectState);

                RefreshItemPickupState(ItemPickup);
                DeleteDeadEntities(World);
            }
        } break;

        case RUN_STATE_PICKUP_MENU:
        {
            if (KeyPressed(SDL_SCANCODE_I))
            {
                ResetInspectMenu(&GameState->InspectState);
                GameState->RunState = RUN_STATE_INVENTORY_MENU;
            }
            
            if (KeyPressed(SDL_SCANCODE_ESCAPE))
            {
                ResetInspectMenu(&GameState->InspectState);
                GameState->RunState = RUN_STATE_IN_GAME;
            }

            if (KeyPressed(SDL_SCANCODE_G))
            {
                entity *ItemPickup = GetEntitiesAt(Player->Pos, World);
                while (ItemPickup)
                {
                    if (ItemPickup->Type == ENTITY_ITEM_PICKUP)
                    {
                        item *ItemFromPickup = ItemPickup->Inventory;
                        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromPickup++)
                        {
                            if (ItemFromPickup->ItemType != ITEM_NONE)
                            {
                                if (AddItemToEntityInventory(ItemFromPickup, Player))
                                {
                                    // TODO: This is kind of a waste
                                    RemoveItemFromEntityInventory(ItemFromPickup, ItemPickup);
                                }
                            }
                        }

                        RefreshItemPickupState(ItemPickup);
                    } 
                        
                    ItemPickup = ItemPickup->Next;
                }

                ResetInspectMenu(&GameState->InspectState);
                GameState->RunState = RUN_STATE_IN_GAME;
            }

            if (GameState->PlayerRequestedPickupItem && GameState->PlayerRequestedPickupItem->ItemType != ITEM_NONE)
            {
                Assert(GameState->PlayerRequestedPickupItemItemPickup != NULL && GameState->PlayerRequestedPickupItemItemPickup->Type != ENTITY_NONE);

                if (AddItemToEntityInventory(GameState->PlayerRequestedPickupItem, Player))
                {
                    RemoveItemFromEntityInventory(GameState->PlayerRequestedPickupItem, GameState->PlayerRequestedPickupItemItemPickup);
                }

                RefreshItemPickupState(GameState->PlayerRequestedPickupItemItemPickup);
                
                ResetInspectMenu(&GameState->InspectState);
                
                GameState->PlayerRequestedPickupItem = NULL;
                GameState->PlayerRequestedPickupItemItemPickup = NULL;
                DeleteDeadEntities(World);
            }
        } break;

        case RUN_STATE_RANGED_ATTACK:
        {
            if (MouseWheel() != 0) CameraIncreaseLogZoomSteps(&GameState->Camera, MouseWheel());
            if (MouseDown(SDL_BUTTON_MIDDLE)) GameState->Camera.Target -= CameraScreenToWorldRel(&GameState->Camera, GetMouseRelPos());
            
            if (KeyPressed(SDL_SCANCODE_ESCAPE) || KeyPressed(SDL_SCANCODE_F))
            {
                GameState->RunState = RUN_STATE_IN_GAME;
            }
            
            if (MouseReleased(SDL_BUTTON_LEFT))
            {
                if (IsInRangedAttackRange(World, Player->Pos, GameInput->MouseWorldTileP, Player->RangedRange))
                {
                    entity *EntityToHit = GetEntitiesOfTypeAt(GameInput->MouseWorldTileP, ENTITY_NPC, World);
                    if (EntityToHit)
                    {
                        GameState->EntityToHit = EntityToHit;
                        GameState->RunState = RUN_STATE_IN_GAME;
                    }
                }
            }
        } break;

        default:
        {
            TraceLog("Unknown run state: %d", GameState->RunState);
            *Quit = true;
        };
    }

    Assert(ValidateEntitySpatialPartition(GameState->World));

    // static_p int VizGen = 0;

    // if (KeyPressedOrRepeat(SDL_SCANCODE_RIGHT))
    // {
    //     VizGen++;
    // }
    // else if (KeyPressedOrRepeat(SDL_SCANCODE_LEFT))
    // {
    //     VizGen--;
    //     if (VizGen < 0)
    //     {
    //         VizGen = 0;
    //     }
    // }

    // SECTION: RENDER
    // NOTE: Draw debug overlay
    BeginTextureMode(GameState->DebugOverlay, Rect(0)); BeginCameraMode(&GameState->Camera); 
    {
        ClearBackground(ColorAlpha(VA_WHITE, 0));

        if (GameState->RunState == RUN_STATE_RANGED_ATTACK)
        {
            for (int Y = Player->Pos.Y - Player->RangedRange; Y <= Player->Pos.Y + Player->RangedRange; Y++)
            {
                for (int X = Player->Pos.X - Player->RangedRange; X <= Player->Pos.X + Player->RangedRange; X++)
                {
                    vec2i P = Vec2I(X, Y);
                    if (IsInRangedAttackRange(World, Player->Pos, P, Player->RangedRange))
                    {
                        color C = (P == GameInput->MouseWorldTileP) ? ColorAlpha(VA_RED, 200) : ColorAlpha(VA_RED, 150);
                        DrawRect(World, P, C);
                    }
                }
            }
        }

        // path_result Path = CalculatePath(World, Player->Pos, World->Exit, &GameState->ScratchArenaA, &GameState->ScratchArenaB, 512, false, VizGen);

        // if (Path.FoundPath && Path.Path)
        // {
        //     vec2i *Step = Path.Path;
        //     for (int i = 0; i < Path.PathSteps; i++, Step++)
        //     {
        //         DrawRect(World, *Step, ColorAlpha(VA_RED, 150));
        //     }
        // }
    }
    EndCameraMode(); EndTextureMode();

    // NOTE: Draw UI to its own render texture
    BeginTextureMode(GameState->UiRenderTex, GameState->UiRect);
    {
        ClearBackground(ColorAlpha(VA_WHITE, 0));

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        
        DrawDebugUI(GameState, GameInput->MouseWorldTileP);
        
        DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);

        switch (GameState->RunState)
        {
            case RUN_STATE_INVENTORY_MENU:
            {
                DrawInventoryUI(GameState);
            } break;

            case RUN_STATE_PICKUP_MENU:
            {
                DrawPickupUI(GameState);
            } break;

            // case RUN_STATE_LEVELUP_MENU:
            // {
            //     DrawLevelupUI(GameState);
            // } break;

            default: break;
        }

        DrawInspectUI(GameState);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    EndTextureMode();

    DrawLighting(GameState);
    
    DrawGround(GameState, World, &GameState->ScratchArenaA);

    f32 LightHeight = 150.0f;
    f32 MaxAwayFromCenter = 50.0f;
    vec2 ScreenCenter = CameraScreenToWorld(&GameState->Camera, GetWindowSize() * 0.5f);
    vec2 LightOffset = GameInput->MouseWorldPxP - ScreenCenter;
    LightOffset = (VecLengthSq(LightOffset) > Square(MaxAwayFromCenter) ?
                   VecNormalize(LightOffset) * MaxAwayFromCenter :
                   LightOffset);
    DrawEntities(GameState, Vec3(ScreenCenter + LightOffset, LightHeight));

    BeginDraw();
    {
        // NOTE: Draw overlay render textures: lighting, debug, UI
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ONE, GL_ZERO);
        DrawTexture(GameState->LightingRenderTex.Texture, Rect(GetWindowSize()), VA_WHITE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

        DrawTexture(GameState->DebugOverlay.Texture, Rect(GetWindowSize()), VA_WHITE);

        DrawTexture(GameState->UiRenderTex.Texture, GameState->UiRect, VA_WHITE);
    }
    EndDraw();

    SavSwapBuffers();
}
