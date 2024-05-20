#ifndef SOU_TURN_QUEUE_H
#define SOU_TURN_QUEUE_H

#include "sav_lib.h"

struct entity;

struct turn_queue_node
{
    entity *Entity;
    int TicksToAct;

    turn_queue_node *Previous;
    turn_queue_node *Next;
};

struct turn_queue
{
    turn_queue_node *NextNode;

    turn_queue_node *NodeStore;
    int NodeCount;
    int NodeMax;

    turn_queue_node *NextFreeNode;
};

turn_queue TurnQueueMake(memory_arena *Arena, int NodeMax);
turn_queue_node *TurnQueueGetFreeNode(turn_queue *TurnQueue);
void TurnQueueDeleteNode(turn_queue *TurnQueue, turn_queue_node *Node);
void TurnQueueInsertEntity(turn_queue *TurnQueue, entity *Entity, int TicksToAct);
int TurnQueueAdvanceTicks(turn_queue *TurnQueue);
entity *TurnQueuePeekEntity(turn_queue *TurnQueue);
entity *TurnQueuePopEntity(turn_queue *TurnQueue);

#endif