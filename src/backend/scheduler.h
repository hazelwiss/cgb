#pragma once
#include<linked_list.h>

typedef struct CPU CPU;

typedef struct Scheduler{
    CPU* reference;
    LinkedList list;
} Scheduler;

void initScheduler(Scheduler*, CPU* cpu);

void scheduleEvent(Scheduler*, size_t cycle, EventFunc func);

void tickScheduler(Scheduler*);
