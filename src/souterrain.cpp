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
DrawGround(game_state *GameState)
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
                MemoryArena_Freeze(&GameState->TrArenaA);
                
                int TileCount = GameState->World.Width * GameState->World.Height;
                vec3 *VertPositions = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec3);
                vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
                vec4 *VertColors = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
                int CurrentVert = 0;
                u32 *VertIndices = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 6, u32);
                int CurrentIndex = 0;

                for (int WorldI = 0; WorldI < GameState->World.Width * GameState->World.Height; WorldI++)
                {
                    b32 TileInitialized = GameState->World.TilesInitialized[WorldI];
                    u8 DarknessLevel = GameState->World.DarknessLevels[WorldI];
                    if (!TileInitialized || DarknessLevel == DARKNESS_UNSEEN) continue;
                    
                    switch (GameState->World.Tiles[WorldI])
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
                        
                    vec2i WorldP = IdxToXY(WorldI, GameState->World.Width);
                    rect Dest = GetWorldDestRect(GameState->World, WorldP);

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

                MemoryArena_Unfreeze(&GameState->TrArenaA);
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

                    MemoryArena_Freeze(&GameState->TrArenaA);
                    
                    vec3 *VertPositions = MemoryArena_PushArray(&GameState->TrArenaA, GameState->GroundPointCount * 4, vec3);
                    vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->TrArenaA, GameState->GroundPointCount * 4, vec4);
                    vec4 *VertColors = MemoryArena_PushArray(&GameState->TrArenaA, GameState->GroundPointCount * 4, vec4);
                    vec4 C = ColorV4(VA_WHITE);
                    for (int i = 0; i < GameState->GroundPointCount * 4; i++)
                    {
                        VertColors[i] = C;
                    }
                    int CurrentVert = 0;
                    u32 *VertIndices = MemoryArena_PushArray(&GameState->TrArenaA, GameState->GroundPointCount * 6, u32);
                    int CurrentIndex = 0;
                    
                    for (int GroundPointI = 0; GroundPointI < GameState->GroundPointCount; GroundPointI++)
                    {
                        vec2 P = GameState->GroundPoints[GroundPointI];
                        vec2 Rots = GameState->GroundRots[GroundPointI];
                        f32 Scale = 10.0f;

                        rect Dest = Rect(P.X, P.Y, GameState->GroundBrushRect.Width * Scale, GameState->GroundBrushRect.Height * Scale);
                        Dest.X -= Dest.Width / 2.0f;
                        Dest.Y -= Dest.Height / 2.0f;
    
                        vec3 Positions[4];
                        RectGetPoints(Dest, Positions);
    
                        vec2 TexCoords[4];
                        rect Source = Rect(0.0f, (GroundVariant-1) * GameState->GroundBrushRect.Height, GameState->GroundBrushRect.Width, GameState->GroundBrushRect.Height);
                        RectGetPoints(Source, TexCoords);
                        Rotate4PointsAroundOrigin(TexCoords, RectGetMid(Source), Rots.E[0]);
                        NormalizeTexCoords(GameState->GroundBrushTex, TexCoords);
                        FlipTexCoords(TexCoords);

                        vec2 VigTexCoords[4];
                        rect VigSource = Rect(GameState->VigTex.Width, GameState->VigTex.Height);
                        RectGetPoints(VigSource, VigTexCoords);
                        // Rotate4PointsAroundOrigin(VigTexCoords, RectGetMid(VigSource), Rots.E[1]);
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

                    MemoryArena_Unfreeze(&GameState->TrArenaA);
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

        MemoryArena_Freeze(&GameState->TrArenaA);
        
        int TileCount = GameState->World.Width * GameState->World.Height;
        vec3 *VertPositions = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec3);
        vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
        vec4 *VertColors = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
        int CurrentVert = 0;
        u32 *VertIndices = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 6, u32);
        int CurrentIndex = 0;
        
        for (int WorldI = 0; WorldI < TileCount; WorldI++)
        {
            u8 DarknessLevel = GameState->World.DarknessLevels[WorldI];
            if (GameState->World.DarknessLevels[WorldI] == DARKNESS_SEEN)
            {
                vec2i WorldP = IdxToXY(WorldI, GameState->World.Width);
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

        MemoryArena_Unfreeze(&GameState->TrArenaA);
    }
    EndCameraMode(); EndTextureMode();
}

void
DrawEntities(game_state *GameState)
{
    static_i b32 WillDrawRoomGround = false;

    if (KeyPressed(SDL_SCANCODE_G))
    {
        WillDrawRoomGround = !WillDrawRoomGround;
    }
    
    BeginDraw();
    {
        // NOTE: Draw entities
        BeginCameraMode(&GameState->Camera);
        {
            MemoryArena_Freeze(&GameState->TrArenaA);

            int TileCount = GameState->World.Width * GameState->World.Height;
            vec3 *VertPositions = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec3);
            vec4 *VertTexCoords = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
            vec4 *VertColors = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 4, vec4);
            int CurrentVert = 0;
            u32 *VertIndices = MemoryArena_PushArray(&GameState->TrArenaA, TileCount * 6, u32);
            int CurrentIndex = 0;

            for (int WorldI = 0; WorldI < TileCount; WorldI++)
            {
                u8 DarknessLevel = GameState->World.DarknessLevels[WorldI];
                if (DarknessLevel != DARKNESS_UNSEEN)
                {
                    entity *Entity = GameState->World.SpatialEntities[WorldI];

                    entity *TopCharacter = Entity;
                    while (TopCharacter)
                    {
                        if (TopCharacter->Type == ENTITY_NPC || TopCharacter->Type == ENTITY_PLAYER)
                        {
                            break;
                        }
                        TopCharacter = TopCharacter->Next;
                    }
                    entity *TopItemPickup = Entity;
                    while (TopItemPickup)
                    {
                        if (TopItemPickup->Type == ENTITY_ITEM_PICKUP)
                        {
                            break;
                        }
                        TopItemPickup = TopItemPickup->Next;
                    }
                    Entity = (TopCharacter ? TopCharacter : (TopItemPickup ? TopItemPickup : Entity));
                    
                    if (Entity)
                    {
                        if (DarknessLevel == DARKNESS_IN_VIEW || (Entity->Type != ENTITY_NPC))
                        {
                            vec2i WorldP = IdxToXY(WorldI, GameState->World.Width);
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
                BindTextureSlot(0, GameState->GlyphAtlas.T);
                DrawVertices(VertPositions, VertTexCoords, VertColors, VertIndices, CurrentVert, CurrentIndex);
                UnbindTextureSlot(0);
            }

            MemoryArena_Unfreeze(&GameState->TrArenaA);
        }
        EndCameraMode();
    }
    EndDraw();
}

void
ApplyHaimaBonus(int HaimaBonus, entity *Entity)
{
    Entity->HaimaBonus += HaimaBonus;
    
    Entity->MaxHealth += HaimaBonus;
    if (Entity->MaxHealth <= 0)
    {
        Entity->MaxHealth = 1;
    }

    Entity->Health += HaimaBonus;
    if (Entity->Health <= 0)
    {
        Entity->Health = 1;
    }
}

void
RemoveItemEffectsFromEntity(item *Item, entity *Entity)
{
    if (Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_NPC)
    {
        ApplyHaimaBonus(-Item->HaimaBonus, Entity);
    }
}

void
ApplyItemEffectsToEntity(item *Item, entity *Entity)
{
    if (Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_NPC)
    {
        ApplyHaimaBonus(Item->HaimaBonus, Entity);
    }
}

b32
AddItemToEntityInventory(item *Item, entity *Entity)
{
    if (Entity->Inventory)
    {
        item *EntityItemSlot = Entity->Inventory;
        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, EntityItemSlot++)
        {
            if (EntityItemSlot->ItemType == ITEM_NONE)
            {
                *EntityItemSlot = *Item;
                ApplyItemEffectsToEntity(Item, Entity);
                return true;
            }
        }
    }

    return false;
}

void
RemoveItemFromEntityInventory(item *Item, entity *Entity)
{
    if (Entity->Inventory)
    {
        item *EntityItemSlot = Entity->Inventory;
        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, EntityItemSlot++)
        {
            if (EntityItemSlot == Item)
            {
                RemoveItemEffectsFromEntity(EntityItemSlot, Entity);
                EntityItemSlot->ItemType = ITEM_NONE;
            }
        }
    }
}

void
RefreshItemPickupState(entity *ItemPickup)
{
    int I;
    for (I = 0; I < INVENTORY_SLOTS_PER_ENTITY; I++)
    {
        if (ItemPickup->Inventory[I].ItemType > ITEM_NONE)
        {
            break;
        }
    }

    if (I < INVENTORY_SLOTS_PER_ENTITY)
    {
        ItemPickup->Glyph = ItemPickup->Inventory[I].Glyph;
        ItemPickup->Color = ItemPickup->Inventory[I].Color;
    }
    else
    {
        ItemPickup->Health = 0;
    }
}

GAME_API void
UpdateAndRender(b32 *Quit, b32 Reloaded, game_memory GameMemory) 
{
    game_state *GameState = (game_state *) GameMemory.Data;

    b32 FirstFrame = false;

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

        // NOTE: Shaders
        GameState->GroundShader = BuildCustomShader("res/ground.vs", "res/ground.fs");
        BeginShaderMode(GameState->GroundShader);
        {
            SetUniformI("sprite", 1);
            SetUniformI("vig", 2);
        }
        EndShaderMode();

        // NOTE: Fonts
        GameState->TitleFont = SavLoadFont(&GameState->ResourceArena, "res/ProtestStrike-Regular.ttf", 32);
        GameState->BodyFont = SavLoadFont(&GameState->ResourceArena, "res/GildaDisplay-Regular.ttf", 28);
        // GameState->BodyFont = SavLoadFont(&GameState->ResourceArena, "res/ProtestStrike-Regular.ttf", 28);

        // NOTE: Textures
        GameState->GlyphAtlas.T = SavLoadTexture("res/NewFontCompromise.png");
        GameState->GlyphAtlas.GlyphsPerRow = 16;
        GameState->GlyphAtlas.GlyphPadX = 1;
        GameState->GlyphAtlas.GlyphPadY = 1;
        GameState->GlyphAtlas.GlyphPxW = 42;
        GameState->GlyphAtlas.GlyphPxH = 54;
        GameState->GlyphAtlas.GlyphPxWPad = GameState->GlyphAtlas.GlyphPxW + 2 * GameState->GlyphAtlas.GlyphPadX;
        GameState->GlyphAtlas.GlyphPxHPad = GameState->GlyphAtlas.GlyphPxH + 2 * GameState->GlyphAtlas.GlyphPadY;
        GameState->VigTex = GenerateVignette(&GameState->TrArenaA);
        GameState->GroundBrushTex = SavLoadTexture("res/GroundBrushes5.png");
        GameState->GroundBrushRect = Rect(GameState->GroundBrushTex.Width, GameState->GroundBrushTex.Width);
        GameState->PlayerPortraitTex = SavLoadTexture("res/PlayerPortrait.png");

        // NOTE: Render textures
        GameState->UiRect = Rect(GetWindowSize());
        GameState->UiRenderTex = SavLoadRenderTexture((int) GetWindowOrigSize().X, (int) GetWindowOrigSize().Y, false);
        GameState->LightingRenderTex = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);
        GameState->DebugOverlay = SavLoadRenderTexture((int) GetWindowSize().X, (int) GetWindowSize().Y, false);

        // NOTE: Audio
        // GameState->BackgroundMusic = LoadMusicStream("res/20240204 Calling.mp3");

        // NOTE: World gen
        GenerateWorld(GameState);

        TraceLog("Allocated %zu KB for %d entities. Each entity is %zu bytes.",
                 GameState->World.EntityMaxCount * sizeof(*GameState->World.Entities) / 1024,
                 GameState->World.EntityMaxCount,
                 sizeof(*GameState->World.Entities));

        GameState->Camera.Rotation = 0.0f;
        CameraInitLogZoomSteps(&GameState->Camera, 0.2f, 5.0f, 5);
        GameState->Camera.Offset = GetWindowSize() / 2.0f;
        UpdateCameraToWorldTarget(&GameState->Camera, &GameState->World, GameState->World.PlayerEntity->Pos);

        // NOTE: Ground drawing state
        int GroundPointsWidth = 32;
        int GroundPointsHeight = 40;
        GameState->GroundPointCount = GroundPointsWidth * GroundPointsHeight;
        GameState->GroundPoints = MemoryArena_PushArray(&GameState->WorldArena, GameState->GroundPointCount, vec2);
        GameState->GroundRots = MemoryArena_PushArray(&GameState->WorldArena, GameState->GroundPointCount, vec2);

        f32 WorldPxWidth = (f32) GameState->World.TilePxW * GameState->World.Width;
        f32 WorldPxHeight = (f32) GameState->World.TilePxH * GameState->World.Height;
        f32 GroundPointDistX = WorldPxWidth / (GroundPointsWidth - 1);
        f32 GroundPointDistY = WorldPxHeight / (GroundPointsHeight - 1);
        for (int i = 0; i < GameState->GroundPointCount; i++)
        {
            vec2i P = IdxToXY(i, GroundPointsWidth);
            f32 PxX = P.X * GroundPointDistX;
            f32 PxY = P.Y * GroundPointDistY;
            f32 RndX = (GetRandomFloat() * 0.1f - 0.05f) * GroundPointDistX;
            f32 RndY = (GetRandomFloat() * 0.1f - 0.05f) * GroundPointDistY;
            GameState->GroundPoints[i] = Vec2(PxX + RndX, PxY + RndY);

            f32 SpriteRot = GetRandomFloat() * 360.0f - 180.0f;
            f32 VigRot = GetRandomFloat() * 360.0f - 180.0f;
            GameState->GroundRots[i] = Vec2(SpriteRot, VigRot);
        }

        // NOTE: Misc
        // PlayMusicStream(GameState->BackgroundMusic);
        FirstFrame = true;
        GameState->IgnoreFieldOfView = false;

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
    }

    // SECTION: GAME LOGIC PRE UPDATE
    MemoryArena_Reset(&GameState->TrArenaA);

    world *World = &GameState->World;
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

    // SECTION: CHECK INPUTS
#ifdef SAV_DEBUG  
    if (KeyPressed(SDL_SCANCODE_B))
    {
        Breakpoint;

        entity **DebugEntities;
        int Count;
        GetAllEntitiesOfType(ENTITY_ITEM_PICKUP, World, &GameState->TrArenaA, &DebugEntities, &Count);

        GetAllEntitiesOfType(ENTITY_NPC, World, &GameState->TrArenaA, &DebugEntities, &Count);

        Noop;
    }
#endif

    if (KeyPressed(SDL_SCANCODE_F11)) ToggleWindowBorderless();

    if (MouseWheel() != 0) CameraIncreaseLogZoomSteps(&GameState->Camera, MouseWheel());
    if (MouseDown(SDL_BUTTON_MIDDLE)) GameState->Camera.Target -= CameraScreenToWorldRel(&GameState->Camera, GetMouseRelPos());

    // SECTION: GAME LOGIC UPDATE
    b32 PlayerFovDirty = false;

    switch (GameState->RunState)
    {
        case RUN_STATE_NONE:
        {
            GameState->RunState = RUN_STATE_PROCESSING_PLAYER;
        } break;

        case RUN_STATE_PROCESSING_PLAYER:
        {
            vec2i PlayerRequestedDP = Vec2I();
            b32 PlayerRequestedSkipTurn = false;
            b32 PlayerRequestedItemPickup = false;
            b32 PlayerRequestedItemDrop = false;
            b32 PlayerRequestedTeleport = false;
            b32 PlayerRequestedInventoryOpen = false;
            
            if (KeyPressedOrRepeat(SDL_SCANCODE_Q)) PlayerRequestedDP = Vec2I(-1, -1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_W)) PlayerRequestedDP = Vec2I( 0, -1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_E)) PlayerRequestedDP = Vec2I( 1, -1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_A)) PlayerRequestedDP = Vec2I(-1,  0);
            if (KeyPressedOrRepeat(SDL_SCANCODE_X)) PlayerRequestedDP = Vec2I( 0,  1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_D)) PlayerRequestedDP = Vec2I( 1,  0);
            if (KeyPressedOrRepeat(SDL_SCANCODE_Z)) PlayerRequestedDP = Vec2I(-1,  1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_C)) PlayerRequestedDP = Vec2I( 1,  1);
            if (KeyPressedOrRepeat(SDL_SCANCODE_S)) PlayerRequestedSkipTurn = true;
            if (KeyPressed(SDL_SCANCODE_G)) PlayerRequestedItemPickup = true;
            if (KeyPressed(SDL_SCANCODE_R)) PlayerRequestedItemDrop = true;
            if (KeyPressed(SDL_SCANCODE_T)) PlayerRequestedTeleport = true;
            if (KeyPressed(SDL_SCANCODE_I)) PlayerRequestedInventoryOpen = true;
            
            if (KeyPressed(SDL_SCANCODE_F))
            {
                GameState->IgnoreFieldOfView = !GameState->IgnoreFieldOfView;
                PlayerFovDirty = true;
            }

            entity *ActiveEntity = EntityTurnQueuePeek(World);
            if (ActiveEntity != Player)
            {
                GameState->RunState = RUN_STATE_PROCESSING_ENTITIES;
                break;
            }

            if (PlayerRequestedDP.X != 0 || PlayerRequestedDP.Y != 0 || PlayerRequestedSkipTurn || PlayerRequestedItemPickup || PlayerRequestedItemDrop || PlayerRequestedTeleport || PlayerRequestedInventoryOpen)
            {
                b32 TurnUsed = false;
                if (PlayerRequestedItemPickup)
                {
                    entity *ItemPickup = GetEntitiesAt(Player->Pos, World);
                    while (ItemPickup)
                    {
                        if (ItemPickup->Type == ENTITY_ITEM_PICKUP)
                        {
                            item *ItemFromPickup = ItemPickup->Inventory;
                            for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromPickup++)
                            {
                                if (ItemFromPickup->ItemType > ITEM_NONE)
                                {
                                    if (AddItemToEntityInventory(ItemFromPickup, Player))
                                    {
                                        // TODO: This is kind of a waste
                                        RemoveItemFromEntityInventory(ItemFromPickup, ItemPickup);
                                        TurnUsed = true;
                                    }
                                }
                            }

                            RefreshItemPickupState(ItemPickup);
                        }
                        
                        ItemPickup = ItemPickup->Next;
                    }
                }
                else if (PlayerRequestedItemDrop)
                {
                    if (Player->Inventory)
                    {
                        entity ItemPickupTestTemplate = {};
                        ItemPickupTestTemplate.Type = ENTITY_ITEM_PICKUP;
                        entity *ItemPickup = AddEntity(World, Player->Pos, &ItemPickupTestTemplate, &GameState->WorldArena);
                        Assert(ItemPickup->Inventory);

                        item *ItemFromInventory = Player->Inventory;
                        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromInventory++)
                        {
                            if (ItemFromInventory->ItemType > ITEM_NONE)
                            {
                                if (AddItemToEntityInventory(ItemFromInventory, ItemPickup))
                                {
                                    RemoveItemFromEntityInventory(ItemFromInventory, Player);
                                    TurnUsed = true;
                                }
                            }
                        }

                        RefreshItemPickupState(ItemPickup);
                    }
                }
                else if (PlayerRequestedSkipTurn)
                {
                    TurnUsed = true;
                }
                else if (PlayerRequestedInventoryOpen)
                {
                    GameState->RunState = RUN_STATE_INVENTORY_MENU;
                }
                else if (PlayerRequestedDP.X != 0 || PlayerRequestedDP.Y != 0 || PlayerRequestedTeleport)
                {
                    TraceLog("");
                    TraceLog("-------------Player makes a move--------------");
                
                    vec2i NewP;
                    b32 FoundPosition;
                    if (PlayerRequestedTeleport)
                    {
                        int Iter = 0;
                        int MaxIters = 10;
                        FoundPosition = true;
                        do
                        {
                            NewP = Vec2I(GetRandomValue(0, World->Width), GetRandomValue(0, World->Height));

                            if (Iter++ >= MaxIters)
                            {
                                FoundPosition = false;
                                break;
                            }
                        }
                        while (CheckCollisions(World, NewP).Collided);
                    }
                    else
                    {
                        FoundPosition = true;
                        NewP = World->PlayerEntity->Pos + PlayerRequestedDP;
                    }
                    
                    // NOTE: Move entity can set TurnUsed to false, if that was a non-attack collision
                    if (FoundPosition && MoveEntity(World, World->PlayerEntity, NewP, &TurnUsed))
                    {
                        UpdateCameraToWorldTarget(&GameState->Camera, World, NewP);
                        PlayerFovDirty = true;
                    }
                }

                if (TurnUsed)
                {
                    if (World->TurnsPassed - World->PlayerEntity->LastHealTurn > World->PlayerEntity->RegenActionCost &&
                        World->PlayerEntity->Health < World->PlayerEntity->MaxHealth && GetRandomValue(0, 2) == 0)
                    {
                        int RegenAmount = RollDice(1, World->PlayerEntity->RegenAmount);
                        World->PlayerEntity->Health += RegenAmount;
                        World->PlayerEntity->LastHealTurn = World->TurnsPassed;
                        TraceLog("Player regens %d health.", RegenAmount);
                    }

                    World->TurnsPassed += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
                    ActiveEntity = EntityTurnQueuePeek(World);
                }
            }

            if (ActiveEntity != Player)
            {
                GameState->RunState = RUN_STATE_PROCESSING_ENTITIES;
            }
        } break;

        case RUN_STATE_PROCESSING_ENTITIES:
        {
            entity *StartingEntity = EntityTurnQueuePeek(World);
            entity *ActiveEntity = StartingEntity;
            while (ActiveEntity != World->PlayerEntity)
            {
                if (ActiveEntity->Type == ENTITY_NPC)
                {
                    UpdateNpcState(GameState, World, ActiveEntity);
                }
        
                World->TurnsPassed += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
                ActiveEntity = EntityTurnQueuePeek(World);

                if (ActiveEntity == StartingEntity)
                {
                    break;
                }
            }

            if (ActiveEntity == Player)
            {
                GameState->RunState = RUN_STATE_PROCESSING_PLAYER;
            }
        } break;

        case RUN_STATE_INVENTORY_MENU:
        {
            if (KeyPressed(SDL_SCANCODE_I)) GameState->RunState = RUN_STATE_PROCESSING_PLAYER;

            if (GameState->PlayerRequestedDropItem && GameState->PlayerRequestedDropItem->ItemType != ITEM_NONE)
            {
                entity ItemPickupTestTemplate = {};
                ItemPickupTestTemplate.Type = ENTITY_ITEM_PICKUP;
                entity *ItemPickup = AddEntity(World, Player->Pos, &ItemPickupTestTemplate, &GameState->WorldArena);
                Assert(ItemPickup->Inventory);

                if (AddItemToEntityInventory(GameState->PlayerRequestedDropItem, ItemPickup))
                {
                    RemoveItemFromEntityInventory(GameState->PlayerRequestedDropItem, Player);
                }

                GameState->PlayerRequestedDropItem = NULL;

                RefreshItemPickupState(ItemPickup);
            }
        } break;

        default:
        {
            TraceLog("Unknown run state: %d", GameState->RunState);
            *Quit = true;
        };
    }

    // NOTE: Update player FOV and darkness levels
    if (PlayerFovDirty || FirstFrame)
    {
        if (!GameState->IgnoreFieldOfView)
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

            if (GameState->IgnoreFieldOfView || World->PlayerEntity->FieldOfView == 0 || World->PlayerEntity->FieldOfView[i] == 1)
            {
                World->DarknessLevels[i] = DARKNESS_IN_VIEW;
            }
        }
    }

    // SECTION: GAME LOGIC POST UPDATE
    // NOTE: Delete dead entities
    for (int i = 0; i < World->EntityUsedCount; i++)
    {
        entity *Entity = World->Entities + i;

        if (EntityExists(Entity) && EntityIsDead(Entity))
        {
            if (Entity->ActionCost > 0)
            {
                EntityTurnQueueDelete(World, Entity);
            }
            DeleteEntity(World, Entity);
        }
    }

    Assert(ValidateEntitySpatialPartition(&GameState->World));

    vec2 MouseP = GetMousePos();
    vec2 MouseWorldPxP = CameraScreenToWorld(&GameState->Camera, MouseP);
    vec2i MouseTileP = GetTilePFromPxP(World, MouseWorldPxP);
    entity *HighlightedEntity = IsInFOV(World, Player->FieldOfView, MouseTileP) ? GetEntitiesAt(MouseTileP, World) : NULL;
 
    // SECTION: RENDER
    // NOTE: Draw debug overlay
    BeginTextureMode(GameState->DebugOverlay, Rect(0)); BeginCameraMode(&GameState->Camera); 
    {
        ClearBackground(ColorAlpha(VA_WHITE, 0));

        if (HighlightedEntity != NULL && HighlightedEntity->FieldOfView)
        {
            for (int i = 0; i < World->Width * World->Width; i++)
            {
                if (HighlightedEntity->FieldOfView[i] == 1)
                {
                    DrawRect(World, IdxToXY(i, World->Width), ColorAlpha(VA_BLUE, 150));
                }
            }
        }
    }
    EndCameraMode(); EndTextureMode();

    // NOTE: Draw UI to its own render texture
    BeginTextureMode(GameState->UiRenderTex, GameState->UiRect);
    {
        ClearBackground(ColorAlpha(VA_WHITE, 0));

        DrawString(TextFormat("%0.3f FPS", GetFPSAvg(), GetDeltaAvg()),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   10, 10, 0,
                   VA_MAROON,
                   true, ColorAlpha(VA_BLACK, 128),
                   &GameState->TrArenaA);

        DrawString(TextFormat("%d, %d", MouseTileP.X, MouseTileP.Y, GameState->World.DarknessLevels[XYToIdx(MouseTileP.X, MouseTileP.Y, GameState->World.Width)]),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   10, 60, 0,
                   VA_MAROON,
                   true, ColorAlpha(VA_BLACK, 128),
                   &GameState->TrArenaA);

        DrawString(TextFormat("Darkness Level: %d", GameState->World.DarknessLevels[XYToIdx(MouseTileP.X, MouseTileP.Y, GameState->World.Width)]),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   10, 110, 0,
                   VA_MAROON,
                   true, ColorAlpha(VA_BLACK, 128),
                   &GameState->TrArenaA);

        DrawString(TextFormat("Turn: %lld", World->TurnsPassed),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   10, 160, 0,
                   VA_MAROON,
                   true, ColorAlpha(VA_BLACK, 128),
                   &GameState->TrArenaA);

        // NOTE: Inspect UI
        if (HighlightedEntity != NULL)
        {
            DrawRect(Rect(1500, 0, 420, 1080), ColorAlpha(VA_SLATEGRAY, 200));

            if (HighlightedEntity->Name)
            {
                DrawString(TextFormat("%s (%d)", HighlightedEntity->Name, HighlightedEntity->DebugID),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           1510, 10, 0,
                           VA_BLACK,
                           false, VA_BLACK,
                           &GameState->TrArenaA);
            }

            if (HighlightedEntity->MaxHealth > 0)
            {
                DrawString(TextFormat("HP: %d/%d", HighlightedEntity->Health, HighlightedEntity->MaxHealth),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           1510, 60, 0,
                           VA_BLACK,
                           false, VA_BLACK,
                           &GameState->TrArenaA);

                DrawString(TextFormat("AC: %d", HighlightedEntity->ArmorClass),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           1510, 110, 0,
                           VA_BLACK,
                           false, VA_BLACK,
                           &GameState->TrArenaA);

                // DrawString(TextFormat("Attack: %d", HighlightedEntity->AttackModifier),
                //            GameState->TitleFont,
                //            GameState->TitleFont->PointSize,
                //            1510, 160, 0,
                //            VA_BLACK,
                //            false, VA_BLACK,
                //            &GameState->TrArenaA);

                DrawString(TextFormat("Damage: 1d%d", HighlightedEntity->Damage),
                           GameState->TitleFont,
                           GameState->TitleFont->PointSize,
                           1510, 210, 0,
                           VA_BLACK,
                           false, VA_BLACK,
                           &GameState->TrArenaA);
            }
            
            if (HighlightedEntity->Description)
            {
                DrawString(HighlightedEntity->Description,
                           GameState->BodyFont,
                           GameState->BodyFont->PointSize,
                           1510, 260, 400,
                           VA_BLACK,
                           false, VA_BLACK,
                           &GameState->TrArenaA);
            }
        }

        // NOTE: Player stats UI
        {
            DrawRect(Rect(0, 780, 500, 1080), ColorAlpha(VA_SLATEGRAY, 200));

            DrawTexture(GameState->PlayerPortraitTex, Rect(10, 860, 128, 128), VA_WHITE);

            DrawString(TextFormat("HP: %d/%d", World->PlayerEntity->Health, World->PlayerEntity->MaxHealth),
                       GameState->TitleFont,
                       GameState->TitleFont->PointSize,
                       160, 790, 0,
                       VA_BLACK,
                       false, VA_BLACK,
                       &GameState->TrArenaA);

            DrawString(TextFormat("AC: %d", World->PlayerEntity->ArmorClass),
                       GameState->TitleFont,
                       GameState->TitleFont->PointSize,
                       160, 840, 0,
                       VA_BLACK,
                       false, VA_BLACK,
                       &GameState->TrArenaA);

            // DrawString(TextFormat("Attack: %d", World->PlayerEntity->AttackModifier),
            //            GameState->TitleFont,
            //            GameState->TitleFont->PointSize,
            //            160, 890, 0,
            //            VA_BLACK,
            //            false, VA_BLACK,
            //            &GameState->TrArenaA);

            DrawString(TextFormat("Damage: 1d%d", World->PlayerEntity->Damage),
                       GameState->TitleFont,
                       GameState->TitleFont->PointSize,
                       160, 940, 0,
                       VA_BLACK,
                       false, VA_BLACK,
                       &GameState->TrArenaA);
        }

        // NOTE: Inventory UI
        if (GameState->RunState == RUN_STATE_INVENTORY_MENU)
        {
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            f32 InventoryWidth = 500.0f;
            f32 InventoryHeight = 900.0f;
            rect InventoryRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - InventoryWidth * 0.5f,
                                      GameState->UiRenderTex.Texture.Height * 0.5f - InventoryHeight * 0.5f,
                                      InventoryWidth,
                                      InventoryHeight);
            DrawRect(InventoryRect, ColorAlpha(VA_SLATEGRAY, 240));

            item *InventoryItem = Player->Inventory;
            f32 LineX = InventoryRect.X + 10.0f;
            f32 LineY = InventoryRect.Y + 10.0f;
            for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, InventoryItem++)
            {
                if (InventoryItem->ItemType != ITEM_NONE)
                {
                    if (GuiButtonRect(Rect(LineX, LineY, InventoryWidth - 20.0f, 40.0f)) == SDL_BUTTON_RIGHT)
                    {
                        GameState->PlayerRequestedDropItem = InventoryItem;
                    }

                    DrawString(TextFormat("%s", InventoryItem->Name),
                               GameState->BodyFont,
                               GameState->BodyFont->PointSize,
                               LineX, LineY, 0,
                               VA_BLACK,
                               false, VA_BLACK,
                               &GameState->TrArenaA);

                    LineY += 40.0f;
                }
            }
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    EndTextureMode();

    DrawLighting(GameState);
    
    DrawGround(GameState);

    DrawEntities(GameState);

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
