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

//////////////////////////
// SECTION: LEVEL GEOMETRY
//////////////////////////

TEMPLATE_FUNC entity
Template_PumiceWall()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    
    Template.Type = ENTITY_WALL;
    Template.Color = VA_SLATEGRAY;
    Template.Glyph = 11 + 13*16;
    Template.Name = "Pumice Wall";
    Template.Description = "Wall made of soft pumice rock.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING | ENTITY_IS_OPAQUE);

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_LatenaStatue() 
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    
    Template.Type = ENTITY_STATUE;
    Template.Color = VA_GRAY;
    Template.Glyph = 9 + 14*16;
    Template.Name = "Statue to Latena";
    Template.Description = "The ox-eyed goddess of pride and imagery. According to the legends, the great god Xetel was so enamaoured by Latena, that he built Souterrain for her.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING);

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_XetelStatue()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    
    Template.Type = ENTITY_STATUE;
    Template.Color = VA_GRAY;
    Template.Glyph = 8 + 14*16;
    Template.Name = "Statue to Xetel";
    Template.Description = "The great god of glory and ambition. He was a de facto ruler of the worlds, until his wife Latena decimated his mind and consumed his body.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING);

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_StairsUp()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};

    Template.Type = ENTITY_STAIR_UP;
    Template.Color = VA_ORANGE;
    Template.Glyph = '<';
    Template.Name = "Stairs Up";
    Template.Description = "Press Shift+W to go to the higher level of Souterrain.";

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC entity
Template_StairsDown()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};

    Template.Type = ENTITY_STAIR_DOWN;
    Template.Color = VA_ORANGE;
    Template.Glyph = '>';
    Template.Name = "Stairs Down";
    Template.Description = "Press Shift+X to go to the lower level of Souterrain.";

    return Template;
}
#else
;
#endif

//////////////////////////
// SECTION: NPCS /////////
//////////////////////////

TEMPLATE_FUNC entity
Template_Player()
#ifdef TEMPLATE_EXPORTS
{
    entity Template = {};
    Template.Type = ENTITY_PLAYER;
    Template.Color = VA_YELLOW;
    Template.Glyph = '@';
    Template.ViewRange = 30;
    Template.RangedRange = 5;
    Template.Name = "Player";
    Template.Description = "After Derval's disappearance you awake in the Souterrain. You've read about this place in the dusty tomes kept in St Catherine's Library. You remember one thing: the only way is down.";
    SetFlags(&Template.Flags, ENTITY_IS_BLOCKING);

    Template.Haima = 20000;
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

//////////////////////////
// SECTION: ITEMS ////////
//////////////////////////

TEMPLATE_FUNC item
Template_Sword()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_MELEE;
    Template.Glyph = ')';
    Template.Color = VA_RED;
    Template.Name = "Old Ceremonial Sword";
    Template.Description = "A rusty ceremonial sword. It was used for pharmakoi sacrifice in the age of Souwarad Expansion.";

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
    Template.Color = VA_CHOCOLATE;
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
    Template.Color = VA_BROWN;
    Template.Name = "Short Bow";
    Template.Description = "A very common kind of short bow used by Ujik poachers to hunt game.";

    Template.HaimaBonus = 0;

    return Template;
}
#else
;
#endif

TEMPLATE_FUNC item
Template_ShoddyPickaxe()
#ifdef TEMPLATE_EXPORTS
{
    item Template = {};
    
    Template.ItemType = ITEM_PICKAXE;
    Template.Glyph = '?';
    Template.Color = VA_DIMGREY;
    Template.Name = "Shoddy Pickaxe";
    Template.Description = "A rusty old pickaxe. Must have been left by a careless miner in the age of Souward Expansion.";

    Template.WallDamage = 10;

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
// TODO: Plutonium sword
// TODO: Black compass

#endif
