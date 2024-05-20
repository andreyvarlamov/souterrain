#ifndef SOUTERRAIN_H
#define SOUTERRAIN_H

#include "sav_lib.h"

#include "va_util.h"
#include "va_types.h"
#include "va_memarena.h"
#include "va_linmath.h"
#include "va_colors.h"

#include "sou_world.h"
#include "sou_entity.h"
#include "sou_player_input.h"
#include "sou_turn_queue.h"

enum { DARKNESS_UNSEEN = 255, DARKNESS_SEEN = 180, DARKNESS_IN_VIEW = 0 };

enum run_state
{
    RUN_STATE_NONE = 0,
    RUN_STATE_MAIN_MENU,
    RUN_STATE_LOAD_WORLD,
    RUN_STATE_IN_GAME,
    RUN_STATE_RANGED_ATTACK,
    RUN_STATE_INVENTORY_MENU,
    RUN_STATE_PICKUP_MENU,
    RUN_STATE_LEVELUP_MENU,
    RUN_STATE_QUIT,
    RUN_STATE_COUNT,
};

struct glyph_atlas
{
    sav_texture T;
    int GlyphsPerRow;
    int GlyphPadX;
    int GlyphPadY;
    int GlyphPxW;
    int GlyphPxH;
    int GlyphPxWPad;
    int GlyphPxHPad;
};

struct game_input
{
    vec2 MouseP;
    vec2 MouseWorldPxP;
    vec2i MouseWorldTileP;
};

struct game_state
{
    b32 IsInitialized;

    memory_arena ResourceArena;
    memory_arena ScratchArenaA;
    memory_arena ScratchArenaB;

    sav_font *TitleFont;
    sav_font *TitleFont64;
    sav_font *BodyFont;
    glyph_atlas GlyphAtlas;
    sav_texture GlyphAtlasNormalTex;
    sav_texture VigTex;
    sav_texture GroundBrushTex;
    rect GroundBrushRect; // TODO: tex + rect, atlas idiom?
    sav_shader GroundShader;
    sav_shader Glyph3DShader;
    sav_texture PlayerPortraitTex;
    sav_texture PlayerPortraitEyesTex;
    sav_texture HaimaTex;
    sav_texture KitrinaTex;
    sav_texture MelanaTex;
    sav_texture SeraTex;
    sav_texture TitleTex;

    music_stream BackgroundMusic;
    camera_2d Camera;

    rect UiRect;
    sav_render_texture UiRenderTex;
    
    sav_render_texture LightingRenderTex;
    sav_render_texture DebugOverlay;

    world *World;

    b32 IgnoreFieldOfView;
    b32 ShowDebugUI;
    
    game_input GameInput;
    req_action PlayerReqAction;

    run_state RunState;
};

static_g vec2i DIRECTIONS[] = {
    Vec2I( 0, -1),
    Vec2I( 1, -1),
    Vec2I( 1,  0),
    Vec2I( 1,  1),
    Vec2I( 0,  1),
    Vec2I(-1,  1),
    Vec2I(-1,  0),
    Vec2I(-1, -1)
};

internal_func inline vec2i IdxToXY(int I, int Width) { return Vec2I(I % Width, I / Width); }
internal_func inline int XYToIdx(vec2i P, int Width) { return P.Y * Width + P.X; }
internal_func inline int XYToIdx(int X, int Y, int Width) { return Y * Width + X; }
internal_func inline b32 CheckFlags(u32 Flags, u32 Mask) { return Flags & Mask; }
internal_func inline void SetFlags(u32 *Flags, u32 Mask) { *Flags |= Mask; }
internal_func inline void ClearFlags(u32 *Flags, u32 Mask) { *Flags &= ~Mask; }
internal_func inline b32 EntityExists(entity *Entity) { return Entity->Type > 0; }

internal_func inline rect
GetGlyphSourceRect(glyph_atlas Atlas, u8 Glyph)
{
    vec2i P = IdxToXY((int) Glyph, Atlas.GlyphsPerRow);
    int X = P.X * Atlas.GlyphPxWPad + Atlas.GlyphPadX;
    int Y = P.Y * Atlas.GlyphPxHPad + Atlas.GlyphPadY;
    return Rect(X, Y, Atlas.GlyphPxW, Atlas.GlyphPxH);
}

internal_func inline rect
GetWorldCameraRect(camera_2d *Camera)
{
    vec2 WorldMin = CameraScreenToWorld(Camera, Vec2(0));
    vec2 WorldMax = CameraScreenToWorld(Camera, GetWindowSize());
                                        
    return RectMinMax(WorldMin, WorldMax);
}

internal_func inline int
RollDice(int DieCount, int DieValue)
{
    int DiceRoll = 0;
    
    for (int DieI = 0; DieI < DieCount; DieI++)
    {
        DiceRoll += GetRandomValue(1, DieValue + 1);
    }

    return DiceRoll;
}

internal_func inline rect
GetWorldDestRect(world *World, vec2i P)
{
    return Rect(P.X * World->TilePxW, P.Y * World->TilePxH, World->TilePxW, World->TilePxH);
}

internal_func inline void
DrawRect(world *World, vec2i P, color Color)
{
    f32 X = (f32) P.X * World->TilePxW;
    f32 Y = (f32) P.Y * World->TilePxH;
    rect R = Rect(X, Y, (f32) World->TilePxW, (f32) World->TilePxH);
    DrawRect(R, Color);
}

internal_func inline vec2i
GetTilePFromPxP(world *World, vec2 PxP)
{
    vec2i TileP;
    TileP.X = (int) (PxP.X / World->TilePxW);
    TileP.Y = (int) (PxP.Y / World->TilePxH);
    return TileP;
}

internal_func inline vec2
GetPxPFromTileP(world *World, vec2i TileP)
{
    vec2 PxP;
    PxP.X = (f32) TileP.X * World->TilePxW + World->TilePxW / 2.0f;
    PxP.Y = (f32) TileP.Y * World->TilePxH + World->TilePxH / 2.0f;
    return PxP;
}

internal_func inline b32
IsPositionInCameraView(vec2i P, camera_2d *Camera, world *World)
{
    rect CameraRect = GetWorldCameraRect(Camera);
    vec2 PxP = GetPxPFromTileP(World, P);
    return IsPInRect(PxP, CameraRect);
}

internal_func inline b32
IsPInBounds(vec2i P, world *World)
{
    return (P.X >= 0 && P.X < World->Width && P.Y >= 0 && P.Y < World->Height);
}

internal_func inline b32
IsPInitialized(vec2i P, world *World)
{
    return  World->TilesInitialized[XYToIdx(P, World->Width)];
}

internal_func inline void
InitializeP(vec2i P, world *World)
{
    World->TilesInitialized[XYToIdx(P, World->Width)] = 1;
}

internal_func inline b32
IsPValid(vec2i P, world *World)
{
    return (IsPInBounds(P, World) && IsPInitialized(P, World));
}

internal_func inline b32
IsInFOV(world *World, u8 *FieldOfVision, vec2i Pos)
{
    return FieldOfVision[XYToIdx(Pos, World->Width)];
}

#define LogEntityAction(Entity, World, Format, ...) do { if (IsInFOV(World, World->PlayerEntity->FieldOfView, Entity->Pos)) { TraceLog(Format, __VA_ARGS__); } } while (0)

#endif
