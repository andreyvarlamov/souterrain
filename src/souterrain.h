#ifndef SOUTERRAIN_H
#define SOUTERRAIN_H

#include "sav_lib.h"

#include "va_util.h"
#include "va_types.h"
#include "va_memarena.h"
#include "va_linmath.h"
#include "va_colors.h"

#ifndef SAV_DEBUG
#define SAV_DEBUG
#endif

enum entity_flags
{
    ENTITY_IS_BLOCKING = 0x1,
    ENTITY_IS_OPAQUE = 0x2,
};

enum entity_type
{
    ENTITY_NONE = 0,
    ENTITY_WALL,
    ENTITY_NPC,
    ENTITY_PLAYER,
    ENTITY_ITEM_PICKUP,
    ENTITY_TYPE_COUNT
};

enum npc_state
{
    NPC_STATE_NONE = 0,
    NPC_STATE_IDLE,
    NPC_STATE_HUNTING,
    NPC_STATE_SEARCHING,
    NPC_STATE_COUNT
};

enum item_type
{
    ITEM_NONE = 0,
    ITEM_MELEE,
    ITEM_RANGED,
    ITEM_HEAD,
    ITEM_CHEST,
    ITEM_ARMS,
    ITEM_LEGS,
    ITEM_CONSUMABLE,
    ITEM_PICKAXE,
    ITEM_COUNT
};

enum { INVENTORY_SLOTS_PER_ENTITY = 64 };

struct item
{
    u8 ItemType;
    u8 Glyph;
    color Color;

    const char *Name;
    const char *Description;

    int WallDamage;

    int HaimaBonus;
};

struct entity
{
    u8 Type;
    u32 Flags;

    vec2i Pos;
    
    u8 Glyph;
    color Color;

    int DebugID;

    int ActionCost;

    int ViewRange;
    int RangedRange;
    u8 *FieldOfView;

    u8 NpcState;
    vec2i Target;

    const char *Name;
    const char *Description;

    int Health;
    int MaxHealth;
    int ArmorClass;
    int Damage;

    i64 LastHealTurn;
    int RegenActionCost;
    int RegenAmount;

    int Haima;
    int Kitrina;
    int Melana;
    int Sera;

    int HaimaBonus;

    item *Inventory;

    entity *Next;
};

enum { ENTITY_MAX_COUNT = 16384 };

struct entity_queue_node
{
    entity *Entity;
    int LeftoverCost;
};

enum { DARKNESS_UNSEEN = 255, DARKNESS_SEEN = 180, DARKNESS_IN_VIEW = 0 };

struct room
{
    int X;
    int Y;
    int W;
    int H;
};

enum tile_type
{
    TILE_NONE = 0,
    TILE_STONE = 1,
    TILE_GRASS,
    TILE_WATER,
    TILE_COUNT
};

enum { OPEN_SET_MAX = 1024, PATH_MAX = 512 };

struct path_state
{
    int *OpenSet;
    int OpenSetCount;

    int *CameFrom;
    f32 *GScores;
    f32 *FScores;
    int MapSize;
};

struct path_result
{
    b32 FoundPath;
    vec2i *Path;
    int PathSteps;
};

struct world
{
    int Width;
    int Height;
    int TilePxW;
    int TilePxH;

    u8 *Tiles;
    u8 *TilesInitialized;
    u8 *DarknessLevels;

    entity *Entities;
    int EntityTightCount;
    int EntityUsedCount;
    int EntityMaxCount;
    int EntityCurrentDebugID;

    entity **SpatialEntities;

    entity_queue_node *EntityTurnQueue;
    int TurnQueueCount;
    int TurnQueueMax;

    i64 TurnsPassed;

    entity *PlayerEntity;
};

struct collision_info
{
    entity *Entity;
    b32 Collided;
};

enum run_state
{
    RUN_STATE_NONE = 0,
    RUN_STATE_PROCESSING_PLAYER,
    RUN_STATE_PROCESSING_ENTITIES,
    RUN_STATE_INVENTORY_MENU,
    RUN_STATE_PICKUP_MENU,
    RUN_STATE_RANGED_ATTACK,
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

enum inspect_type
{
    INSPECT_NONE = 0,
    INSPECT_ENTITY,
    INSPECT_ITEM_TO_PICKUP,
    INSPECT_ITEM_TO_DROP
};

struct inspect_state_entity
{
    entity *EntityToInspect;
};

struct inspect_state_item
{
    item *ItemToInspect;
    entity *ItemPickup;
};

struct inspect_state
{
    inspect_type T;

    b32 JustOpened;

    union
    {
        inspect_state_entity IS_Entity;
        inspect_state_item IS_Item;
    };
};

struct game_state
{
    b32 IsInitialized;

    memory_arena RootArena;
    memory_arena WorldArena;
    memory_arena ResourceArena;
    memory_arena TrArenaA;
    memory_arena TrArenaB;

    sav_font *TitleFont;
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

    music_stream BackgroundMusic;
    camera_2d Camera;

    rect UiRect;
    sav_render_texture UiRenderTex;
    
    sav_render_texture LightingRenderTex;
    sav_render_texture DebugOverlay;

    world World;

    vec2 *GroundPoints;
    vec2 *GroundRots;
    int GroundPointCount;

    b32 IgnoreFieldOfView;

    run_state RunState;

    item *PlayerRequestedDropItem;
    item *PlayerRequestedPickupItem;
    entity *PlayerRequestedPickupItemItemPickup;

    inspect_state InspectState;
    
    entity *EntityToHit;
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

inline vec2i IdxToXY(int I, int Width) { return Vec2I(I % Width, I / Width); }
inline int XYToIdx(vec2i P, int Width) { return P.Y * Width + P.X; }
inline int XYToIdx(int X, int Y, int Width) { return Y * Width + X; }
inline b32 CheckFlags(u32 Flags, u32 Mask) { return Flags & Mask; }
inline void SetFlags(u32 *Flags, u32 Mask) { *Flags |= Mask; }
inline void ClearFlags(u32 *Flags, u32 Mask) { *Flags &= ~Mask; }
inline b32 EntityExists(entity *Entity) { return Entity->Type > 0; }
inline b32 EntityIsDead(entity *Entity) { return Entity->Health <= 0; }

inline void
UpdateCameraToWorldTarget(camera_2d *Camera, world *World, vec2i WorldP)
{
    f32 TargetPxX = (f32) WorldP.X * World->TilePxW + World->TilePxW / 2.0f;
    f32 TargetPxY = (f32) WorldP.Y * World->TilePxH + World->TilePxH / 2.0f;
    Camera->Target = Vec2(TargetPxX, TargetPxY);
}

inline rect
GetWorldDestRect(world World, vec2i P)
{
    return Rect(P.X * World.TilePxW, P.Y * World.TilePxH, World.TilePxW, World.TilePxH);
}

inline rect
GetGlyphSourceRect(glyph_atlas Atlas, u8 Glyph)
{
    vec2i P = IdxToXY((int) Glyph, Atlas.GlyphsPerRow);
    int X = P.X * Atlas.GlyphPxWPad + Atlas.GlyphPadX;
    int Y = P.Y * Atlas.GlyphPxHPad + Atlas.GlyphPadY;
    return Rect(X, Y, Atlas.GlyphPxW, Atlas.GlyphPxH);
}

inline void
DrawRect(world *World, vec2i P, color Color)
{
    f32 X = (f32) P.X * World->TilePxW;
    f32 Y = (f32) P.Y * World->TilePxH;
    rect R = Rect(X, Y, (f32) World->TilePxW, (f32) World->TilePxH);
    DrawRect(R, Color);
}

inline vec2i
GetTilePFromPxP(world *World, vec2 PxP)
{
    vec2i TileP;
    TileP.X = (int) (PxP.X / World->TilePxW);
    TileP.Y = (int) (PxP.Y / World->TilePxH);
    return TileP;
}

inline rect
GetWorldCameraRect(camera_2d *Camera)
{
    vec2 WorldMin = CameraScreenToWorld(Camera, Vec2(0));
    vec2 WorldMax = CameraScreenToWorld(Camera, GetWindowSize());
                                        
    return RectMinMax(WorldMin, WorldMax);
}
    
inline b32
IsPInBounds(vec2i P, world *World)
{
    return (P.X >= 0 && P.X < World->Width && P.Y >= 0 && P.Y < World->Height);
}

inline b32
IsPInitialized(vec2i P, world *World)
{
    return  World->TilesInitialized[XYToIdx(P, World->Width)];
}

inline void
InitializeP(vec2i P, world *World)
{
    World->TilesInitialized[XYToIdx(P, World->Width)] = 1;
}

inline b32
IsPValid(vec2i P, world *World)
{
    return (IsPInBounds(P, World) && IsPInitialized(P, World));
}

inline int
RollDice(int DieCount, int DieValue)
{
    int DiceRoll = 0;
    
    for (int DieI = 0; DieI < DieCount; DieI++)
    {
        DiceRoll += GetRandomValue(1, DieValue + 1);
    }

    return DiceRoll;
}

inline b32
IsInFOV(world *World, u8 *FieldOfVision, vec2i Pos)
{
    return FieldOfVision[XYToIdx(Pos, World->Width)];
}

inline item *
IsItemTypeInEntityInventory(item_type ItemType, entity *Entity)
{
    if (Entity->Inventory)
    {
        item *EntityItemSlot = Entity->Inventory;
        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, EntityItemSlot++)
        {
            if (EntityItemSlot->ItemType == ItemType)
            {
                return EntityItemSlot;
            }
        }
    }

    return NULL;
}

#define LogEntityAction(Entity, World, Format, ...) do { if (IsInFOV(World, World->PlayerEntity->FieldOfView, Entity->Pos)) { TraceLog(Format, __VA_ARGS__); } } while (0)

#endif
