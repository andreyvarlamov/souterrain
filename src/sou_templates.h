#ifndef SOU_TEMPLATES_H
#define SOU_TEMPLATES_H

#ifdef TEMPLATE_EXPORTS
#define TEMPLATE_FUNC extern "C" __declspec(dllexport)
#else
#define TEMPLATE_FUNC extern "C" __declspec(dllimport)
#endif

#include "souterrain.h"

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

// TODO: Aether ant
// TODO: Martyr worshipped by aether creatures (boss)
// TODO: Trees that are seemingly inanimate, but wake up after a delay
// TODO: Derval - main boss; Derval's ring - goal
// TODO: A vagaband - maybe boss? or friendly npc???????????????????????

#endif
