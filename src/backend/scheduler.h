#pragma once
#include <utility.h>
#include <backend/events.h>
#define SCHED_MAX_ENTRIES 256

typedef struct CPU CPU;

struct SchedulerEntry {
    uint64_t cycles;
    EventFunc func;
    EventEnum ee;
};

typedef struct Scheduler {
    CPU *reference;
    size_t list_size;
    struct SchedulerEntry list[SCHED_MAX_ENTRIES];
} Scheduler;

void initScheduler(Scheduler *, CPU *cpu);

void scheduleEvent(Scheduler *, size_t cycle, EventEnum event);
void removeEvent(Scheduler *, EventEnum event);

void tickScheduler(Scheduler *);
