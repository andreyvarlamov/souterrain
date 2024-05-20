#ifndef SOU_ENTITY_H
#define SOU_ENTITY_H

enum entity_flags
{
    ENTITY_IS_BLOCKING = 0x1,
    ENTITY_IS_OPAQUE = 0x2,
};

enum entity_type
{
    ENTITY_NONE = 0,
    ENTITY_WALL,
    ENTITY_CHARACTER,
    ENTITY_ITEM_PICKUP,
    ENTITY_STATUE,
    ENTITY_STAIR_DOWN,
    ENTITY_STAIR_UP,
    ENTITY_TYPE_COUNT
};

enum npc_state : u8
{
    NPC_STATE_NONE = 0,
    NPC_STATE_IDLE,
    NPC_STATE_HUNTING,
    NPC_STATE_SEARCHING,
    NPC_STATE_COUNT
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
        req_action_attack_ranged AttackRanged;
    };
};

struct health_stat
{
    f32 Current;
    f32 Max;
};

struct health_data
{
    health_stat Blood;
    health_stat Head;
    health_stat Chest;
    health_stat Stomach;
    health_stat LeftArm;
    health_stat RightArm;
    health_stat LeftLeg;
    health_stat RightLeg;
};

struct entity
{
    u8 Type;
    u32 Flags;

    vec2i Pos;
    
    u8 Glyph;
    color Color;

    int DebugID;

    int ViewRange;
    u8 *FieldOfView;

    u8 NpcState;
    vec2i Target;
    int SearchTurns;

    const char *Name;
    const char *Description;

    health_data HealthData;

    entity *Next;
};

struct collision_info
{
    entity *Entity;
    b32 Collided;
};

struct world;
struct game_state;

internal_func collision_info CheckCollisions(world *World, vec2i P, b32 IgnoreNPCs = false);
internal_func entity *AddEntity(world *World, vec2i Pos, entity *CopyEntity, b32 NeedFOV = false);
internal_func b32 UpdateEntity(world *World, entity *Entity, req_action *Action);

#endif
