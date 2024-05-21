#ifndef SOU_TURN_QUEUE_H
#define SOU_TURN_QUEUE_H

#include "va_common.h"

struct entity;
struct turn_queue_node;

struct turn_queue
{
    turn_queue_node *NextNode;

    turn_queue_node *NodeStore;
    int NodeCount;
    int NodeMax;

    turn_queue_node *NextFreeNode;
};

internal_func turn_queue TurnQueueMake(memory_arena *Arena, int NodeMax);
internal_func void TurnQueueInsertEntity(turn_queue *TurnQueue, entity *Entity, int TicksToAct);
internal_func void TurnQueueRemoveEntity(turn_queue *TurnQueue, entity *Entity);
internal_func entity *TurnQueuePopEntity(turn_queue *TurnQueue);
internal_func entity *TurnQueuePeekEntity(turn_queue *TurnQueue);
internal_func int TurnQueueAdvanceTicks(turn_queue *TurnQueue);

#endif