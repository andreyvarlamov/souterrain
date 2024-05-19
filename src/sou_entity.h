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

