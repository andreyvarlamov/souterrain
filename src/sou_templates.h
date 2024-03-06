#ifndef SOU_TEMPLATES_H
#define SOU_TEMPLATES_H

#ifdef TEMPLATE_EXPORTS
#define TEMPLATE_FUNC extern "C" __declspec(dllexport)
#else
#define TEMPLATE_FUNC extern "C" __declspec(dllimport)
#endif

#include "souterrain.h"

TEMPLATE_FUNC void
Template_DUMMY()
#ifdef TEMPLATE_EXPORTS
{
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_PumiceWall()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    
    Template.Type = ENTITY_STATIC;
    Template.Color = VA_SLATEGRAY;
    Template.Glyph = 11 + 16*13;
    Template.Name = "Pumice Wall";
    Template.Description = "Wall made of soft pumice rock.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING | ENTITY_IS_OPAQUE);

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_Player()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    Template.Type = ENTITY_PLAYER;
    Template.Color = VA_MAROON;
    Template.Glyph = '@';
    Template.ViewRange = 30;
    Template.Name = "Player";
    Template.Description = "After Derval's disappearance you awake in the Souterrain. You've read about this place in the dusty tomes kept in St Catherine's Library. You remember one thing: the only way is down.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING);

    Template.Haima = 17;
    Template.Kitrina = 9;
    Template.Melana = 5;
    Template.Sera = 3;
    
    Template.ArmorClass = 10 + Max(0, (Template.Kitrina - 5) / 2);
    Template.Damage = 2; // NOTE: From weapon
    Template.Health = Template.MaxHealth = Template.Haima;

    Template.ActionCost = 100;

    Template.RegenActionCost = 1000;
    Template.RegenAmount = 2;

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_AetherFly()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    Template.Type = ENTITY_NPC;
    Template.Color = VA_CORAL;
    Template.Glyph = 1 + 9*16;
    Template.ViewRange = 15;
    Template.NpcState = NPC_STATE_IDLE;
    Template.Name = "Aether Fly";
    Template.Description = "Sentient dipteron from the outer realms. Condemned to roam the Souterrain for eternity by the jealous goddess Latena.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING);

    Template.Haima = 3;
    Template.Kitrina = 5;
    Template.Melana = 1;
    Template.Sera = 1;
  
    Template.ArmorClass = 10 + Max(0, (Template.Kitrina - 5) / 2);
    Template.Damage = 1;
    Template.Health = Template.MaxHealth = Template.Haima;

    Template.ActionCost = 80;

    Template.RegenActionCost = 1500;
    Template.RegenAmount = 1;
    
    return Template;
}
#else
;
#endif

TEMPLATE_FUNC item
Template_Sword()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_MELEE;
    Template.Glyph = ')';
    Template.Color = VA_RED;
    Template.Name = "Short sword";
    Template.Description = "A sword";

    Template.HaimaBonus = 0;

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC item
Template_LeatherCuirass()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_CHEST;
    Template.Glyph = '<';
    Template.Color = VA_BROWN;
    Template.Name = "Leather Cuirass";
    Template.Description = "A leather cuirass";

    Template.HaimaBonus = 0;

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC item
Template_HaimaPotion()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_CONSUMABLE;
    Template.Glyph = '!';
    Template.Color = VA_YELLOW;
    Template.Name = "Haima Potion";
    Template.Description = "A haima potion";

    Template.HaimaBonus = 5;

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC item
Template_ShortBow()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_RANGED;
    Template.Glyph = '(';
    Template.Color = VA_GREEN;
    Template.Name = "Short Bow";
    Template.Description = "A short bow";

    Template.HaimaBonus = 0;

    return Template;
}
#else
;
#endif

// TODO: Aether ant
// TODO: Martyr worshipped by aether creatures (boss)
// TODO: Trees that are seemingly inanimate, but wake up after a delay
// TODO: Derval - main boss; Derval's ring - goal
// TODO: A vagaband - maybe boss? or friendly npc???????????????????????

#endif
