#include"scheduler.h"
#include<backend/cpu.h>

void initScheduler(Scheduler* sched, CPU* cpu){
    sched->reference = cpu;
    initList(&sched->list);
}

void scheduleEvent(Scheduler* sched, size_t cycles, EventFunc func){
    listAddNode(&sched->list, sched->reference->t_cycles + cycles, func);
}

void tickScheduler(Scheduler* sched){
    while(sched->list.begin){
        if((*sched->list.begin).cycles <=  sched->reference->t_cycles){
            (*sched->list.begin).func(sched);
            listRemoveFirstNode(&sched->list);
        } else
            break;
    }
}