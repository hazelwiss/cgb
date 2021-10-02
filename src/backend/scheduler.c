#include"scheduler.h"
#include<backend/cpu.h>

EventFunc EVENT_FUNCS[eCOUNT] = {
    [eDI] =
        eventDI,
    [eEI] =
        eventEI,
    [eTIMER_INTERRUPT] =
        eventTimerInterrupt,
    [eEVALUATE_INTERRUPTS] =
        eventEvaluateInterrupts,
    [ePPUMODE0] = 
        eventPPUMode0,
    [ePPUMODE1] =
        eventPPUMode1,
    [ePPUMODE2] = 
        eventPPUMode2,
    [ePPUMODE3] =
        eventPPUMode3,
    [ePPUFRAME] =
        eventPPUFrame,
};

void initScheduler(Scheduler* sched, CPU* cpu){
    sched->reference = cpu;
    initList(&sched->list);
}

void scheduleEvent(Scheduler* sched, size_t cycles, EventEnum event){
    if(!EVENT_FUNCS[event])
        PANIC;
    listAddNode(&sched->list, sched->reference->t_cycles + cycles, EVENT_FUNCS[event]);
}

void removeEvent(Scheduler* sched, EventEnum event){
    
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