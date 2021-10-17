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
        eventEvaluateInterrupts
};

void initScheduler(Scheduler* sched, CPU* cpu){
    sched->reference = cpu;
    sched->list_size = 0;
    sched->list[0].cycles = 0;
}

void scheduleEvent(Scheduler* sched, size_t cycles, EventEnum event){
    if(!EVENT_FUNCS[event] || sched->list_size + 1 > SCHED_MAX_ENTRIES)
        PANIC;
    uint64_t tot_cycles = sched->reference->t_cycles + cycles;
    EventFunc func = EVENT_FUNCS[event];
    bool is_duplicate = false;
    for(size_t i = 0; i < sched->list_size; ++i){
        if(sched->list[i].ee == event){
            if(i != 0){
                memmove(&sched->list[i], &sched->list[i+1], sizeof(*sched->list)*(SCHED_MAX_ENTRIES - i));
            } else{
                sched->list[0].cycles = UINT64_MAX;
                size_t index = 0;
                uint64_t value = UINT64_MAX;
                for(size_t i = 1; i < sched->list_size; ++i){
                    if(value > sched->list[i].cycles){
                        value = sched->list[i].cycles;
                        index = i;
                    }
                }
                memmove(sched->list, &sched->list[index], sizeof(*sched->list));
            }
            is_duplicate = true;
            break;
        }
    }
    if(tot_cycles > sched->list[0].cycles){
        sched->list[sched->list_size].cycles = tot_cycles;
        sched->list[sched->list_size].func = func;
        sched->list[sched->list_size].ee = event;
    } else{
        memmove(sched->list+1, sched->list, sizeof(sched->list)-sizeof(*sched->list));
        sched->list[0].cycles = tot_cycles; 
        sched->list[0].func = func;
        sched->list[0].ee = event;
    }
    sched->list_size += !is_duplicate;
}

void removeEvent(Scheduler* sched, EventEnum event){
    
}

void tickScheduler(Scheduler* sched){
    while(sched->list_size){
        if(sched->list[0].cycles <= sched->reference->t_cycles){
            EventFunc func = sched->list[0].func;
            size_t index = 1;
            uint64_t value = UINT64_MAX;
            for(size_t i = 1; i < sched->list_size; ++i){
                if(value > sched->list[i].cycles){
                    value = sched->list[i].cycles;
                    index = i;
                }
            }
            struct SchedulerEntry tmp;
            memcpy(&tmp, &sched->list[index], sizeof(tmp));
            if(sched->list_size < SCHED_MAX_ENTRIES || index != sched->list_size-1)
                memmove(&sched->list[index], &sched->list[index+1], sizeof(*sched->list)*(sched->list_size - index));
            memcpy(sched->list, &tmp, sizeof(tmp));
            --sched->list_size;
            func(sched);
        } else
            break;
    }
}