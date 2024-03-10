#include "sav_lib.h"
#include "souterrain.h"

inline int
GetRangedRange(int Kitrina)
{
    return Kitrina;
}

inline int
GetFireballDamage(int Melana)
{
    return Melana / 2;
}

inline int
GetFireballRange(int Melana)
{
    return Melana;
}

inline int
GetFireballArea(int Melana)
{
    return Melana / 2;
}

inline int
GetRendMindDamage(int Sera)
{
    return Sera / 2;
}

inline int
GetRendMindRange(int Sera)
{
    return Sera * 2;
}

inline int
GetArmorClass(int Armor, int Kitrina)
{
    return Armor + Max(0, (Kitrina - 5) / 2);
}

inline int
GetMaxHealth(int Haima)
{
    return Haima;
}

inline int
GetMaxMana(int Melana)
{
    return Melana;
}

inline int
GetRegenAmount(int Haima)
{
    return Haima / 5;
}

inline void
SetEntityStatsBasedOnAttributes(entity *Entity)
{
    Entity->RangedRange = GetRangedRange(Entity->Kitrina);
    Entity->FireballDamage = GetFireballDamage(Entity->Melana);
    Entity->FireballRange = GetFireballRange(Entity->Melana);
    Entity->FireballArea = GetFireballArea(Entity->Melana);
    Entity->RendMindDamage = GetRendMindDamage(Entity->Sera);
    Entity->RendMindRange = GetRendMindRange(Entity->Sera);
    int OldMaxHealth = Entity->MaxHealth;
    Entity->MaxHealth = GetMaxHealth(Entity->Haima);
    Entity->Health += Entity->MaxHealth - OldMaxHealth;
    int OldMaxMana = Entity->MaxMana;
    Entity->MaxMana = GetMaxMana(Entity->Melana);
    Entity->Mana += Entity->MaxMana - OldMaxMana;
    Entity->RegenAmount = GetRegenAmount(Entity->Haima);

    Entity->ArmorClass = GetArmorClass(Entity->Armor, Entity->Kitrina);
}
