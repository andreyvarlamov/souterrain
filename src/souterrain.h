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

#include "sou_entity.h"

enum { INVENTORY_SLOTS_PER_ENTITY = 64 };

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

struct item
{
    u8 ItemType;
    u8 Glyph;
    color Color;

    const char *Name;
    const char *Description;

    int WallDamage;

    int HaimaBonus;
    int AC;
    int Damage;
    int RangedDamage;
};

enum { ENTITY_MAX_COUNT = 16384 };

struct entity_queue_node
{
    entity *Entity;
    int LeftoverCost;
};

enum { DARKNESS_UNSEEN = 255, DARKNESS_SEEN = 180, DARKNESS_IN_VIEW = 0 };
 
enum room_type
{
    ROOM_NONE,
    ROOM_ENTRANCE,
    ROOM_SACRIFICIAL,
    ROOM_WASTE,
    ROOM_TEMPLE,
    ROOM_XLARGE,
    ROOM_LARGE,
    ROOM_MEDIUM,
    ROOM_SMALL,
    ROOM_EXIT
};

enum gen_tile_type
{
    GEN_TILE_NONE = 0,
    GEN_TILE_FLOOR,
    GEN_TILE_WALL,
    GEN_TILE_CORRIDOR,
    GEN_TILE_STATUE,
    GEN_TILE_STAIR_UP,
    GEN_TILE_STAIR_DOWN,
};

struct room
{
    int X;
    int Y;
    int W;
    int H;

    room_type Type;
};
 
enum tile_type
{
    TILE_NONE = 0,
    TILE_STONE = 1,
    TILE_GRASS,
    TILE_WATER,
    TILE_COUNT
};

enum { OPEN_SET_MAX = 1024, PATHFIND_MAX_ITERATIONS = 1024, PATH_MAX = 512 };

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

struct ground_splat
{
    vec2 Position;
    f32 Rotation;
    f32 Scale;
};

struct swarm
{
    i64 Cooldown;
};

enum { MAX_SWARMS = 64 };

struct world
{
    memory_arena Arena;
    
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

    int GroundSplatCount;
    ground_splat *GroundSplats;

    int SwarmCount;
    swarm Swarms[MAX_SWARMS];
    
    entity *PlayerEntity;

    i64 CurrentTurn;

    vec2i Exit;
};

struct collision_info
{
    entity *Entity;
    b32 Collided;
};

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

enum inspect_type
{
    INSPECT_NONE = 0,
    INSPECT_ENTITY,
    INSPECT_ITEM
};

struct inspect_state_entity
{
    entity *EntityToInspect;
};

struct inspect_state_item
{
    item *ItemToInspect;
    entity *InventoryEntity;
    int MenuSlotI;
};

struct inspect_state
{
    inspect_type T;

    b32 JustOpened;
    b32 ButtonClicked;
    const char *ButtonLabel;

    union
    {
        inspect_state_entity IS_Entity;
        inspect_state_item IS_Item;
    };
};

/* struct run_state_ */

/* struct run_state */
/* { */
/*     run_state_type T; */

/*     union */
/*     { */
        
/*     }; */
/* }; */

enum { MAX_WORLDS = 32 };

struct game_input
{
    vec2 MouseP;
    vec2 MouseWorldPxP;
    vec2i MouseWorldTileP;
};

enum req_action_type
{
    ACTION_NONE = 0,
    ACTION_MOVE,
    ACTION_SKIP_TURN,
    ACTION_TELEPORT,
    ACTION_OPEN_INVENTORY,
    ACTION_OPEN_PICKUP,
    ACTION_NEXT_LEVEL,
    ACTION_PREV_LEVEL,
    ACTION_DROP_ALL_ITEMS,
    ACTION_DROP_ITEMS,
    ACTION_PICKUP_ALL_ITEMS,
    ACTION_PICKUP_ITEMS,
    ACTION_START_RANGED_ATTACK,
    ACTION_START_FIREBALL,
    ACTION_START_RENDMIND,
    ACTION_ATTACK_RANGED,
};

struct req_action_drop_items
{
    int ItemCount;
    item *Items[INVENTORY_SLOTS_PER_ENTITY];
};

struct req_action_pickup_items
{
    int ItemCount;
    item *Items[INVENTORY_SLOTS_PER_ENTITY];
    entity *ItemPickups[INVENTORY_SLOTS_PER_ENTITY];
};

enum ranged_attack_type
{
    RANGED_NONE,
    RANGED_WEAPON,
    RANGED_FIREBALL,
    RANGED_RENDMIND
};

struct req_action_attack_ranged
{
    vec2i Target;
    ranged_attack_type Type;
};

struct req_action
{
    req_action_type T;

    union
    {
        vec2i DP;
        req_action_drop_items DropItems;
        req_action_pickup_items PickupItems;
        req_action_attack_ranged AttackRanged;
    };
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
    int CurrentWorld;
    world *OtherWorlds[MAX_WORLDS];

    b32 IgnoreFieldOfView;
    b32 ShowDebugUI;
    
    game_input GameInput;
    req_action PlayerReqAction;

    run_state RunState;

    inspect_state InspectState;

    ranged_attack_type RangedAttackType;
    
    b32 InventorySkipSlot[INVENTORY_SLOTS_PER_ENTITY];
    b32 PickupSkipSlot[INVENTORY_SLOTS_PER_ENTITY];
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

inline rect
GetWorldDestRect(world *World, vec2i P)
{
    return Rect(P.X * World->TilePxW, P.Y * World->TilePxH, World->TilePxW, World->TilePxH);
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

inline vec2
GetPxPFromTileP(world *World, vec2i TileP)
{
    vec2 PxP;
    PxP.X = (f32) TileP.X * World->TilePxW + World->TilePxW / 2.0f;
    PxP.Y = (f32) TileP.Y * World->TilePxH + World->TilePxH / 2.0f;
    return PxP;
}
 
inline rect
GetWorldCameraRect(camera_2d *Camera)
{
    vec2 WorldMin = CameraScreenToWorld(Camera, Vec2(0));
    vec2 WorldMax = CameraScreenToWorld(Camera, GetWindowSize());
                                        
    return RectMinMax(WorldMin, WorldMax);
}

inline b32
IsPositionInCameraView(vec2i P, camera_2d *Camera, world *World)
{
    rect CameraRect = GetWorldCameraRect(Camera);
    vec2 PxP = GetPxPFromTileP(World, P);
    return IsPInRect(PxP, CameraRect);
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

#define LogEntityAction(Entity, World, Format, ...) do { if (IsInFOV(World, World->PlayerEntity->FieldOfView, Entity->Pos)) { TraceLog(Format, __VA_ARGS__); } } while (0)

#endif
