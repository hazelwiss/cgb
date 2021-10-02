#pragma once
#include<linked_list.h>
#include<backend/events.h>

typedef struct CPU CPU;

typedef struct Scheduler{
    CPU* reference;
    LinkedList list;
} Scheduler;

void initScheduler(Scheduler*, CPU* cpu);

void scheduleEvent(Scheduler*, size_t cycle, EventEnum event);
void removeEvent(Scheduler*, EventEnum event);

void tickScheduler(Scheduler*);
